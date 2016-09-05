<?php

session_start();
ob_start();

if (empty($_SESSION["WRS_INFO"])){

	$wrsinfogenerator = "/var/www/data/wrs-info-generator.php";
	shell_exec("/usr/bin/php-cgi ".$wrsinfogenerator."> /dev/null 2>&1 &");
	sleep(2);
}
if (is_file("data/wrs-info.php")) include "data/wrs-info.php";

if (empty($_SESSION["WRS_INFO"])){
	$_SESSION["WRS_INFO"] = $GLOBALS["WRS_INFO"];
}

if(empty($_SESSION["KCONFIG"])){
	load_kconfig();
}

// Forms are firstly generated by default, but can be modified by users.

$WRS_TABLE_INFO = Array (
	'DASHBOARD' => Array(
		'DASHBOARD_00' => Array(
			'name' => 'HOSTNAME',
			'value' => $_SESSION["WRS_INFO"][HOSTNAME],
		),
		'DASHBOARD_02' => Array(
			'name' => 'IP Address',
			'value' => $_SESSION["WRS_INFO"][IPADDRESS],
		),
		'DASHBOARD_03' => Array(
			'name' => 'MACADDRESS',
			'value' => $_SESSION["WRS_INFO"][MACADDRESS],
		),
		'DASHBOARD_04' => Array(
			'name' => 'KERNEL',
			'value' => $_SESSION["WRS_INFO"][KERNEL],
		),
		'DASHBOARD_05' => Array(
			'name' => 'FIRMWARE',
			'value' => $_SESSION["WRS_INFO"][FIRMWARE],
		),
		'DASHBOARD_06' => Array(
			'name' => 'HARDWARE',
			'value' => $_SESSION["WRS_INFO"][HARDWARE],
		),
		'DASHBOARD_07' => Array(
			'name' => 'FPGA',
			'value' => $_SESSION["WRS_INFO"][FPGA],
		),
		'DASHBOARD_08' => Array(
			'name' => 'MANUFACTURER',
			'value' => $_SESSION["WRS_INFO"][MANUFACTURER],
		),
		'DASHBOARD_09' => Array(
			'name' => 'SERIALNUMBER',
			'value' => $_SESSION["WRS_INFO"][SERIALNUMBER],
		),
		'DASHBOARD_10' => Array(
			'name' => 'KERNELCOMPILEDDATE',
			'value' => $_SESSION["WRS_INFO"][KERNELCOMPILEDDATE],
		),
	),
	'CONTACT' => Array (
		'CONTACT_00' => Array(
			'name' => 'FIRMWARE',
			'value' => $_SESSION["WRS_INFO"][FIRMWARE],
		),
		'CONTACT_01' => Array(
			'name' => 'KERNEL',
			'value' => $_SESSION["WRS_INFO"][KERNEL],
		),
		'CONTACT_02' => Array(
			'name' => 'COMPILEDBY',
			'value' => $_SESSION["WRS_INFO"][COMPILEDBY],
		),
		'CONTACT_03' => Array(
			'name' => 'KERNELCOMPILEDDATE',
			'value' => $_SESSION["WRS_INFO"][KERNELCOMPILEDDATE],
		),
		'CONTACT_04' => Array(
			'name' => 'HARDWARE',
			'value' => $_SESSION["WRS_INFO"][HARDWARE],
		),
		'CONTACT_05' => Array(
			'name' => 'FPGA',
			'value' => $_SESSION["WRS_INFO"][FPGA],
		),
		'CONTACT_06' => Array(
			'name' => 'MANUFACTURER',
			'value' => $_SESSION["WRS_INFO"][MANUFACTURER],
		),
		'CONTACT_07' => Array(
			'name' => 'SERIALNUMBER',
			'value' => $_SESSION["WRS_INFO"][SERIALNUMBER],
		),
		'CONTACT_08' => Array(
			'name' => 'GATEWARE',
			'value' => $_SESSION["WRS_INFO"][GATEWARE],
		),
		'CONTACT_09' => Array(
			'name' => 'GATEWAREBUILD',
			'value' => $_SESSION["WRS_INFO"][GATEWAREBUILD],
		),
		'CONTACT_10' => Array(
			'name' => 'WRSHDLCOMMIT',
			'value' => $_SESSION["WRS_INFO"][WRSHDLCOMMIT],
		),
		'CONTACT_11' => Array(
			'name' => 'GCORESCOMMIT',
			'value' => $_SESSION["WRS_INFO"][GCORESCOMMIT],
		),
		'CONTACT_12' => Array(
			'name' => 'WRCORESCOMMIT',
			'value' => $_SESSION["WRS_INFO"][WRCORESCOMMIT],
		),
	),
);

$WRS_FORMS = Array(
	'DNS_SETUP' => Array(
		'DNS_SETUP_00' => Array(
			'key' => "CONFIG_DNS_SERVER",
			'name' => "DNS Server",
			'value' => $_SESSION["KCONFIG"]["CONFIG_DNS_SERVER"],
			'vname' => "dnsserver",
		),
		'DNS_SETUP_01' => Array(
			'key' => "CONFIG_DNS_DOMAIN",
			'name' => "DNS Domain",
			'value' => $_SESSION["KCONFIG"]["CONFIG_DNS_DOMAIN"],
			'vname' => "dnsdomain",
		),
	),

	'SYSTEM_LOGS' => Array(
		'SYSTEM_LOGS_00' => Array(
			'key' => "CONFIG_WRS_LOG_HAL",
			'name' => "HAL log",
			'value' => $_SESSION["KCONFIG"]["CONFIG_WRS_LOG_HAL"],
			'vname' => "loghal",
		),
		'SYSTEM_LOGS_01' => Array(
			'key' => "CONFIG_WRS_LOG_RTU",
			'name' => "RTU log",
			'value' => $_SESSION["KCONFIG"]["CONFIG_WRS_LOG_RTU"],
			'vname' => "logrtu",
		),
		'SYSTEM_LOGS_02' => Array(
			'key' => "CONFIG_WRS_LOG_PTP",
			'name' => "PTP log",
			'value' => $_SESSION["KCONFIG"]["CONFIG_WRS_LOG_PTP"],
			'vname' => "logptp",
		),
		'SYSTEM_LOGS_03' => Array(
			'key' => "CONFIG_WRS_LOG_OTHER",
			'name' => "other applications log",
			'value' => $_SESSION["KCONFIG"]["CONFIG_WRS_LOG_OTHER"],
			'vname' => "logother",
		),
		'SYSTEM_LOGS_04' => Array(
			'key' => "CONFIG_WRS_LOG_MONIT",
			'name' => "Monitor log",
			'value' => $_SESSION["KCONFIG"]["CONFIG_WRS_LOG_MONIT"],
			'vname' => "logmonit",
		),
		'SYSTEM_LOGS_05' => Array(
			'key' => "CONFIG_WRS_LOG_SNMPD",
			'name' => "SNMPd log",
			'value' => $_SESSION["KCONFIG"]["CONFIG_WRS_LOG_SNMPD"],
			'vname' => "logsnmp",
		),
	),

	'NETWORK_SETUP' => Array(
		'NETWORK_SETUP_00' => Array(
			'key' => "CONFIG_ETH0_IP",
			'name' => IPADDRESS,
			'value' => $_SESSION["WRS_INFO"][IPADDRESS],
			'vname' => "ethip",
		),
		'NETWORK_SETUP_01' => Array(
			'key' => "CONFIG_ETH0_MASK",
			'name' => NETMASK,
			'value' => $_SESSION["WRS_INFO"][NETMASK],
			'vname' => "ethnetmask",
		),
		'NETWORK_SETUP_02' => Array(
			'key' => "CONFIG_ETH0_BROADCAST",
			'name' => BROADCAST,
			'value' => $_SESSION["WRS_INFO"][BROADCAST],
			'vname' => "ethbroadcast",
		),
		'NETWORK_SETUP_03' => Array(
			'key' => "CONFIG_ETH0_MAC",
			'name' => MACADDRESS,
			'value' => $_SESSION["WRS_INFO"][MACADDRESS],
			'vname' => "ethmac",
		),
	),

	'CONFIG_PPSI' => Array(
		'CONFIG_PPSI_00' => Array(
			'name' => "Clock Class",
			'value' => "",
			'vname' => "clkclass",
		),
		'CONFIG_PPSI_01' => Array(
			'name' => "Clock Accuracy",
			'value' => "",
			'vname' => "clkacc",
		),
	),

	'CONFIG_WRSAUXCLK' => Array(
		'CONFIG_WRSAUXCLK_00' => Array(
			'key' => "CONFIG_WRSAUXCLK_FREQ",
			'name' => "Frequency",
			'value' => $_SESSION["KCONFIG"]["CONFIG_WRSAUXCLK_FREQ"],
			'vname' => "auxclkfreq",
		),
		'CONFIG_WRSAUXCLK_01' => Array(
			'key' => "CONFIG_WRSAUXCLK_DUTY",
			'name' => "Duty",
			'value' => $_SESSION["KCONFIG"]["CONFIG_WRSAUXCLK_DUTY"],
			'vname' => "auxclkduty",
		),
		'CONFIG_WRSAUXCLK_02' => Array(
			'key' => "CONFIG_WRSAUXCLK_CSHIFT",
			'name' => "C. Shift",
			'value' => $_SESSION["KCONFIG"]["CONFIG_WRSAUXCLK_CSHIFT"],
			'vname' => "auxclkcshift",
		),
		'CONFIG_WRSAUXCLK_03' => Array(
			'key' => "CONFIG_WRSAUXCLK_SIGDEL",
			'name' => "Signal",
			'value' => $_SESSION["KCONFIG"]["CONFIG_WRSAUXCLK_SIGDEL"],
			'vname' => "auxclksigdel",
		),
		'CONFIG_WRSAUXCLK_04' => Array(
			'key' => "CONFIG_WRSAUXCLK_PPSHIFT",
			'name' => "PP. Shift",
			'value' => $_SESSION["KCONFIG"]["CONFIG_WRSAUXCLK_PPSHIFT"],
			'vname' => "auxclkppshift",
		),
	),

);

?>




