/*
 * White Rabbit RTU (Routing Table Unit)
 * Copyright (C) 2010, CERN.
 *
 * Version:     wrsw_rtud v1.0
 *
 * Authors:     Juan Luis Manas (juan.manas@integrasys.es)
 *              Miguel Baizan   (miguel.baizan@integrasys.es)
 *              Maciej Lipinski (maciej.lipinski@cern.ch)
 *
 * Description: RTU driver module in user space. Provides read/write access
 *              to RTU_at_HW  components including:
 *              - UFIFO
 *              - MFIFO
 *              - Aging RAM for Main Hashtable
 *              - VLAN Table
 *              - RTU Global Control Register
 *              - RTU Port settings
 *
 * Fixes:
 *              Alessandro Rubini
 *              Tomasz Wlostowski
 *
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stddef.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/ioctl.h>


#include <switch_hw.h>
#include <hal_client.h>

#include <fpga_io.h>
#include <regs/rtu-regs.h>

#include "rtu_drv.h"
#include "wr_rtu.h"


extern int shw_fpga_mmap_init();

static void write_mfifo_addr(uint32_t zbt_addr);
static void write_mfifo_data(uint32_t word);

static uint32_t mac_entry_word0_w(struct filtering_entry *ent);
static uint32_t mac_entry_word1_w(struct filtering_entry *ent);
static uint32_t mac_entry_word2_w(struct filtering_entry *ent);
static uint32_t mac_entry_word3_w(struct filtering_entry *ent);
static uint32_t mac_entry_word4_w(struct filtering_entry *ent);

/*
 * Used to communicate to RTU UFIFO IRQ handler device at kernel space
 */
static int fd;

#define HAL_CONNECT_RETRIES 1000
#define HAL_CONNECT_TIMEOUT 2000000 /* us */

/**
 * \brief Initialize HW RTU memory map
 */

int rtu_init(void)
{
    int err;

    if(halexp_client_try_connect(HAL_CONNECT_RETRIES, HAL_CONNECT_TIMEOUT) < 0)
    {
        TRACE(
            TRACE_FATAL,
            "The HAL is not responding... Are you sure it's running on your switch?\n"
            );
				exit(-1);
		}

    // Used to 'get' RTU IRQs from kernel
	fd = open(RTU_DEVNAME, O_RDWR);
	if (fd < 0)
	{
   		TRACE(TRACE_ERROR, "Can't open %s: is the RTU kernel driver loaded?", RTU_DEVNAME);
		return errno;
	}
    // init IO memory map
    err = shw_fpga_mmap_init();
    if(err)
        return err;

    TRACE(TRACE_INFO,"module initialized\n");

    return 0;
}

void rtu_exit(void)
{
    if(fd >= 0)
        close(fd);

    TRACE(TRACE_INFO, "module cleanup\n");
}


#define rtu_rd(reg) \
	 _fpga_readl(FPGA_BASE_RTU + offsetof(struct RTU_WB, reg))

#define rtu_wr(reg, val) \
	 _fpga_writel(FPGA_BASE_RTU + offsetof(struct RTU_WB, reg), val)

static inline void write_pcr(int port, uint32_t pcr)
{
	rtu_wr(PSR, RTU_PSR_PORT_SEL_W(port));
	rtu_wr(PCR, pcr);
}

static inline uint32_t read_pcr(int port)
{
	rtu_wr(PSR, RTU_PSR_PORT_SEL_W(port));
	return rtu_rd(PCR);
}

// UFIFO

/**
 * \brief Returns the UFIFO empty flag.
 * @return Value of UFIFO empty flag.
 */
int rtu_ufifo_is_empty(void)
{
    uint32_t csr =  rtu_rd( UFIFO_CSR);
    return RTU_UFIFO_CSR_EMPTY & csr;
}

/**
 * \brief Returns the current learning queue length.
 * @return Number of unrecognised requests currently in the learning queue.
 */
int rtu_read_learning_queue_cnt(void)
{
    // Get counter from UFIFO Control-Status Register
    // Fixme: USEDW returns 0 (FIFO overflow?)
    uint32_t csr = rtu_rd( UFIFO_CSR);
    return RTU_UFIFO_CSR_USEDW_R(csr);
}

/**
 * \brief Reads unrecognised request from UFIFO.
 * Blocks on read if learning queue is empty.
 * @param req pointer to unrecognised request data. Memory handled by callee.
 * @return error code
 */

static int irq_disabled = 1;

int rtu_read_learning_queue(struct rtu_request *req)
{
    int err;

		if(irq_disabled)
		{
        ioctl(fd, WR_RTU_IRQENA);
        irq_disabled = 0;
		}

    // If learning queue is empty, wait for UFIFO IRQ
    if (rtu_ufifo_is_empty()) {
        err = ioctl(fd, WR_RTU_IRQWAIT);
        if (err && (err != -EAGAIN))
            return err;
    }


    // read data from mapped IO memory
    uint32_t r0 = rtu_rd( UFIFO_R0);
    uint32_t r1 = rtu_rd( UFIFO_R1);
    uint32_t r2 = rtu_rd( UFIFO_R2);
    uint32_t r3 = rtu_rd( UFIFO_R3);
    uint32_t r4 = rtu_rd( UFIFO_R4);

    // Once read: if learning queue becomes empty again, enable UFIFO IRQ

    // unmarshall data and populate request
    uint32_t dmac_lo = RTU_UFIFO_R0_DMAC_LO_R(r0);
    uint32_t dmac_hi = RTU_UFIFO_R1_DMAC_HI_R(r1);
    uint32_t smac_lo = RTU_UFIFO_R2_SMAC_LO_R(r2);
    uint32_t smac_hi = RTU_UFIFO_R3_SMAC_HI_R(r3);

    req->port_id  = RTU_UFIFO_R4_PID_R(r4);
    req->has_prio = RTU_UFIFO_R4_HAS_PRIO & r4;
    req->prio     = RTU_UFIFO_R4_PRIO_R(r4);
    req->has_vid  = RTU_UFIFO_R4_HAS_VID & r4;
    req->vid      = RTU_UFIFO_R4_VID_R(r4);
    // destination mac
    req->dst[5]   = 0xFF &  dmac_lo;
    req->dst[4]   = 0xFF & (dmac_lo >> 8);
    req->dst[3]   = 0xFF & (dmac_lo >> 16);
    req->dst[2]   = 0xFF & (dmac_lo >> 24);
    req->dst[1]   = 0xFF &  dmac_hi;
    req->dst[0]   = 0xFF & (dmac_hi >> 8);
    // source mac
    req->src[5]   = 0xFF &  smac_lo;
    req->src[4]   = 0xFF & (smac_lo >> 8);
    req->src[3]   = 0xFF & (smac_lo >> 16);
    req->src[2]   = 0xFF & (smac_lo >> 24);
    req->src[1]   = 0xFF &  smac_hi;
    req->src[0]   = 0xFF & (smac_hi >> 8);

	  ioctl(fd, WR_RTU_IRQENA);

    return 0;
}


// MFIFO -> HTAB

/**
 * \brief Returns the current main hashtable CPU access FIFO (MFIFO) length.
 * @return Number of MAC entries currently in the MFIFO.
 */
int rtu_read_mfifo_cnt(void)
{
    // Get counter from MFIFO Control-Status Register
    uint32_t csr = rtu_rd( MFIFO_CSR);
    return RTU_MFIFO_CSR_USEDW_R(csr);
}

/**
 * \brief Checks whether the main hashtable CPU access FIFO (MFIFO) is full.
 * @return 1 if MFIFO is full. 0 otherwise.
 */
int rtu_mfifo_is_full(void)
{
    uint32_t csr = rtu_rd( MFIFO_CSR);
    return RTU_MFIFO_CSR_FULL & csr;
}

/**
 * \brief Checks whether the main hashtable CPU access FIFO (MFIFO) is empty.
 * @return 1 if MFIFO is empty. 0 otherwise.
 */
int rtu_mfifo_is_empty(void)
{
    uint32_t csr = rtu_rd( MFIFO_CSR);
    return RTU_MFIFO_CSR_EMPTY & csr;
}

static void flush_mfifo()
{
	uint32_t gcr = rtu_rd (GCR);
	rtu_wr(GCR, gcr | RTU_GCR_MFIFOTRIG);

	while(!rtu_rd(GCR) & RTU_GCR_MFIFOTRIG); /* wait while the RTU is busy flushing the MFIFO */
}

/**
 * \brief Writes one MAC entry into main hash table at the given address.
 * @param ent MAC table entry to be written to MFIFO.
 * @param zbt_addr ZBT SRAM memory address in which MAC entry shoud be added.
 */
void rtu_write_htab_entry(uint16_t zbt_addr, struct filtering_entry *ent, int flush)
{
    write_mfifo_addr(zbt_addr);
    write_mfifo_data(mac_entry_word0_w(ent));
    write_mfifo_data(mac_entry_word1_w(ent));
    write_mfifo_data(mac_entry_word2_w(ent));
    write_mfifo_data(mac_entry_word3_w(ent));
    write_mfifo_data(mac_entry_word4_w(ent));

		if(flush)
			flush_mfifo();

    TRACE_DBG(
        TRACE_INFO,
        "write htab entry [with flush]: addr %x ent %08x %08x %08x %08x %08x",
        zbt_addr,
        mac_entry_word0_w(ent),
        mac_entry_word1_w(ent),
        mac_entry_word2_w(ent),
        mac_entry_word3_w(ent),
        mac_entry_word4_w(ent)
    );
}

/**
 * \brief Cleans main hash table.
 * Cleans all entries in HTAB inactive bank.
 */
void rtu_clean_htab(void)
{
    int addr;
    for (addr = 0; addr < RTU_ENTRIES; addr++) {
        write_mfifo_addr(addr * 8);
        write_mfifo_data(0x00000000);
        write_mfifo_data(0x00000000);
        write_mfifo_data(0x00000000);
        write_mfifo_data(0x00000000);
        write_mfifo_data(0x00000000);
		flush_mfifo();
    }
}


// AGING RAM - HTAB

/**
 * \brief Read word from aging HTAB.
 * Aging RAM Size: 256 32-bit words
 */
// tmp hack-fix
#define RTU_ARAM_BASE 0x400 

void rtu_read_aging_bitmap( uint32_t *bitmap )
{
	int i;
	for(i=0; i< RTU_ENTRIES / 32; i++)
	{
   	bitmap[i] = _fpga_readl(FPGA_BASE_RTU + RTU_ARAM_BASE + 4*i);
    _fpga_writel(FPGA_BASE_RTU + RTU_ARAM_BASE + 4*i, 0);	  
	}
}

// VLAN TABLE

/**
 * \brief Writes entry to vlan table.
 * VLAN table size: 4096 32-bit words.
 * @param addr entry memory address
 */
void rtu_write_vlan_entry(int vid, struct vlan_table_entry *ent)
{
 	uint32_t vtr1=0, vtr2=0;

  vtr2 = ent->port_mask;
  vtr1 = RTU_VTR1_UPDATE
					| RTU_VTR1_VID_W(vid)
          | (ent->drop ? RTU_VTR1_DROP : 0)
          | (ent->prio_override ? RTU_VTR1_PRIO_OVERRIDE : 0)
          | (ent->has_prio ? RTU_VTR1_HAS_PRIO : 0)
          | RTU_VTR1_PRIO_W(ent->prio)
          | RTU_VTR1_FID_W(ent->fid);

	rtu_wr(VTR2,  vtr2);
	rtu_wr(VTR1,  vtr1);

	if(ent->drop > 0 && ent->port_mask == 0x0)
	{
	  TRACE(TRACE_INFO, "RemoveVlan: vid %d (fid %d)", vid, ent->fid);
	}
	else
	{
	  TRACE(TRACE_INFO, "AddVlan: vid %d (fid %d) port_mask 0x%x", vid, ent->fid, ent->port_mask);
	}

}

/**
 * \brief Cleans VLAN entry in VLAN table
 * @param addr memory address which shoud be cleaned.
 */
void rtu_clean_vlan_entry( int vid )
{
 	uint32_t vtr1, vtr2;

  vtr2 = 0;
  vtr1 = RTU_VTR1_UPDATE | RTU_VTR1_VID_W(vid);

	rtu_wr(VTR2,  vtr2);
	rtu_wr(VTR1,  vtr1);
}

/**
 * \brief Cleans VLAN table (drop bit is set to 1)
 */
void rtu_clean_vlan(void)
{
    int addr;
   	for (addr = 0; addr < NUM_VLANS; addr++)
			rtu_clean_vlan_entry(addr);
}


// RTU Global Control Register

/**
 * \brief Enables RTU operation.
 */
void rtu_enable(void)
{
	uint32_t gcr = rtu_rd( GCR);
	rtu_wr(GCR, gcr | RTU_GCR_G_ENA);
       TRACE(TRACE_INFO,"updated gcr (enable): %x\n", gcr);
}

/**
 * \brief Disables RTU operation.
 */
void rtu_disable(void)
{
	uint32_t gcr = rtu_rd( GCR);
	rtu_wr(GCR, gcr & (~RTU_GCR_G_ENA));
       TRACE(TRACE_INFO,"updated gcr (disable): %x\n", gcr);
}

/**
 * \brief Gets the polynomial used for hash computation in RTU at HW.
 * @return hash_poly hex representation of polynomial
 */
uint16_t rtu_read_hash_poly(void)
{
	uint32_t gcr = rtu_rd( GCR);
  return RTU_GCR_POLY_VAL_R(gcr);
}

/**
 * \brief Sets the polynomial used for hash computation in RTU at HW.
 * @param hash_poly hex representation of polynomial
 */
void rtu_write_hash_poly(uint16_t hash_poly)
{
    // Get current GCR
	uint32_t gcr = rtu_rd( GCR);
    // Clear previous hash poly and insert the new one
    gcr = (gcr & (~RTU_GCR_POLY_VAL_MASK)) | RTU_GCR_POLY_VAL_W(hash_poly);
    // Update GCR
	rtu_wr(GCR, gcr );
    TRACE_DBG(TRACE_INFO,"updated gcr (poly): %x\n", gcr);
}


// PORT SETTINGS

/**
 * \brief Sets fixed priority of value 'prio' on indicated port.
 * It overrides the priority coming form the endpoint.
 * @param port port number (0 to 9)
 * @param prio priority value
 * @return error code.
 */
int rtu_set_fixed_prio_on_port(int port, uint8_t prio)
{
    if( (port < MIN_PORT) || (port > MAX_PORT) )
        return -EINVAL;

	uint32_t pcr = read_pcr(port);
	write_pcr(port, pcr | RTU_PCR_FIX_PRIO | RTU_PCR_PRIO_VAL_W(prio));
	return 0;
}

/**
 * \brief Unsets fixed priority on indicated port.
 * Orders to use priority from the endpoint instead.
 * @param port port number (0 to 9)
 * @return error code.
 */
int rtu_unset_fixed_prio_on_port(int port)
{
    if( (port < MIN_PORT) || (port > MAX_PORT) )
        return -EINVAL;

	uint32_t pcr = read_pcr(port);
	write_pcr(port, pcr & (RTU_PCR_LEARN_EN | RTU_PCR_PASS_ALL | RTU_PCR_PASS_BPDU | RTU_PCR_B_UNREC));

	return 0;
}

/**
 * \brief Sets the LEARN_EN flag on indicated port.
 * @param port port number (0 to 9)
 * @param flag 0 disables learning. Otherwise: enables learning porcess on this port.
 * @return error code.
 */
int rtu_learn_enable_on_port(int port, int flag)
{
    if( (port < MIN_PORT) || (port > MAX_PORT) )
        return -EINVAL;

	uint32_t pcr = read_pcr(port);
  pcr = flag ?
        RTU_PCR_LEARN_EN    | pcr :
        (~RTU_PCR_LEARN_EN) & pcr ;

	write_pcr(port, pcr);
  return 0;
}

/**
 * \brief Sets the PASS_BPDU flag on indicated port.
 * @param port port number (0 to 9)
 * @param flag 0: BPDU packets are passed RTU rules only if PASS_ALL is set.
 * Otherwise: BPDU packets are passed according to RTU rules.
 * @return error code.
 */
int rtu_pass_bpdu_on_port(int port, int flag)
{
    if( (port < MIN_PORT) || (port > MAX_PORT) )
        return -EINVAL;

	uint32_t pcr = read_pcr(port);
  pcr = flag ?
        RTU_PCR_PASS_BPDU    | pcr :
       (~RTU_PCR_PASS_BPDU) & pcr ;
	write_pcr(port, pcr);
  return 0;
}

/**
 * \brief Sets the PASS_ALL flag on indicated port.
 * @param port port number (0 to 9)
 * @param flag 0: all packets are dropped. Otherwise: all packets are passed.
 * @return error code.
 */
int rtu_pass_all_on_port(int port, int flag)
{
  if( (port < MIN_PORT) || (port > MAX_PORT) )
      return -EINVAL;

	uint32_t pcr = read_pcr(port);

  pcr = flag ?
        RTU_PCR_PASS_ALL    | pcr :
        (~RTU_PCR_PASS_ALL) & pcr ;
	write_pcr(port, pcr);
  return 0;
}

/**
 * \brief Sets the B_UNREC flag on indicated port.
 * @param port port number (0 to 9)
 * @param flag 0: packet is dropped. Otherwise: packet is broadcast.
 * @return error code.
 */
int rtu_set_unrecognised_behaviour_on_port(int port, int flag)
{
    if( (port < MIN_PORT) || (port > MAX_PORT) )
        return -EINVAL;

	uint32_t pcr = read_pcr(port);

  pcr = flag ?
        RTU_PCR_B_UNREC    | pcr :
        (~RTU_PCR_B_UNREC) & pcr ;

	write_pcr(port, pcr);
  return 0;
}
// --------------------------------------------
// RTU eXtension
// --------------------------------------------

int rtu_ext_simple_test() 
{
  uint32_t val;
  
  
  TRACE(TRACE_INFO,"RTU eXtension simple test (19 Nov 2012, 10am)\n");
  val = rtu_rd(GCR);
  
  if(RTU_GCR_RTU_VERSION_R(val) == 2)
  {
    TRACE(TRACE_INFO,"RTUeX: G/W version : %d (correct G/W !!! )\n",RTU_GCR_RTU_VERSION_R(val));
  }
  else
  {
    TRACE(TRACE_INFO,"RTUeX: G/W version : %d (in correct G/W !!! )\n",RTU_GCR_RTU_VERSION_R(val));
    TRACE(TRACE_INFO,"RTUeX: TEST FAILED ");
//     return -1;
  }
      
  val = 8;
  rtu_wr(RX_CTR, RTU_RX_CTR_PRIO_MASK_W(val));
  TRACE(TRACE_INFO,"writing to the RX_CTR register value = %d\n",val);
  val = rtu_rd(RX_CTR);
  TRACE(TRACE_INFO,"reading from the RX_CTR register value = %d\n",val);
  
  
  
//   TRACE(TRACE_INFO,"reading from the RTU_RX_FF_MAC_R1 max number values:\n");
//   val = rtu_rd(RX_FF_MAC_R1);
//   TRACE(TRACE_INFO,"\trange max number:%d (should be 1)\n",RTU_RX_FF_MAC_R1_HI_ID_R(val));
//   TRACE(TRACE_INFO,"\trange max number:%d (should be 4)\n",RTU_RX_FF_MAC_R1_ID_R(val));
/*  
  TRACE(TRACE_INFO,"mirror traffic from port 2 into port 3\n");
  val = 1 << 2;
  rtu_wr(RX_MP_R0, RTU_RX_MP_R0_SRC_MASK_W(val));  
  val = 1 << 3;
  rtu_wr(RX_MP_R1, RTU_RX_MP_R1_DST_MASK_W(val));  */

  return 0;
}



//---------------------------------------------
// Private Methods
//---------------------------------------------

// to write on MFIFO

static void write_mfifo_addr(uint32_t zbt_addr)
{
    rtu_wr(MFIFO_R0, RTU_MFIFO_R0_AD_SEL);
    rtu_wr(MFIFO_R1, zbt_addr);
}

static void write_mfifo_data(uint32_t word)
{
    rtu_wr(MFIFO_R0, RTU_MFIFO_R0_DATA_SEL);
    rtu_wr(MFIFO_R1, word);
}

// to marshall MAC entries

static uint32_t mac_entry_word0_w(struct filtering_entry *ent)
{
    return
        ((0xFF & ent->mac[0])                        << 24)  |
        ((0xFF & ent->mac[1])                        << 16)  |
        ((0xFF & ent->fid)                           <<  4)  |
        ((0x1  & ent->is_bpdu)                       <<  2)  |
        ((0x1  & ent->end_of_bucket)                 <<  1)  |
        ((0x1  & ent->valid )                             )  ;
}

static uint32_t mac_entry_word1_w(struct filtering_entry *ent)
{
    return
        ((0xFF & ent->mac[2])                        << 24)  |
        ((0xFF & ent->mac[3])                        << 16)  |
        ((0xFF & ent->mac[4])                        <<  8)  |
        ((0xFF & ent->mac[5])                             )  ;
}

static uint32_t mac_entry_word2_w(struct filtering_entry *ent)
{
    return
        ((0x1 & ent->drop_when_dest)                 << 28)  |
        ((0x1 & ent->prio_override_dst)              << 27)  |
        ((0x7 & ent->prio_dst)                       << 24)  |
        ((0x1 & ent->has_prio_dst)                   << 23)  |
        ((0x1 & ent->drop_unmatched_src_ports)       << 22)  |
        ((0x1 & ent->drop_when_source)               << 21)  |
        ((0x1 & ent->prio_override_src)              << 20)  |
		    ((0x7 & ent->prio_src)                       << 17)  |
        ((0x1 & ent->has_prio_src)                   << 16);
}

static uint32_t mac_entry_word3_w(struct filtering_entry *ent)
{
    return
        ((0xFFFF & ent->port_mask_dst)               << 16)  |
        ((0xFFFF & ent->port_mask_src)                    )  ;
}

static uint32_t mac_entry_word4_w(struct filtering_entry *ent)
{
    return
        ((0xFFFF & (ent->port_mask_dst >> 16))         << 16)  |
        ((0xFFFF & (ent->port_mask_src >> 16))              )  ;
}



void rtu_show_port_status(int port_id) 
{
  uint32_t pcr       = read_pcr(port_id);
  int      b_unrec   = (RTU_PCR_B_UNREC   & pcr) != 0 ? 1 : 0;
  int      fix_prio  = (RTU_PCR_FIX_PRIO  & pcr) != 0 ? 1 : 0;
  int      pass_bpdu = (RTU_PCR_PASS_BPDU & pcr) != 0 ? 1 : 0;
  int      pass_all  = (RTU_PCR_PASS_ALL  & pcr) != 0 ? 1 : 0;
  int      learn_en  = (RTU_PCR_LEARN_EN  & pcr) != 0 ? 1 : 0;
  int      prio      = RTU_PCR_PRIO_VAL_R(pcr);
  
  TRACE(TRACE_INFO, "Port %2d: b_unrec=%d,prio=%d,fix_prio=%d,pass_bpdu=%d,pass_all=%d,learn_en=%d",  
                     port_id, b_unrec, prio, fix_prio, pass_bpdu, pass_all, learn_en); 
}

int rtu_port_setting(int port_id) 
{
  uint32_t pcr       = read_pcr(port_id);
  return ((RTU_PCR_PASS_ALL  & pcr) != 0 ? 1 : 0);
}

void rtu_show_status() 
{
   int i;
   uint32_t gcr = rtu_rd( GCR);
   uint32_t psr = rtu_rd(PSR);
   int num_ports= RTU_PSR_N_PORTS_R(psr);
   if(gcr & RTU_GCR_G_ENA)
   {
      TRACE(TRACE_INFO, "RTU enabled");
   }
   else
   {
      TRACE(TRACE_INFO, "RTU disabled");
   }
   TRACE(TRACE_INFO, "[G/W version =%d, port_numeber=%d]",RTU_GCR_RTU_VERSION_R(gcr),
                                                          num_ports);
   for(i=0;i<num_ports;i++)
     rtu_show_port_status(i);
}


void rtu_set_life(char *optarg)
{
  
  int i;
  uint32_t n1,n2, n3,n4;
  int opt         = strtol(optarg,   &optarg, 0);
  int sub_opt     = strtol(optarg+1, &optarg, 0);
  int sub_sub_opt = strtol(optarg+1, &optarg, 0);
  
  int err = shw_fpga_mmap_init();
  if(err)
  {
     TRACE(TRACE_INFO, "Problem with fpga mapping");
     exit(1);
  }
  
  TRACE(TRACE_INFO, "opt: %ld, sub_opt  %ld, sub_sub_opt %ld",opt, sub_opt,sub_sub_opt);
  
  switch(opt){
    case 1:
       if(sub_opt == 1)
	 rtu_enable();
       else
         rtu_disable();
    break;
    case  2:
       rtu_pass_all_on_port(sub_sub_opt,sub_opt);
       TRACE(TRACE_INFO, "PORT_%d config ena_flag=%d",sub_sub_opt,sub_opt);
    break;
    case  3:

      vlan_entry_vd( sub_opt,      //vid, 
                     sub_sub_opt,  //port_mask, 
                     sub_opt,      //fid, 
                     0,            //prio,
                     0,            //has_prio,
		      0,            //prio_override, 
                     0             //drop
                     );
      TRACE(TRACE_INFO, "VLAN_e: vid=%d, port_mask=%d, fid=%d, prio=0, drop=0",
                       sub_opt,sub_sub_opt, sub_opt);
    break;
    case  4:

    case  10:
       rtu_show_status();
    break;
    default:
       TRACE(TRACE_INFO, "RTU control options");
       TRACE(TRACE_INFO, "Usage: wrsw_rtu -0 [<option> <value>]");
       TRACE(TRACE_INFO, "-o 0           show this info");
       TRACE(TRACE_INFO, "-o 1 1/0       enable/disable RTU");
       TRACE(TRACE_INFO, "-o 2 1/0  num  enable/disable port num");
       TRACE(TRACE_INFO, "-o 3 vlan mask show status");
       TRACE(TRACE_INFO, "-o 10          show status");
  };
  exit(1);
  
}
