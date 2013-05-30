#!/bin/ash
export WR_HOME="/wr"

# Get parameter from kernel commandline
for arg in $(cat /proc/cmdline); do
	echo $arg | grep -q "wr_nic.macaddr" ;
	if [ $? == 0 ]; then
		val=$(echo $arg | cut -d= -f2);
	fi;
done

# Obtain the type of FPGA (LX130XT or LX240XT)
tfpga=$($WR_HOME/bin/shw_ver -F)

$WR_HOME/bin/load-virtex $WR_HOME/lib/firmware/18p_mb-${tfpga}.bin

###### v4-dev stuff ############################################################
#
# you might need to replace FPGA bitstream with your custom
# 
# $WR_HOME/bin/load-virtex $WR_HOME/lib/firmware/18p_mb-LX240T-your-name.bin
################################################################################

$WR_HOME/bin/load-lm32 $WR_HOME/lib/firmware/rt_cpu.bin
insmod $WR_HOME/lib/modules/at91_softpwm.ko
insmod $WR_HOME/lib/modules/wr_vic.ko
insmod $WR_HOME/lib/modules/wr-nic.ko macaddr=$val
insmod $WR_HOME/lib/modules/wr_rtu.ko

###### v4-dev stuff ############################################################
#
# uncomment below if you want to use pstats
# 
# insmod $WR_HOME/lib/modules/wr_pstats.ko
# 
################################################################################


$WR_HOME/sbin/start-daemons.sh

