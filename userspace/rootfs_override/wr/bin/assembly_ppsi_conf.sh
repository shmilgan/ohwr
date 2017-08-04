#!/bin/bash

# Adam Wujek @ CERN
# script to assembly ppsi.conf based on dot-config configuration

PRE_FILE="/wr/etc/ppsi-pre.conf"
OUTPUT_FILE="/etc/ppsi.conf"
DOTCONFIG_FILE="/wr/etc/dot-config"

if [ -f "$DOTCONFIG_FILE" ]; then
    . "$DOTCONFIG_FILE"
else
    # if dot-config not available remove ppsi's config
    rm $OUTPUT_FILE
    exit 1
fi

echo "# Autogenerated file, please don't edit." > $OUTPUT_FILE
echo "# This file will be overwritten at next boot." >> $OUTPUT_FILE

#copy top of ppsi.conf
cat $PRE_FILE >> $OUTPUT_FILE


if [ -n "$CONFIG_PTP_OPT_CLOCK_CLASS" ]; then
	echo clock-class "$CONFIG_PTP_OPT_CLOCK_CLASS" >> $OUTPUT_FILE
else
	if [ "$CONFIG_TIME_GM" = "y" ]; then
		echo clock-class 6 >> $OUTPUT_FILE
	fi
	if [ "$CONFIG_TIME_FM" = "y" ]; then
		echo clock-class 52 >> $OUTPUT_FILE
	fi
	if [ "$CONFIG_TIME_BC" = "y" ]; then
		echo clock-class 248 >> $OUTPUT_FILE
	fi
fi

if [ -n "$CONFIG_PTP_OPT_CLOCK_ACCURACY" ]; then
	echo clock-accuracy "$CONFIG_PTP_OPT_CLOCK_ACCURACY" >> $OUTPUT_FILE
fi
if [ -n "$CONFIG_PTP_OPT_CLOCK_ALLAN_VARIANCE" ]; then
	echo clock-allan-variance "$CONFIG_PTP_OPT_CLOCK_ALLAN_VARIANCE" >> $OUTPUT_FILE
fi

if [ -n "$CONFIG_PTP_OPT_DOMAIN_NUMBER" ]; then
	echo domain-number "$CONFIG_PTP_OPT_DOMAIN_NUMBER" >> $OUTPUT_FILE
fi

if [ -n "$CONFIG_PTP_OPT_ANNOUNCE_INTERVAL" ]; then
	echo announce-interval "$CONFIG_PTP_OPT_ANNOUNCE_INTERVAL" >> $OUTPUT_FILE
fi

if [ -n "$CONFIG_PTP_OPT_SYNC_INTERVAL" ]; then
	echo sync-interval "$CONFIG_PTP_OPT_SYNC_INTERVAL" >> $OUTPUT_FILE
fi

if [ -n "$CONFIG_PTP_OPT_PRIORITY1" ]; then
	echo priority1 "$CONFIG_PTP_OPT_PRIORITY1" >> $OUTPUT_FILE
fi

if [ -n "$CONFIG_PTP_OPT_PRIORITY2" ]; then
	echo priority2 "$CONFIG_PTP_OPT_PRIORITY2" >> $OUTPUT_FILE
fi

# 2 new lines
echo -n -e "\n\n"  >> $OUTPUT_FILE

for i_zero in {01..18};do
	# unset parametes
	unset p_name
	unset p_proto
	unset p_role
	unset p_ext
	# delay mechanism
	unset p_dm
	unset p_monitor
	# parse parameters
	param_line=$(eval "echo \$CONFIG_PORT"$i_zero"_PARAMS")
	IFS_OLD=$IFS
	IFS=','
	# save pairs into array
	pair_array=($param_line)
	IFS=$IFS_OLD
	for pair in ${pair_array[@]}
	do
		# split pairs
		IFS='=' read param value <<< "$pair"
		case "$param" in
		"name")
			p_name="$value";;
		"proto")
			p_proto="$value";;
		"role")
			p_role="$value";;
		"ext")
			p_ext="$value";;
		"dm")
			p_dm="$value";;
		"monitor")
			# read by SNMP directly from the config
			continue;;
		"rx"|"tx"|"fiber")
			continue;;
		*)
			echo "$0: Invalid parameter $param in CONFIG_PORT"$i_zero"_PARAMS" ;;
		esac

	done

	# if role "none" skip port configuration
	if [ "$p_role" == "none" ]; then
		continue
	fi

	#remove leading zero from i_zero (params has numbers with leading zero,
	#interface names are without leading zero)
	i=$(expr $i_zero + 0)

	if [ -n "$p_proto" ]; then
		echo "port wri$i-$p_proto" >> $OUTPUT_FILE
		echo "proto $p_proto" >> $OUTPUT_FILE
	else
		echo "port wri$i-raw" >> $OUTPUT_FILE
	fi

	echo "iface wri$i" >> $OUTPUT_FILE

	if [ -n "$p_role" ]; then
		echo "role $p_role" >> $OUTPUT_FILE
	else
		echo "$0: Role not defined in CONFIG_PORT"$i_zero"_PARAMS"
	fi

	if [ "${p_ext,,}" = "wr" ]; then # lower case
		echo "extension whiterabbit" >> $OUTPUT_FILE
	# add HA one day...
	# elif [ "${p_ext,,}" = "ha" ]; then # lower case
	# 	echo "extension whiterabbit" >> $OUTPUT_FILE
	elif [ "${p_ext,,}" = "none" ]; then # lower case
		# do nothing
		echo "# no extension" >> $OUTPUT_FILE
	elif [ -n "$p_ext" ]; then
		echo "$0: Invalid parameter ext=\"$p_ext\" in CONFIG_PORT"$i_zero"_PARAMS"
	else
		# default
		echo "extension whiterabbit" >> $OUTPUT_FILE
	fi

	if [ "${p_dm,,}" = "p2p" ]; then # lower case
		echo "mechanism p2p" >> $OUTPUT_FILE
	elif [ "${p_dm,,}" = "e2e" ]; then # lower case
		# do nothing
		true
	elif [ -n "$p_dm" ]; then
		echo "$0: Invalid parameter dm=\"$p_dm\" in CONFIG_PORT"$i_zero"_PARAMS"
	fi

	# add vlans
	if [ "$CONFIG_VLANS_ENABLE" = "y" ]; then
		unset ppsi_vlans;
		unset port_mode_access;
		unset port_mode_trunk;
		unset port_mode_unqualified;
		unset port_mode_disabled;

		# check port mode
		port_mode_access=$(eval "echo \$CONFIG_VLANS_PORT"$i_zero"_MODE_ACCESS")
		port_mode_trunk=$(eval "echo \$CONFIG_VLANS_PORT"$i_zero"_MODE_TRUNK")
		port_mode_unqualified=$(eval "echo \$CONFIG_VLANS_PORT"$i_zero"_MODE_UNQUALIFIED")
		port_mode_disabled=$(eval "echo \$CONFIG_VLANS_PORT"$i_zero"_MODE_DISABLED")

		# check port mode
		if [ "$port_mode_access" = "y" ]; then
			ppsi_vlans=$(eval "echo \$CONFIG_VLANS_PORT"$i_zero"_VID")
			# use "&> /dev/null" to avoid error when $ppsi_vlans
			# is not a number
			if [ "$ppsi_vlans" -ge 0 ]  &> /dev/null \
			    && [ "$ppsi_vlans" -le 4094 ] &> /dev/null; then
				echo "vlan $ppsi_vlans" >> $OUTPUT_FILE
			else
				echo "$0: Wrong value \"$ppsi_vlans\" in CONFIG_VLANS_PORT"$i_zero"_VID"
				continue;
			fi
		fi

		if [ "$port_mode_trunk" = "y" ] \
		    || [ "$port_mode_disabled" = "y" ] \
		    || [ "$port_mode_unqualified" = "y" ]; then
			ppsi_vlans=$(eval "echo \$CONFIG_VLANS_PORT"$i_zero"_VID")
			if [ -n "$ppsi_vlans" ]; then
				mod_vlans=${ppsi_vlans//;/,}
				echo "vlan $mod_vlans" >> $OUTPUT_FILE
			fi
		fi
	fi

	# separate ports
	echo "" >> $OUTPUT_FILE
done
