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
 *              - HCAM
 *              - Aging RAM for Main Hashtable
 *              - Aging Register for HCAM
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

#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#include <hw/switch_hw.h>
#include <hw/wrsw_rtu_wb.h>
#include <hal_client.h>

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

static void mac_entry_word0_r(uint32_t word, struct filtering_entry *ent);
static void mac_entry_word1_r(uint32_t word, struct filtering_entry *ent);
static void mac_entry_word2_r(uint32_t word, struct filtering_entry *ent);
static void mac_entry_word3_r(uint32_t word, struct filtering_entry *ent);
static void mac_entry_word4_r(uint32_t word, struct filtering_entry *ent);

static uint32_t vlan_entry_word0_w(struct vlan_table_entry *ent);
static uint32_t fpga_rtu_pcr_addr(int port);

/*
 * Used to communicate to RTU UFIFO IRQ handler device at kernel space
 */
static int fd;

/**
 * \brief Initialize HW RTU memory map
 */
int rtu_init(void)
{
    int err;

    if(halexp_client_init() < 0)
        TRACE(
            TRACE_FATAL,
            "WRSW_HAL is not responding... Are you sure it's running on your switch?\n"
            );

    // Used to 'get' RTU IRQs from kernel
	fd = open(RTU_DEVNAME, O_RDWR);
	if (fd < 0)
        return errno;

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


// UFIFO

/**
 * \brief Returns the UFIFO empty flag.
 * @return Value of UFIFO empty flag.
 */
int rtu_ufifo_is_empty(void)
{
    uint32_t csr =  _fpga_readl(FPGA_BASE_RTU + RTU_REG_UFIFO_CSR);
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
    uint32_t csr = _fpga_readl(FPGA_BASE_RTU + RTU_REG_UFIFO_CSR);
    return RTU_UFIFO_CSR_USEDW_R(csr);
}

/**
 * \brief Reads unrecognised request from UFIFO.
 * Blocks on read if learning queue is empty.
 * @param req pointer to unrecognised request data. Memory handled by callee.
 * @return error code
 */
int rtu_read_learning_queue(struct rtu_request *req)
{
    int err;

    // If learning queue is empty, wait for UFIFO IRQ
    if (rtu_ufifo_is_empty()) {
        err = ioctl(fd, WR_RTU_IRQWAIT);
        if (err && (err != -EAGAIN))
            return err;
    }

    // read data from mapped IO memory
    uint32_t r0 = _fpga_readl(FPGA_BASE_RTU + RTU_REG_UFIFO_R0);
    uint32_t r1 = _fpga_readl(FPGA_BASE_RTU + RTU_REG_UFIFO_R1);
    uint32_t r2 = _fpga_readl(FPGA_BASE_RTU + RTU_REG_UFIFO_R2);
    uint32_t r3 = _fpga_readl(FPGA_BASE_RTU + RTU_REG_UFIFO_R3);
    uint32_t r4 = _fpga_readl(FPGA_BASE_RTU + RTU_REG_UFIFO_R4);

    // Once read: if learning queue becomes empty again, enable UFIFO IRQ
    if (rtu_ufifo_is_empty())
        ioctl(fd, WR_RTU_IRQENA);

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
    uint32_t csr = _fpga_readl(FPGA_BASE_RTU + RTU_REG_MFIFO_CSR);
    return RTU_MFIFO_CSR_USEDW_R(csr);
}

/**
 * \brief Checks whether the main hashtable CPU access FIFO (MFIFO) is full.
 * @return 1 if MFIFO is full. 0 otherwise.
 */
int rtu_mfifo_is_full(void)
{
    uint32_t csr = _fpga_readl(FPGA_BASE_RTU + RTU_REG_MFIFO_CSR);
    return RTU_MFIFO_CSR_FULL & csr;
}

/**
 * \brief Checks whether the main hashtable CPU access FIFO (MFIFO) is empty.
 * @return 1 if MFIFO is empty. 0 otherwise.
 */
int rtu_mfifo_is_empty(void)
{
    uint32_t csr = _fpga_readl(FPGA_BASE_RTU + RTU_REG_MFIFO_CSR);
    return RTU_MFIFO_CSR_EMPTY & csr;
}

/**
 * \brief Cleans MFIFO
 */
void rtu_clean_mfifo(void)
{
    while(!rtu_mfifo_is_empty()) {
        _fpga_writel(FPGA_BASE_RTU + RTU_REG_MFIFO_R0, RTU_MFIFO_R0_AD_SEL);
        _fpga_writel(FPGA_BASE_RTU + RTU_REG_MFIFO_R1, 0);
        usleep(10);
    }
}

/**
 * \brief Writes one MAC entry into main hash table at the given address.
 * @param ent MAC table entry to be written to MFIFO.
 * @param zbt_addr ZBT SRAM memory address in which MAC entry shoud be added.
 */
void rtu_write_htab_entry(uint16_t zbt_addr, struct filtering_entry *ent)
{
    write_mfifo_addr(zbt_addr);
    write_mfifo_data(mac_entry_word0_w(ent));
    write_mfifo_data(mac_entry_word1_w(ent));
    write_mfifo_data(mac_entry_word2_w(ent));
    write_mfifo_data(mac_entry_word3_w(ent));
    write_mfifo_data(mac_entry_word4_w(ent));

    TRACE_DBG(
        TRACE_INFO,
        "write htab entry: addr %x ent %08x %08x %08x %08x %08x",
        zbt_addr,
        mac_entry_word0_w(ent),
        mac_entry_word1_w(ent),
        mac_entry_word2_w(ent),
        mac_entry_word3_w(ent),
        mac_entry_word4_w(ent)
    );
}

/**
 * \brief Cleans MAC entry in main hash table at the given address
 * @param zbt_addr memory address which shoud be cleaned.
 */
void rtu_clean_htab_entry(uint16_t zbt_addr)
{
	struct filtering_entry ent;
	rtu_write_htab_entry(zbt_addr, rtu_fe_clean(&ent));
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
    }
}


// HCAM

/**
 * \brief Reads MAC entry from HCAM Hash collisions memory
 * @param ent used to store the entry read. Memory should be handled by callee.
 * @param cam_addr memory address which shoud be read.
 */
void rtu_read_hcam_entry( uint16_t cam_addr, struct filtering_entry *ent )
{
    // read data from mapped IO memory
    uint32_t w0 = _fpga_readl(FPGA_BASE_RTU + RTU_HCAM + 4*cam_addr        );
    uint32_t w1 = _fpga_readl(FPGA_BASE_RTU + RTU_HCAM + 4*(cam_addr + 0x1));
    uint32_t w2 = _fpga_readl(FPGA_BASE_RTU + RTU_HCAM + 4*(cam_addr + 0x2));
    uint32_t w3 = _fpga_readl(FPGA_BASE_RTU + RTU_HCAM + 4*(cam_addr + 0x3));
    uint32_t w4 = _fpga_readl(FPGA_BASE_RTU + RTU_HCAM + 4*(cam_addr + 0x4));
    TRACE_DBG(
        TRACE_INFO,
        "read hcam entry: addr %x ent %08x %08x %08x %08x %08x",
        cam_addr,
        w0,
        w1,
        w2,
        w3,
        w4
    );
    // unmarshall data and populate entry
    mac_entry_word0_r( w0, ent );
    mac_entry_word1_r( w1, ent );
    mac_entry_word2_r( w2, ent );
    mac_entry_word3_r( w3, ent );
    mac_entry_word4_r( w4, ent );
}

/**
 * \brief Writes MAC entry to HCAM Hash collisions memory
 * @param ent MAC table entry to be written to HCAM.
 * @param cam_addr memory address in which MAC entry shoud be added.
 */
void rtu_write_hcam_entry( uint16_t cam_addr, struct filtering_entry *ent)
{
	_fpga_writel(FPGA_BASE_RTU + RTU_HCAM + 4*cam_addr        , mac_entry_word0_w(ent));
	_fpga_writel(FPGA_BASE_RTU + RTU_HCAM + 4*(cam_addr + 0x1), mac_entry_word1_w(ent));
	_fpga_writel(FPGA_BASE_RTU + RTU_HCAM + 4*(cam_addr + 0x2), mac_entry_word2_w(ent));
	_fpga_writel(FPGA_BASE_RTU + RTU_HCAM + 4*(cam_addr + 0x3), mac_entry_word3_w(ent));
	_fpga_writel(FPGA_BASE_RTU + RTU_HCAM + 4*(cam_addr + 0x4), mac_entry_word4_w(ent));
    TRACE_DBG(
        TRACE_INFO,
        "write hcam entry: addr %x ent %08x %08x %08x %08x %08x",
        cam_addr,
        mac_entry_word0_w(ent),
        mac_entry_word1_w(ent),
        mac_entry_word2_w(ent),
        mac_entry_word3_w(ent),
        mac_entry_word4_w(ent)
    );
}

/**
 * \brief Cleans MAC entry in HCAM Hash collisions memory
 * @param addr memory address which shoud be cleaned.
 */
void rtu_clean_hcam_entry( uint8_t cam_addr )
{
 	_fpga_writel(FPGA_BASE_RTU + RTU_HCAM + 4*cam_addr        , 0x00000000);
	_fpga_writel(FPGA_BASE_RTU + RTU_HCAM + 4*(cam_addr + 0x1), 0x00000000);
	_fpga_writel(FPGA_BASE_RTU + RTU_HCAM + 4*(cam_addr + 0x2), 0x00000000);
	_fpga_writel(FPGA_BASE_RTU + RTU_HCAM + 4*(cam_addr + 0x3), 0x00000000);
	_fpga_writel(FPGA_BASE_RTU + RTU_HCAM + 4*(cam_addr + 0x4), 0x00000000);
    TRACE_DBG(
        TRACE_INFO,
        "write hcam entry: addr %x ent 00000000 00000000 00000000 00000000 00000000", cam_addr);
}

/**
 * \brief Cleans HCAM.
 * Cleans all entries in HCAM inactive bank.
 */
void rtu_clean_hcam(void)
{
    int addr;
   	for (addr = 0; addr < (RTU_HCAM_WORDS/RTU_BANKS); addr++) {
        _fpga_writel(FPGA_BASE_RTU + RTU_HCAM + 4*addr, 0x00000000);
    }
}

// AGING RAM - HTAB

/**
 * \brief Read word from aging HTAB.
 * Aging RAM Size: 256 32-bit words
 */
uint32_t rtu_read_agr_htab( uint32_t addr )
{
    return _fpga_readl(FPGA_BASE_RTU + RTU_ARAM_MAIN + 4*addr) ;
}

/**
 * \brief Clears aging bitmap for HTAB
 */
void rtu_clean_agr_htab(void)
{
    int addr;
	for(addr=0;addr < RTU_ARAM_MAIN_WORDS;addr++) {
	    _fpga_writel(FPGA_BASE_RTU + RTU_ARAM_MAIN + 4*addr, 0x00000000);
    }
}


// AGING RAM - HCAM

/**
 * \brief Read aging register for HCAM.
 * Each bit corresponds to one MAC entry in HCAM memory.
 */
uint32_t rtu_read_agr_hcam(void)
{
	return _fpga_readl(FPGA_BASE_RTU + RTU_REG_AGR_HCAM);
}

/**
 * \brief Clears aging register for HCAM
 */
void rtu_clean_agr_hcam(void)
{
	_fpga_writel(FPGA_BASE_RTU + RTU_REG_AGR_HCAM, 0x00000000);
}


// VLAN TABLE

/**
 * \brief Writes entry to vlan table.
 * VLAN table size: 4096 32-bit words.
 * @param addr entry memory address
 */
void rtu_write_vlan_entry(uint32_t addr, struct vlan_table_entry *ent)
{

//	printf("write_VLAN_ent: addr %x val %x\n", + RTU_VLAN_TAB + 4*addr, vlan_entry_word0_w(ent));
	_fpga_writel(FPGA_BASE_RTU + RTU_VLAN_TAB + 4*addr, vlan_entry_word0_w(ent));
    TRACE_DBG(
        TRACE_INFO,
        "write vlan entry: addr %x ent %08x %08x %08x %08x %08x",
        addr,
        vlan_entry_word0_w(ent)
    );
}

/**
 * \brief Cleans VLAN entry in VLAN table
 * @param addr memory address which shoud be cleaned.
 */
void rtu_clean_vlan_entry( uint32_t addr )
{
    // Value 0x80000000 sets drop field to 1 (VLAN entry not registered)
 	_fpga_writel(FPGA_BASE_RTU + RTU_VLAN_TAB + 4*addr, 0x80000000);
}

/**
 * \brief Cleans VLAN table (drop bit is set to 1)
 */
void rtu_clean_vlan(void)
{
    int addr;
   	for (addr = 0; addr < NUM_VLANS; addr++) {
 	    _fpga_writel(FPGA_BASE_RTU + RTU_VLAN_TAB + 4*addr, 0x80000000);
    }
}


// RTU Global Control Register

/**
 * \brief Enables RTU operation.
 */
void rtu_enable(void)
{
    // Get current GCR
	uint32_t gcr = _fpga_readl(FPGA_BASE_RTU + RTU_REG_GCR);
    // Set G_ENA bit value = 1
    gcr = gcr | RTU_GCR_G_ENA;
    // Update GCR
	_fpga_writel(FPGA_BASE_RTU + RTU_REG_GCR, gcr );
    TRACE_DBG(TRACE_INFO,"updated gcr (enable): %x\n", gcr);
}

/**
 * \brief Disables RTU operation.
 */
void rtu_disable(void)
{
    // Get current GCR
	uint32_t gcr = _fpga_readl(FPGA_BASE_RTU + RTU_REG_GCR);
    // Set G_ENA bit value = 0
    gcr = gcr & (~RTU_GCR_G_ENA);
    // Update GCR
	_fpga_writel(FPGA_BASE_RTU + RTU_REG_GCR, gcr );
    TRACE_DBG(TRACE_INFO,"updated gcr (disable): %x\n", gcr);
}

/**
 * \brief Gets the polynomial used for hash computation in RTU at HW.
 * @return hash_poly hex representation of polynomial
 */
uint16_t rtu_read_hash_poly(void)
{
    // Get current GCR
	uint32_t gcr = _fpga_readl(FPGA_BASE_RTU + RTU_REG_GCR);
    return RTU_GCR_POLY_VAL_R(gcr);
}

/**
 * \brief Sets the polynomial used for hash computation in RTU at HW.
 * @param hash_poly hex representation of polynomial
 */
void rtu_write_hash_poly(uint16_t hash_poly)
{
    // Get current GCR
	uint32_t gcr = _fpga_readl(FPGA_BASE_RTU + RTU_REG_GCR);
    // Clear previous hash poly and insert the new one
    gcr = (gcr & (~RTU_GCR_POLY_VAL_MASK)) | RTU_GCR_POLY_VAL_W(hash_poly);
    // Update GCR
	_fpga_writel(FPGA_BASE_RTU + RTU_REG_GCR, gcr );
    TRACE_DBG(TRACE_INFO,"updated gcr (poly): %x\n", gcr);
}

/**
 * \brief Set active ZBT bank.
 * @param bank active ZBT bank (0 or 1). Other values will be evaluated as 1.
 */
void rtu_set_active_htab_bank(uint8_t bank)
{
	uint32_t gcr = _fpga_readl(FPGA_BASE_RTU + RTU_REG_GCR);
    gcr = bank ?
            (  RTU_GCR_HT_BSEL  | gcr ) :
            ((~RTU_GCR_HT_BSEL) & gcr ) ;
	_fpga_writel(FPGA_BASE_RTU + RTU_REG_GCR, gcr);
    TRACE_DBG(TRACE_INFO,"updated gcr (htab bank): %x\n", gcr);
}

/**
 * \brief Set active CAM bank.
 * @param bank active CAM bank (0 or 1). Other values will be evaluated as 1.
 */
void rtu_set_active_hcam_bank(uint8_t bank)
{
	uint32_t gcr = _fpga_readl(FPGA_BASE_RTU + RTU_REG_GCR);
    gcr = bank ?
            (  RTU_GCR_HCAM_BSEL  | gcr ) :
            ((~RTU_GCR_HCAM_BSEL) & gcr ) ;
	_fpga_writel(FPGA_BASE_RTU + RTU_REG_GCR, gcr );
    TRACE_DBG(TRACE_INFO,"updated gcr (hcam bank): %x\n", gcr);
}

/**
 * \brief Set active ZBT and CAM banks at once.
 * @param bank active ZBT and CAM bank (0 or 1). Other values will be evaluated as 1.
 */
void rtu_set_active_bank(uint8_t bank)
{
	uint32_t gcr = _fpga_readl(FPGA_BASE_RTU + RTU_REG_GCR);
    gcr = bank ?
            (  RTU_GCR_HT_BSEL | RTU_GCR_HCAM_BSEL | gcr ) :
            ((~RTU_GCR_HT_BSEL) & (~RTU_GCR_HCAM_BSEL) & gcr ) ;
	_fpga_writel(FPGA_BASE_RTU + RTU_REG_GCR, gcr );
    TRACE_DBG(TRACE_INFO,"updated gcr (htab/hcam bank): %x\n", gcr);
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
    uint32_t pcr_addr = fpga_rtu_pcr_addr(port);
	uint32_t pcr = _fpga_readl(FPGA_BASE_RTU + pcr_addr);
    // Be careful! the following assumes every port control reg has same layout
    pcr = pcr | RTU_PCR0_FIX_PRIO | RTU_PCR0_PRIO_VAL_W(prio);
	_fpga_writel(FPGA_BASE_RTU + pcr_addr, pcr);
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
    uint32_t pcr_addr = fpga_rtu_pcr_addr(port);
	uint32_t pcr = _fpga_readl(FPGA_BASE_RTU + pcr_addr);
    // Be careful! the following assumes every port control reg has same layout
    pcr = pcr & (RTU_PCR0_LEARN_EN | RTU_PCR0_PASS_ALL | RTU_PCR0_PASS_BPDU | RTU_PCR0_B_UNREC);
	_fpga_writel(FPGA_BASE_RTU + pcr_addr, pcr);
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
    uint32_t pcr_addr = fpga_rtu_pcr_addr(port);
	uint32_t pcr = _fpga_readl(FPGA_BASE_RTU + pcr_addr);
    // Be careful! the following assumes every port control reg has same layout
    pcr = flag ?
            RTU_PCR0_LEARN_EN    | pcr :
            (~RTU_PCR0_LEARN_EN) & pcr ;
	_fpga_writel(FPGA_BASE_RTU + pcr_addr, pcr);
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
    uint32_t pcr_addr = fpga_rtu_pcr_addr(port);
	uint32_t pcr = _fpga_readl(FPGA_BASE_RTU + pcr_addr);
    // Be careful! the following assumes every port control reg has same layout
    pcr = flag ?
            RTU_PCR0_PASS_BPDU    | pcr :
            (~RTU_PCR0_PASS_BPDU) & pcr ;
	_fpga_writel(FPGA_BASE_RTU + pcr_addr, pcr);
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
    uint32_t pcr_addr = fpga_rtu_pcr_addr(port);
	uint32_t pcr = _fpga_readl(FPGA_BASE_RTU + pcr_addr);
    // Be careful! the following assumes every port control reg has same layout
    pcr = flag ?
            RTU_PCR0_PASS_ALL    | pcr :
            (~RTU_PCR0_PASS_ALL) & pcr ;
	_fpga_writel(FPGA_BASE_RTU + pcr_addr, pcr);
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
    uint32_t pcr_addr = fpga_rtu_pcr_addr(port);
	uint32_t pcr = _fpga_readl(FPGA_BASE_RTU + pcr_addr);
    // Be careful! the following assumes every port control reg has same layout
    pcr = flag ?
            RTU_PCR0_B_UNREC    | pcr :
            (~RTU_PCR0_B_UNREC) & pcr ;
	_fpga_writel(FPGA_BASE_RTU + pcr_addr, pcr);
    return 0;
}

// IRQs

void rtu_enable_irq(void)
{
    _fpga_writel(FPGA_BASE_RTU + RTU_REG_EIC_IER, RTU_EIC_IER_NEMPTY);
}

void rtu_disable_irq(void)
{
    _fpga_writel(FPGA_BASE_RTU + RTU_REG_EIC_IDR, RTU_EIC_IDR_NEMPTY);
}

void rtu_clear_irq(void)
{
    _fpga_writel(FPGA_BASE_RTU + RTU_REG_EIC_ISR, RTU_EIC_ISR_NEMPTY);
}

//---------------------------------------------
// Private Methods
//---------------------------------------------

// to write on MFIFO

static void write_mfifo_addr(uint32_t zbt_addr)
{
    // workaround required to solve MFIFO overflow
    rtu_clean_mfifo();

    _fpga_writel(FPGA_BASE_RTU + RTU_REG_MFIFO_R0, RTU_MFIFO_R0_AD_SEL);
    _fpga_writel(FPGA_BASE_RTU + RTU_REG_MFIFO_R1, zbt_addr);
}

static void write_mfifo_data(uint32_t word)
{
    _fpga_writel(FPGA_BASE_RTU + RTU_REG_MFIFO_R0, RTU_MFIFO_R0_DATA_SEL);
    _fpga_writel(FPGA_BASE_RTU + RTU_REG_MFIFO_R1, word);
    // workaround required to solve MFIFO overflow
    while(rtu_mfifo_is_full());
}

// to marshall MAC entries

static uint32_t mac_entry_word0_w(struct filtering_entry *ent)
{
    return
        ((0xFF & ent->mac[0])                        << 24)  |
        ((0xFF & ent->mac[1])                        << 16)  |
        ((0xFF & ent->fid)                           <<  4)  |
        ((0x1  & ent->go_to_cam)                     <<  3)  |
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
        ((0x1 & ent->has_prio_src)                   << 16)  |
        ((0x01FF & ent->cam_addr)                          )  ;
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
        (ent->last_access_t);
}


// to unmarshall MAC entries

static void mac_entry_word0_r(uint32_t word, struct filtering_entry *ent)
{
    ent->mac[0]                         = 0xFF & (word >> 24);
    ent->mac[1]                         = 0xFF & (word >> 16);
    ent->fid                            = 0xFF & (word >>  4);
    ent->go_to_cam                      = 0x1  & (word >>  3);
    ent->is_bpdu                        = 0x1  & (word >>  2);
    ent->end_of_bucket                  = 0x1  & (word >>  1);
    ent->valid                          = 0x1  & (word      );
}

static void mac_entry_word1_r(uint32_t word, struct filtering_entry *ent)
{
    ent->mac[2]                         = 0xFF & (word >> 24);
    ent->mac[3]                         = 0xFF & (word >> 16);
    ent->mac[4]                         = 0xFF & (word >>  8);
    ent->mac[5]                         = 0xFF & (word      );
}

static void mac_entry_word2_r(uint32_t word, struct filtering_entry *ent)
{
    ent->drop_when_dest                 = 0x1   & (word >> 28);
    ent->prio_override_dst              = 0x1   & (word >> 27);
    ent->prio_dst                       = 0x7   & (word >> 24);
    ent->has_prio_dst                   = 0x1   & (word >> 23);
    ent->drop_unmatched_src_ports       = 0x1   & (word >> 22);
    ent->drop_when_source               = 0x1   & (word >> 21);
    ent->prio_override_src              = 0x1   & (word >> 20);
    ent->prio_src                       = 0x7   & (word >> 17);
    ent->has_prio_src                   = 0x1   & (word >> 16);
    ent->cam_addr                       = 0x1FF & (word      );
}

static void mac_entry_word3_r(uint32_t word, struct filtering_entry *ent)
{
    ent->port_mask_dst                  = 0xFFFF & (word >> 16);
    ent->port_mask_src                  = 0xFFFF & (word      );
}

static void mac_entry_word4_r(uint32_t word, struct filtering_entry *ent)
{
    ent->last_access_t                  = word;
}


// to marshall VLAN entries

static uint32_t vlan_entry_word0_w(struct vlan_table_entry *ent)
{
    return
        ((0x1    & ent->drop)                             << 31)  |
        ((0x1    & ent->prio_override)                    << 30)  |
        ((0x7    & ent->prio)                             << 27)  |
        ((0x1    & ent->has_prio)                         << 26)  |
        ((0xFF   & ent->fid)                              << 16)  |
        ((0xFFFF & ent->port_mask)                             )  ;
}

static uint32_t fpga_rtu_pcr_addr(int port)
{
    switch(port){
    case 0: return RTU_REG_PCR0;
    case 1: return RTU_REG_PCR1;
    case 2: return RTU_REG_PCR2;
    case 3: return RTU_REG_PCR3;
    case 4: return RTU_REG_PCR4;
    case 5: return RTU_REG_PCR5;
    case 6: return RTU_REG_PCR6;
    case 7: return RTU_REG_PCR7;
    case 8: return RTU_REG_PCR8;
    case 9: return RTU_REG_PCR9;
    default:return -EINVAL;
    }
}


