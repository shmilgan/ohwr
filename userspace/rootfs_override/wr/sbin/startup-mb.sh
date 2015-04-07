#!/bin/ash
export WR_HOME="/wr"
LOAD_FPGA_STATUS_FILE="/tmp/load_fpga_status"

# Get parameter from kernel commandline
for arg in $(cat /proc/cmdline); do
	echo $arg | grep -q "wr_nic.macaddr" ;
	if [ $? == 0 ]; then
		val=$(echo $arg | cut -d= -f2);
	fi;
done

# Obtain the type of FPGA (LX130XT or LX240XT)
tfpga=$($WR_HOME/bin/wrs_version -F)
if [ "$tfpga" = "UNKNOWN" ]; then
    tfpga="LX240T"
fi
FP_FILE="$WR_HOME/lib/firmware/18p_mb-${tfpga}.bin"


# TODO: Update wrsw_version to read this value from DF.
scb_ver=33
if mtdinfo -a | grep -A 1 dataflash | grep 264 &> /dev/null; then
	scb_ver=34
fi
LM_FILE="$WR_HOME/lib/firmware/rt_cpu.elf"

if ! [ -f "$FP_FILE" ]; then
    echo "Fatal: can't find \"$FP_FILE\"" >& 2
    echo "load_file_not_found" > $LOAD_FPGA_STATUS_FILE
    exit 1;
fi
if ! [ -f "$LM_FILE" ]; then
    echo "Fatal: can't find \"$LM_FILE\"" >& 2
    exit 1;
fi

$WR_HOME/bin/load-virtex $FP_FILE
if [ $? -eq 0 ];
then
    echo "load_ok" > $LOAD_FPGA_STATUS_FILE
else
    echo "Fatal: load FPGA failed" >& 2
    echo "load_error" > $LOAD_FPGA_STATUS_FILE
fi

$WR_HOME/bin/load-lm32   $LM_FILE scb_ver=${scb_ver}
insmod $WR_HOME/lib/modules/at91_softpwm.ko
insmod $WR_HOME/lib/modules/wr_vic.ko
insmod $WR_HOME/lib/modules/wr-nic.ko macaddr=$val
insmod $WR_HOME/lib/modules/wr_rtu.ko
insmod $WR_HOME/lib/modules/wr_pstats.ko pstats_nports=18
insmod $WR_HOME/lib/modules/wr_clocksource.ko
$WR_HOME/sbin/start-daemons.sh

