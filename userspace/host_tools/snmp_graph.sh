#!/bin/bash

function print_help {
    echo "Script to retrieve data via SNMP and plot it using $PLOT_PROG"
    echo "Usage:"
    echo "$0 -f <file> --ip <ip_addr> [-c] [-h] [--help] [--no-plot] [-i <interval>] [--mibs-load <MIB1:MIB2:..>] [--mibs-path <PATH1:PATH2>]"
    echo "Where:"
    echo "       -f <file>                  load OIDs from the given file (one OID per"
    echo "                                  line or IP|OID per line)"
    echo "       --ip <ip_addr>             retrieve data from the host with the provided"
    echo "                                  IP address; this IP will be used if IP is not"
    echo "                                  specified in the file with OIDs"
    echo "       -c                         don't remove previous data"
    echo "       -h | --help                print this help"
    echo "       --no-plot                  don't plot data, only save it to the file"
    echo "       -i <interval>              interval of data retrieves"
    echo "       --mibs-load <MIB1:MIB2:..> load the given MIBs (colon separated list)"
    echo "       --mibs-path <PATH1:PATH2>  load MIBs from the given paths (colon"
    echo "                                  separated list)"
    echo ""
    echo "Example:"
    echo "$0 -f oids_temp.txt --ip 192.168.1.12"
    echo "$0 -f oids_temp.txt --ip 192.168.1.12 --mibs-path \"/var/lib/mibs/ietf:../snmpd\" --mibs-load WR-SWITCH-MIB"
}

PLOT_PROG=gnuplot
SNMP_PROG=snmpget
GNUPLOT_SCRIPT=looper

while [ ! -z "$1" ]; do
    if [ "$1" == "-h" ] || [ "$1" == "--help" ] ; then
	print_help
	exit 1
    fi
    # keep previous data
    if [ "$1" == "-c" ] ; then
	keep_data=1
    fi
    # file with oids
    if [ "$1" == "-f" ] ; then
	OID_FILE="$2"
	shift
    fi
    # IP address of a target
    if [ "$1" == "--ip" ] ; then
	HOST_IP="$2"
	shift
    fi
    #  interval of data read/get
    if [ "$1" == "-i" ] ; then
	SNMP_INTEVAL="$2"
	shift
    fi
    # MIBs to be loaded
    if [ "$1" == "--mibs-load" ] ; then
	MIBS_LOAD="$2"
	shift
    fi
    # path to MIBs
    if [ "$1" == "--mibs-path" ] ; then
	MIBS_PATH="$2"
	shift
    fi
    # don't plot date
    if [ "$1" == "--no-plot" ] ; then
	no_plot=1
    fi

    shift
done

if [ -z "$OID_FILE" ]; then
    echo "No file with OIDs!"
    exit 1
fi

if [ -z "$HOST_IP" ]; then
    echo "No target IP address defined!"
    exit 1
fi

if [ -z "$SNMP_INTEVAL" ]; then
    # use default interval
    SNMP_INTEVAL=5
fi

if [ -z "$MIBS_LOAD" ]; then
    # use all mibs by default
    MIBS_LOAD="all"
fi

if [ ! -z "$MIBS_PATH" ]; then
    # use default paths to MIBs
    MIBS_PATH="-M $MIBS_PATH"
fi

DATA_FILE="${OID_FILE%.*}.data"
SNMP_CMD="$SNMP_PROG -c public -v 2c -m $MIBS_LOAD $MIBS_PATH -Ov -Ot -Oq"

command -v $SNMP_PROG &> /dev/null
if [ $? -ne 0 ]; then
    echo "Program $SNMP_PROG for getting the data via SNMP not found"
    exit 1
fi

if [ -z $keep_data ]; then
    echo "Remove previous data"
    rm -f "$DATA_FILE"
fi

oid_count=0
for oid in `cat $OID_FILE`; do
    echo -n "$oid " >> $DATA_FILE
    ((oid_count++))
done
echo "" >> "$DATA_FILE"

if [ -z $no_plot ]; then

    command -v $PLOT_PROG &> /dev/null
    if [ $? -ne 0 ]; then
	echo "Program $PLOT_PROG for plotting graphs not found"
	exit 1
    fi
    i=2
    # generate gnuplot's script
    echo -n "plot '$DATA_FILE' using 0:1" > "$GNUPLOT_SCRIPT"
    while ((i<=oid_count)); do
	echo -n ", '' using 0:$i" >> "$GNUPLOT_SCRIPT"
	((i++))
    done
    echo "" >> "$GNUPLOT_SCRIPT"
    echo "pause $SNMP_INTEVAL" >> "$GNUPLOT_SCRIPT"
    echo "reread" >> "$GNUPLOT_SCRIPT"

    { sleep 1; \
	gnuplot -e "set key above; \
	    set key autotitle columnheader; \
	    set style data lines; \
	    load 'looper'";\
    } &
fi

while true; do
    for oid in `cat $OID_FILE`; do
	IP="$HOST_IP"
	# check if file contains IP|OID or just OISs
	IFS='|' read -ra touple <<< "$oid"
	if [ ! -z  ${touple[1]} ]; then
		IP=${touple[0]}
		oid=${touple[1]}
	fi
	$SNMP_CMD "$IP" "$oid" | tr -d '\n' >> "$DATA_FILE"
	echo -n " " >> "$DATA_FILE"
    done
    echo "" >> "$DATA_FILE"
    sleep "$SNMP_INTEVAL"
done
