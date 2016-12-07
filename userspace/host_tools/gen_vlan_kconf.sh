#!/bin/bash

# Adam Wujek, CERN
# Generate Kconfig entries for Vlans
# redirect the output to the proper file

echo "menu \"VLANs\""
echo "config VLANS_ENABLE"
echo "	bool \"Enable VLANs\""
echo "	default n"
echo "	help"
echo "	  Enable VLAN configuration via dot-config"
echo ""
echo "menu \"Ports configuration\""
echo "	depends on VLANS_ENABLE"

############# generate options for ports
for port_i in {1..18}; do
	port_0i=$(printf "%02d" $port_i)
	echo ""
	echo "comment \"Port $port_i\""
	echo "choice VLANS_PORT"$port_0i"_MODE"
	echo "	prompt \"Port "$port_i" VLAN mode\""
	echo "	default VLANS_PORT"$port_0i"_MODE_UNQUALIFIED"
	echo "	help"
	if [ $port_i -eq 1 ]; then
		echo "	  In terms of VLAN-tag, there are four types of VLAN-tags that can"
		echo "	  extend the Ethernet Frame header:"
		echo "	  * none     - tag is not included in the Ethernet Frame"
		echo "	  * priority - tag that has VID=0x0"
		echo "	  * VLAN     - tag that has VID in the range 0x001 to 0xFFE"
		echo "	  * null     - tag that has VID=0xFFF"
		echo ""
		echo "	  The behaviour of each PMODE that can be set per-port depends on the"
		echo "	  type of VLAN-tag in the received frame."
		echo ""
		echo "	  - PMODE=access (0x0), frames with:"
		echo "	    * no VLAN-tag  : are admitted, tagged with the values of VID and"
		echo "	                     priority that are configured in PORT_VID and"
		echo "	                     PRIO_VAL respectively"
		echo "	    * priority tag : are admitted, their tag is unchanged, the value of"
		echo "	                     VID provided to the RTU is overridden with the"
		echo "	                     configured in PORT_VID. If PRIO_VAL is not -1,"
		echo "	                     the value of priority provided to RTU is"
		echo "	                     overridden with the configured PRIO_VAL"
		echo "	    * VLAN tag     : are discarded"
		echo "	    * null tag     : are discarded"
		echo ""
		echo "	  - PMODE=trunk (0x1), frames with:"
		echo "	    * no VLAN-tag  : are discarded"
		echo "	    * priority tag : are discarded"
		echo "	    * VLAN tag     : are admitted; if PRIO_VAL is not -1, the value of"
		echo "	                     priority provided to RTU is overridden with"
		echo "	                     the configured PRIO_VAL"
		echo "	    * null tag     : are discarded"
		echo ""
		echo "	  - PMODE=disabled (0x2), frames with:"
		echo "	    * no VLAN-tag  : are admitted. No other configuration is used even"
		echo "	                     if set."
		echo "	    * priority tag : are admitted; if PRIO_VAL is not -1, the value of"
		echo "	                     priority provided to RTU is overridden with"
		echo "	                     the configured PRIO_VAL"
		echo "	    * VLAN tag     : are admitted; if PRIO_VAL is not -1, the value of"
		echo "	                     priority provided to RTU is overridden with"
		echo "	                     the configured PRIO_VAL"
		echo "	    * null tag     : are admitted; if PRIO_VAL is not -1, the value of"
		echo "	                     priority provided to RTU is overridden with"
		echo "	                     the configured PRIO_VAL"
		echo ""
		echo "	  - PMODE=unqualified (0x3), frames with:"
		echo "	    * no VLAN-tag  : are admitted. No other configuration is used even"
		echo "	                     if set."
		echo "	    * priority tag : are admitted. Their tag is unchanged, the value of"
		echo "	                     VID provided to the RTU is overridden with the"
		echo "	                     configured in PORT_VID. If PRIO_VAL is not -1,"
		echo "	                     the value of priority provided to RTU is"
		echo "	                     overridden with the configured PRIO_VAL"
		echo "	    * VLAN tag     : are admitted; if PRIO_VAL is not -1, the value of"
		echo "	                     priority provided to RTU is overridden with"
		echo "	                     the configured PRIO_VAL"
		echo "	    * null tag     : discarded."
	else
		echo "	    Please check the help of VLANS_PORT01_MODE"
	fi
	echo ""
	echo "config VLANS_PORT"$port_0i"_MODE_ACCESS"
	echo "	bool \"Access mode\""
	echo "	help"
	echo "	  Please check the help of VLANS_PORT01_MODE"
	echo ""
	echo "config VLANS_PORT"$port_0i"_MODE_TRUNK"
	echo "	bool \"Trunk mode\""
	echo "	help"
	echo "	  Please check the help of VLANS_PORT01_MODE"
	echo ""
	echo "config VLANS_PORT"$port_0i"_MODE_DISABLED"
	echo "	bool \"VLAN-disabled mode\""
	echo "	help"
	echo "	  Please check the help of VLANS_PORT01_MODE"
	echo ""
	echo "config VLANS_PORT"$port_0i"_MODE_UNQUALIFIED"
	echo "	bool \"Unqualified mode\""
	echo "	help"
	echo "	  Please check the help of VLANS_PORT01_MODE"
	echo ""
	echo "endchoice"
	echo ""
	echo "choice VLANS_PORT"$port_0i"_UNTAG"
	echo "	prompt \"Port "$port_i" untag frames\""
	echo "	default VLANS_PORT"$port_0i"_UNTAG_ALL"
	echo "	depends on VLANS_PORT"$port_0i"_MODE_ACCESS"
	echo "	help"
	echo "	  Decide whether VLAN-tags should be removed"
	echo ""
	echo "config VLANS_PORT"$port_0i"_UNTAG_ALL"
	echo "	bool \"untag all\""
	echo "	help"
	echo "	  Untag all tagged frames."
	echo ""
	echo "config VLANS_PORT"$port_0i"_UNTAG_NONE"
	echo "	bool \"untag none\""
	echo "	help"
	echo "	  Keep VLAN tags for all tagged frames."
	echo ""
	echo "endchoice"
	echo ""
	echo "config VLANS_PORT"$port_0i"_PRIO"
	echo "	int \"Port "$port_i" priority\""
	echo "	default -1"
	echo "	range -1 7"
	echo "	help"
	echo "	  Priority value used when tagging frames or to override priority passed"
	echo "	  to RTU."
	echo "	  -1 disables the priority overwrite. Valid values are from -1 to 7."
	echo ""
	echo "config VLANS_PORT"$port_0i"_VID"
	echo "	int \"Port "$port_i" VID\""
	echo "	default 0"
	echo "	range 0 4094"
	echo "	depends on VLANS_PORT"$port_0i"_MODE_ACCESS || VLANS_PORT"$port_0i"_MODE_UNQUALIFIED"
	echo "	help"
	echo "	  VID value used when tagging frames or to override VID passed to RTU"
	echo ""
	echo "config VLANS_PORT"$port_0i"_VID_PTP"
	echo "	string \"Port "$port_i" VIDs for PTP\""
	echo "	default \"0\" if VLANS_PORT"$port_0i"_MODE_ACCESS || VLANS_PORT"$port_0i"_MODE_TRUNK"
	echo "	default \"\" if VLANS_PORT"$port_0i"_MODE_UNQUALIFIED"
	echo "	depends on VLANS_PORT"$port_0i"_MODE_ACCESS || VLANS_PORT"$port_0i"_MODE_TRUNK \\"
	echo "		   || VLANS_PORT"$port_0i"_MODE_UNQUALIFIED"
	echo "	help"
	echo "	  Semicolon separated list describing which vlans shall be assigned to"
	echo "	  a PTP instance on a particular port"
	echo ""

done

echo "# Ports configuration"
echo "endmenu"

################## VLANS configuration
echo "menu \"VLANs configuration\""
echo "	depends on VLANS_ENABLE"
echo ""
for set_i in {1..3}; do
	if [ $set_i == "1" ]; then
		vlan_min=0
		vlan_max=22
	fi
	if [ $set_i == "2" ]; then
		vlan_min=23
		vlan_max=100
	fi
	if [ $set_i == "3" ]; then
		vlan_min=101
		vlan_max=4094
	fi

	vlan_min_0=$(printf "%04d" $vlan_min)
	echo "config VLANS_ENABLE_SET"$set_i
	echo "	bool \"Enable configuration for VLANs "$vlan_min"-"$vlan_max"\""
	echo "	default n"
	echo "	help"
	echo ""
	echo "menu \"Configuration for VLANs "$vlan_min"-"$vlan_max"\""
	echo "	depends on VLANS_ENABLE_SET"$set_i

	for ((vlan_i=$vlan_min;vlan_i<=$vlan_max;vlan_i++));do
		vlan_0i=$(printf "%04d" $vlan_i)
		echo "config VLANS_VLAN"$vlan_0i
		echo "	string \"VLAN"$vlan_i" configuration\""
		echo "	default \"\""
		echo "	help"

		if [ $vlan_i -eq $vlan_min ]; then
			# for the first VLAN in the menu print full help
			echo "	  Provide the configuration for VLAN"$vlan_i
			echo "	  Example:"
			echo "	  fid="$vlan_min",prio=4,drop=no,ports=1;2;3-5;7"
			echo "	  Where:"
			echo "	  --\"fid\" is a associated Filtering ID (FID) number. One FID can be"
			echo "	    associated with many VIDs. RTU learning is performed per-FID."
			echo "	    Associating many VIDs with a single FID allowed shared-VLANs"
			echo "	    learning."
			echo "	  --\"prio\" is a priority of a VLAN; can take values between -1 and 7"
			echo "	    -1 disables priority override, any other valid value takes"
			echo "	    precedence over port priority"
			echo "	  --If \"drop\" is set to y, yes or 1 all frames belonging to this VID are"
			echo "	    dropped (note that frame can belong to a VID as a consequence of"
			echo "	    per-port Endpoint configuration); can take values y, yes, 1, n, no, 0"
			echo "	  --\"ports\" is a list of ports separated with a semicolon sign (\";\");"
			echo "	    ports ranges are supported (with a minus sign)"
		else
			# for the rest just refer to the first VLAN in the menu
			echo "	  Please check the help of VLANS_VLAN"$vlan_min_0
		fi
		echo ""
	done
	echo "# Configuration for VLANs "$vlan_min"-"$vlan_max""
	echo "endmenu"
	echo ""
	echo ""
done

echo "# VLANs configuration"
echo "endmenu"
echo ""
echo "# VLANs"
echo "endmenu"
