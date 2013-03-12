#!/bin/bash

cp ./userspace/wrsw_rtud/wrsw_rtud /var/lib/tftpboot/rootfs/wr/bin/
cp ./kernel/wr_rtu/wr_rtu.ko /var/lib/tftpboot/rootfs/wr/lib/modules/
cp ./kernel/wr_nic/wr-nic.ko /var/lib/tftpboot/rootfs/wr/lib/modules/