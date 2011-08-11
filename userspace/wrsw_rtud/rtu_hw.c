/*
 * White Rabbit RTU (Routing Table Unit)
 * Copyright (C) 2010, CERN.
 *
 * Version:     wrsw_rtud v1.0
 *
 * Authors:     Juan Luis Manas (juan.manas@integrasys.es)
 *              Miguel Baizan   (miguel.baizan@integrasys.es)
 *
 * Description: Handles HTAB/HCAM HW writes.
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
#include <stdlib.h>

#include "rtu_drv.h"
#include "rtu_hw.h"

// Used to declare memory type at filtering database entry handles.
#define HTAB            0
#define HCAM            1

// Used to declare HW request types.
#define HW_WRITE_REQ    0
#define HW_CLEAN_REQ    1

/**
 * \brief Filtering Database entry handle.
 */
struct fd_handle {
    int mem_type;                       // HTAB or HCAM
    uint16_t addr;                      // zbt_addr or cam_addr
    struct filtering_entry *entry_ptr;  // pointer to entry at mirror fd
};

/**
 * \brief HW (HTAB or HCAM) write request.
 */
struct hw_req {
    int type;                           // WRITE or CLEAN
    struct fd_handle handle;            // filtering database entry handle
    struct hw_req *next;                // linked list
};

/**
 * \brief HW write and clean requests list.
 * Used to temporarily store entry changes performed at SW.
 */
struct hw_req *hw_req_list;

/**
 * \brief Table bank to write entries to.
 * HTAB and HCAM banks will be handled according to this single bank value.
 */
static uint8_t bank;


/**
 * \brief Adds a new HW request at the end of the main hw request list.
 */
static int add_hw_req(int type, int mem, uint16_t addr, struct filtering_entry *ent)
{
    struct hw_req *req, *ptr = NULL;

    // get tail of current list
    if (hw_req_list) {
        for (ptr = hw_req_list; ptr->next; ptr = ptr->next) {
            if (ptr->handle.entry_ptr == ent)
                return 0; // avoid duplicated writes
        }
    }

    req = (struct hw_req*) malloc(sizeof(struct hw_req));
    if(!req)
        return -ENOMEM;

    req->type             = type;
    req->handle.mem_type  = mem;
    req->handle.addr      = addr;
    req->handle.entry_ptr = ent;
    req->next             = NULL;

    if(hw_req_list)
        ptr->next   = req;
    else
        hw_req_list = req;

    return 0;
}

/**
 * \brief Removes all write requests.
 */
static void clean_list(struct hw_req *head)
{
    struct hw_req *ptr;

    while(head) {
        ptr  = head;
        head = head->next;
        free(ptr);
    }
}

/**
 * \brief Read changes from hw_req_list and invoke RTU driver to efectively
 * write or clean the entry.
 */
static void commit(void)
{
    struct hw_req *req;     // used to scan hw_req_list

    for(req = hw_req_list; req; req = req->next){
        switch(req->type){
        case HW_WRITE_REQ:
            if(req->handle.mem_type == HTAB)
                rtu_write_htab_entry(req->handle.addr, req->handle.entry_ptr);
            else
                rtu_write_hcam_entry(req->handle.addr, req->handle.entry_ptr);
            break;
        case HW_CLEAN_REQ:
            if(req->handle.mem_type == HTAB)
                rtu_clean_htab_entry(req->handle.addr);
            else
                rtu_clean_hcam_entry(req->handle.addr);
            break;
        }
    }
}

/**
 * \brief Set the filtering database active bank both in software and hardware.
 * Note  both HTAB and HCAM active banks are switched at once.
 * Bank switching is delayed until MFIFO is empty (method remains blocked
 * meanwhile).
 */
static void set_active_bank(int b)
{
    // wait until MFIFO is empty
    rtu_clean_mfifo();
    // inactive bank becomes active (both banks are switched at once)
	rtu_set_active_bank(b);
    // active bank becomes inactive one
    bank = (b == 0) ? 1:0;
}

int rtu_hw_write_htab_entry(uint16_t addr, struct filtering_entry *e)
{
    return add_hw_req(HW_WRITE_REQ, HTAB, addr, e);
}

int rtu_hw_write_hcam_entry(uint16_t addr, struct filtering_entry *e)
{
    return add_hw_req(HW_WRITE_REQ, HCAM, addr, e);
}

int rtu_hw_clean_htab_entry(uint16_t addr)
{
    return add_hw_req(HW_CLEAN_REQ, HTAB, addr, NULL);
}

int rtu_hw_clean_hcam_entry(uint16_t addr)
{
    return add_hw_req(HW_CLEAN_REQ, HCAM, addr, NULL);
}

void rtu_hw_clean_fdb(void){
    set_active_bank(0);
    rtu_clean_htab();
    rtu_clean_hcam();
    set_active_bank(1);
    rtu_clean_htab();
    rtu_clean_hcam();
}

/**
 * \brief Commits entry changes at software to hardware HTAB and HCAM.
 */
void rtu_hw_commit(void)
{
    if(!hw_req_list)
        return;

    // write entries to inactive bank
    commit();
    // switch bank to make entries available to RTU at HW
    set_active_bank(bank);
    // both banks need same content
    commit();

    // the list is no longer needed
    clean_list(hw_req_list);
    hw_req_list = NULL;
}

/**
 * \brief Orders to delete pending entry changes at software.
 */
void rtu_hw_rollback(void)
{
    clean_list(hw_req_list);
    hw_req_list = NULL;
}

