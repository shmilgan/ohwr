
#define PRIV_IOCGCALIBRATE (SIOCDEVPRIVATE + 1)
#define PRIV_IOCGGETPHASE (SIOCDEVPRIVATE + 2)
#define PRIV_IOCREADREG (SIOCDEVPRIVATE + 3)
#define PRIV_IOCPHYREG (SIOCDEVPRIVATE + 4)

#define NIC_READ_PHY_CMD(addr)  (((addr) & 0xff) << 16)
#define NIC_RESULT_DATA(val) ((val) & 0xffff)
#define NIC_WRITE_PHY_CMD(addr, value)  ((((addr) & 0xff) << 16) \
 | (1 << 31) \
 | ((value) & 0xffff))
