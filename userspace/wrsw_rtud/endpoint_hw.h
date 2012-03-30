#ifndef ENDPOINT_HW_H
#define ENDPOINT_HW_H

/* Custom IOCTLs */
#define PRIV_IOCGGETRFCR (SIOCDEVPRIVATE+3)
#define PRIV_IOCSSETRFCR (SIOCDEVPRIVATE+4)
#define PRIV_IOCGGETECR (SIOCDEVPRIVATE+5)

/* RFCR */
#define WRN_RFCR_GET_VID_VAL 1
#define WRN_RFCR_GET_PRIO_VAL 2
#define WRN_RFCR_GET_QMODE 4
#define WRN_RFCR_SET_VID_VAL 9
#define WRN_RFCR_SET_PRIO_VAL 10
#define WRN_RFCR_SET_QMODE 12

/* ECR. For now only get operation on PORTID implemented */
#define WRN_ECR_GET_PORTID 4

/* For custom ioctl operations on endpoint registers */
struct wrn_register_req {
    int         cmd;
    uint32_t    val;
};

int ep_hw_init(void);

int ep_hw_get_qmode(int port_idx);
int ep_hw_set_qmode(int port_idx, int qmode);

int ep_hw_get_pvid(int port_idx);
int ep_hw_set_pvid(int port_idx, int pvid);

int ep_hw_get_port_id(int port_idx);

#endif /* ENDPOINT_HW_H */
