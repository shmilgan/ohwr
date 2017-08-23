<?php
/*
 * Generates data/wrs-info.php
 *
 * @author José Luis Gutiérrez <jlgutierrez@ugr.es>
 *
 * Generates a data/wrs-info.php with the $options marked as "y" en
 * the last field.
 * It is generated once per session.
 *
 * Result: A file in $outputfilename with some info from the WRS.
 *
 */

#Save info to temp file so we dont have to execute command at each call

if(empty($_SESSION["WRSVERSIONTMP"])){
	$file_version = "/tmp/www_wrs_version.txt";
	if (!file_exists($file_version)) {
		$_SESSION["WRSVERSIONTMP"] = shell_exec("/wr/bin/wrs_version -t | tee > ".$file_version); 
	} else {
		$_SESSION["WRSVERSIONTMP"] = shell_exec("cat ".$file_version); 
	}
}

$outputfilename = "/var/www/data/wrs-info.php";


// Organized as (CONST,
//					FULL STRING,
//					BASH COMMAND TO GET THE VALUE,
//					ENABLED/DISABLED)
$options = Array (
	Array ("IPADDRESS", "IP Address", "ifconfig eth0 | grep 'inet addr:' | cut -d: -f2 | awk '{ print $1}'", "y"),
	Array ("NETMASK", "Netmask", "ifconfig eth0 | grep 'inet addr:' | cut -d: -f4 | awk '{ print $1}'", "y"),
	Array ("BROADCAST","Broadcast","ifconfig eth0 | grep 'inet addr:' | cut -d: -f3 | awk '{ print $1}'","y"),
	Array ("MACADDRESS","Mac Address","ifconfig eth0 | grep -o -E '([[:xdigit:]]{1,2}:){5}[[:xdigit:]]{1,2}'","y"),
	Array ("HOSTNAME","Hostname","uname -n","y"),
	Array ("KERNEL","Kernel Version","uname -r","y"),
	Array ("KERNELCOMPILEDDATE","Kernel Compiled Date","uname -v","y"),
	Array ("FIRMWARE","Firmware Version","/wr/bin/wrs_version |  awk '{print $4}'","y"),
	Array ("HARDWARE","Hardware Version","cat /tmp/www_wrs_version.txt  | grep 'scb\|back' | sort -r | sed 's/back/ back/' | sed 's/-version: /: v/'","y"),
	Array ("FPGA","FPGA Version","cat /tmp/www_wrs_version.txt | grep 'fpga-type' | sed 's/[^:]*: //'","y"),
	Array ("COMPILEDBY","Compiled By","cat /tmp/www_wrs_version.txt | grep 'bult-by' | sed 's/[^:]*: //'","y"),
	Array ("MANUFACTURER","Manufacturer","cat /tmp/www_wrs_version.txt | grep 'manufacturer' | sed 's/[^:]*: //'","y"),
	Array ("SERIALNUMBER","Serial Number","cat /tmp/www_wrs_version.txt | grep 'serial' | sed 's/[^:]*: //'","y"),
	Array ("GATEWARE","Gateware Version","cat /tmp/www_wrs_version.txt | grep 'gateware-version' | sed 's/[^:]*: //'","y"),
	Array ("GATEWAREBUILD","Gateware Build","cat /tmp/www_wrs_version.txt | grep 'gateware-build' | sed 's/[^:]*: //'","y"),
	Array ("WRSHDLCOMMIT","WR Switch HDL Commit","cat /tmp/www_wrs_version.txt | grep 'wr_switch_hdl-commit' | sed 's/[^:]*: //'","y"),
	Array ("GCORESCOMMIT","General Cores Commit","cat /tmp/www_wrs_version.txt | grep 'general-cores-commit' | sed 's/[^:]*: //'","y"),
	Array ("WRCORESCOMMIT","WR Cores Commit","cat /tmp/www_wrs_version.txt | grep 'wr-cores-commit' | sed 's/[^:]*: //'","y"),
);

// Code for wrs-info.php generation.

$output = '<?php

// This file has been automatically generated by wrs-info-generator.php
// Do not touch it.
// If you want to perform changes, use wrs-info.generator.php';

$defines = "\n\n// ---- BEGINNING OF DEFINE SECTION ----- //\n";
$datastructure = "\$WRS_INFO = Array(\n";

foreach($options as $element){
	if($element[3]=="y"){
		//We include the element in the resulting wrs-info.php data structure.
		$defines .= 'define("'.$element[0].'","'.$element[1].'");'."\n";
		$datastructure .= "\t".$element[0]." => '".str_replace("\n","",shell_exec($element[2]))."',\n";
	}
}

$defines .= "\n// ---- END OF DEFINE SECTION ----- //\n";
$datastructure .= ");\n";

$output .= $defines;
$output .= $datastructure;

$output .= "?>\n";

// Save content to file.
file_put_contents($outputfilename, $output);

?>
