#ifndef _PSTATSTABLE_H
#define _PSTATSTABLE_H

struct inpcb {
    struct inpcb   *inp_next;   /* pointers to other pcb's */
    struct in_addr  inp_faddr;  /* foreign host table entry */
    u_short         inp_fport;  /* foreign port */
    struct in_addr  inp_laddr;  /* local host table entry */
    u_short         inp_lport;  /* local port */
    int             inp_state;
    int             uid;        /* owner of the connection */
};

extern void                init_pstatsTable(void);
extern Netsnmp_Node_Handler     pstatsTable_handler;
extern NetsnmpCacheLoad         pstatsTable_load;
extern NetsnmpCacheFree         pstatsTable_free;
extern Netsnmp_First_Data_Point pstatsTable_first_entry;
extern Netsnmp_Next_Data_Point  pstatsTable_next_entry;

#define TCPCONNSTATE	     1
#define TCPCONNLOCALADDRESS  2
#define TCPCONNLOCALPORT     3
#define TCPCONNREMOTEADDRESS 4
#define TCPCONNREMOTEPORT    5

#endif /* _PSTATSTABLE_H */
