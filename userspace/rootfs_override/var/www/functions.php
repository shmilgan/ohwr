<?php

//Global Variables
$etcdir="/usr/wr/etc/"; //configuration file folder for WRS
$snmpconf="snmpd.conf";
$ppsiconf="ppsi.conf";
$wrswhalconf="wrsw_hal.conf";
$sfpdatabaseconf="sfp_database.conf";
$wrdateconf="wr_date.conf";
$vlancolor = array("#27DE2A", "#B642A8", "#6E42B6", "#425DB6" , "#428DB6", "#4686B6", "#43B88B", "#42B65F", "#82B642", "#B6AE42", "#B67E42");
$MAX_PHP_FILESIZE = 40;
$phpusersfile="/usr/etc/phpusers";
$profilefile="/usr/etc/profile";
$phpinifile="/etc/php.ini";
$interfacesfile = "/usr/etc/network/interfaces";

/*
 * Displays the current status of each enpoint.
 * 
 * @author José Luis Gutiérrez <jlgutierrez@ugr.es>
 *
 * Displays the current status of each endpoint as wr_mon tool 
 * in the swich does. It retrieves the information from wr_mon
 * and displays: master and slaves endpoints, calibration and locking 
 * status.
 * If all endpoints looks disabled (but they are not) it means
 * PTP is not running.
 * 
 */
function wrs_header_ports(){
		
	// Check whether $WRS_MANAGEMENT is set or we take the program
	// by default.

	session_start();
	if(empty($_SESSION['portsupdated'])){
		$_SESSION['portsupdated'] = intval(shell_exec("date +%s"));
	}
	 
	// Let's update endpoints info every 15 seconds.
	$currenttime = intval(shell_exec("date +%s"));
	$interval = $currenttime - $_SESSION['portsupdated'];
		
	if(!file_exists("/tmp/ports.conf") || $interval>15){
		$cmd = wrs_env_sh();
		shell_exec("killall wr_management");
		$str = shell_exec($cmd." ports");
		$fp = fopen('/tmp/ports.conf', 'w+');
		fwrite($fp, $str);
		fclose($fp);
		$ports = $str;
		$_SESSION['portsupdated'] = intval(shell_exec("date +%s"));	
	}else{
		$ports = shell_exec("cat /tmp/ports.conf");
	}

	$ports = explode(" ", $ports);

	// We parse and show the information comming from each endpoint.
	echo "<table border='0' align='center'  vspace='1'>";
	echo '<tr><th><h1 align=center>White-Rabbit Switch Manager</h1></th></tr>';
	echo '</table>';
	echo "<table border='0' align='center' vspace='15'>";
	
	echo '<tr>';
	$cont = 1;
	for($i=1; $i<18*4; $i=$i+4){
		
		if (strstr($ports[($i-1)],"up")){
			if (!strcmp($ports[($i)],"Master")){
				echo '<th>'."<IMG SRC='img/master.png' align=left ,   width=40 , hight=40 , border=0 , alt='master', title='wr".$cont."'>".'</th>';
			}else{
				echo  '<th>'."<IMG SRC='img/slave.png' align=left ,  width=40 , hight=40 , border=0 , alt='slave', title='wr".$cont."'>".'</th>';
			}
		}else{
			echo '<th>'."<IMG SRC='img/linkdown.png' align=left ,  width=40 , hight=40 , border=0 , alt='down', title='wr".$cont."'>".'</th>';
		}
		$cont++;
		
	}
	echo '</tr>';
	
	echo '<tr>';
	for($i=1; $i<18*4; $i=$i+4){
		
		if (!strstr($ports[($i+1)],"NoLock")){
			echo '<th>'."<IMG SRC='img/locked.png' align=center ,  width=15 , hight=15 , border=0 , alt='locked', title = 'locked'>";
		}else{
			echo '<th>'."<IMG SRC='img/unlocked.png' align=center ,  width=15 , hight=15 , border=0 , alt='unlocked', title = 'unlocked'>";
		}
		if (!strstr($ports[($i+2)],"Uncalibrated")){
			echo "<IMG SRC='img/check.png' align=center ,  width=15 , hight=15 , border=0 , alt='check', title = 'calibrated'>".'</th>';
		}else{
			echo "<IMG SRC='img/uncheck.png' align=center ,  width=15 , hight=15 , border=0 , alt='uncheck', title = 'uncalibrated'>".'</th>';
		}
	}
	echo '</tr>';
	
	echo '<tr>';
	echo '</tr>';
	
	echo '</table>';

}

/*
 * Displays OS, Hardware and protocols running on the switch
 * 
 * @author José Luis Gutiérrez <jlgutierrez@ugr.es>
 *
 * Displays the info comming from the following commands:
 * 	 uname, shw_ver, wr_date and php.ini
 * 
 */
function wrs_main_info(){
	
	//Changing php filesize in case it is necessary
	if(wrs_php_filesize()<$GLOBALS['MAX_PHP_FILESIZE']){php_file_transfer_size($GLOBALS['MAX_PHP_FILESIZE']);}
	
	if(empty($_SESSION["utc"])){
		$utc_exists = shell_exec("cat ".$GLOBALS['profilefile']." | grep -c TZ");
		if($utc_exists){
			$_SESSION["utc"]=shell_exec("cat ".$GLOBALS['profilefile']." | grep TZ | awk '{print $2}' | sed 's/TZ=//g'");
		}else{
			$_SESSION["utc"]="UTC";
		}
	}
	
	echo "<table border='1' align='center' class='altrowstabledash' id='alternatecolor'>";
	echo '<tr class="sub"><td> <b><center>Switch Info </center></b></td></tr>';

	$str = shell_exec("uname -n");
	if(strcmp($str,"(none)")) shell_exec("/bin/busybox hostname -F /etc/hostname");
	echo '<tr><th align=center><b><font color="darkblue">Hostname</font></b></th><th><center>'; $str = shell_exec("uname -n"); echo $str; echo '</center></th></tr>';
	echo '<tr><th  align=center> <b><font color="darkblue">Switch Mode</font></b> </th><th><center>'; $str = check_switch_mode(); echo $str; echo '</center></th></tr>';
	echo '<tr><th  align=center> <b><font color="darkblue">IP Address</font></b> </th><th><center>'; $ip = shell_exec("ifconfig eth0 | grep 'inet addr:' | cut -d: -f2 | awk '{ print $1}'"); echo $ip;  
			echo '(<a href="network.php">'; echo wrs_interface_setup(); echo '</a>)'; echo '</center></th></tr>';
	echo '<tr><th  align=center> <b><font color="darkblue">HW Address</font></b> </th><th><center>'; $mac = shell_exec("ifconfig eth0 | grep -o -E '([[:xdigit:]]{1,2}:){5}[[:xdigit:]]{1,2}'"); echo $mac; echo '</center></th></tr>';
	//echo '<tr><th> <b>OS Release:</b> </th><th><center>'; $str = shell_exec("uname -r"); echo $str; echo '</center></th></tr>';
	//echo '<tr><th> <b>OS name:</b> </th><th><center>'; $str = shell_exec("uname -s"); echo $str; echo '</center></th></tr>';
	echo '<tr><th  align=center> <b><font color="darkblue">Kernel Version</font></b> </th><th><center>'; $str = shell_exec("uname -r"); echo $str; $str = shell_exec("uname -v"); echo $str; echo '</center></th></tr>';
	
	echo '<tr><th  align=center> <b><font color="darkblue">Firmware Version</font></b> </th><th><center> '; $str = shell_exec("/wr/bin/shw_ver  |  awk '{print $4}'");
	echo '<a href="showfile.php?help_id=gateware&name=GateWare Info" onClick="showPopup(this.href);return(false);"</a>';
	echo $str; echo '</center></th></tr>';
	
	echo '<tr><th  align=center> <b><font color="darkblue">PCB Version</font></b> </th><th><center>'; $str = shell_exec("/wr/bin/shw_ver -p"); echo $str;  echo '</center></th></tr>';
	echo '<tr><th  align=center> <b><font color="darkblue">FPGA</font></b> </th><th><center>'; $str = shell_exec("/wr/bin/shw_ver -f"); echo $str; echo '</center></th></tr>';
	echo '<tr><th  align=center> <b><font color="darkblue">Compiling Date</font></b> </th><th><center>'; $str = shell_exec("/wr/bin/shw_ver -c"); echo $str; echo '</center></th></tr>';
	echo '<tr><th  align=center> <b><font color="darkblue">White-Rabbit Date</font></b></th><th><center>'; $str = shell_exec("export TZ=".$_SESSION['utc']." /wr/bin/wr_date -n get"); echo str_replace("\n","<br>",$str); echo '</center></th></tr>';
	echo '<tr><th  align=center> <b><font color="darkblue">PPSi</font></b> </th><th><center>';  echo wrs_check_ptp_status() ? '[<A HREF="ptp.php">on</A>]' : '[<A HREF="ptp.php">off</A>]'; echo '</center></th></tr>';
	echo '<tr><th  align=center> <b><font color="darkblue">Net-SNMP Server</font></b> </th><th><center>';  echo check_snmp_status() ? '[on] ' : '[off] '; echo '&nbsp;&nbsp;ver. '; echo shell_exec("snmpd -v | grep version | awk '{print $3}'");
			echo '( port '; $str = shell_exec("cat ".$GLOBALS['etcdir']."snmpd.conf | grep agent | cut -d: -f3 | awk '{print $1}'"); echo $str; echo ')'; 	echo /*" <a href='help.php?help_id=snmp' onClick='showPopup(this.href);return(false);'> [OIDs]</a>*/"</center></th></tr>";
	echo '<tr><th  align=center> <b><font color="darkblue">NTP Server</font></b> </th><th><center> <A HREF="management.php">';  $str = check_ntp_server(); echo $str; echo '</A> '.$_SESSION['utc'].'</center></th></tr>';
	echo '<tr><th  align=center> <b><font color="darkblue">Max. Filesize Upload</font> </b></th><th><center>'; echo shell_exec("cat ".$GLOBALS['phpinifile']." | grep upload_max_filesize | awk '{print $3}'"); echo '</center></th></tr>';
	echo '</table>';
	
	
	  

}

function check_ntp_server(){
	$output = "cat ".$GLOBALS['etcdir'].$GLOBALS['wrdateconf']. " | grep ntpserver | awk '{print $2}' ";
	$output = shell_exec($output);

	if(!strcmp($output, "")){
		return "not set";
	}else{
		return $output;
	}
	
}
function check_snmp_status(){
	$output = intval(shell_exec("ps aux | grep -c snmpd"));
	return ($output>2) ? 1 : 0;
	
	
}


function wrs_interface_setup(){
	
	$interfaces = shell_exec('cat '.$GLOBALS['interfacesfile'].' | grep dhcp');
	
	return (strcmp($interfaces[0],"#")) ? "dhcp" : "static";
}

/*
 * It checks whether the filesystem is writable or not.
 * 
 * @author José Luis Gutiérrez <jlgutierrez@ugr.es>
 *
 * It checks whether the filesystem is writable or not and prints a
 * warning message if so.
 * 
 */
function wrs_check_writeable(){

	$output = shell_exec('mount | grep "(ro,"');
	echo (!isset($output) || trim($output) == "") ? "" : "<br><font color='red'>WARNING: WRS is mounted as READ-ONLY, please contact the maintainer</font>";

}

/*
 * It modifies filesystem to rw o ro
 * 
 * @author José Luis Gutiérrez <jlgutierrez@ugr.es>
 *
 * @param $m should be rw for writable and ro for read-only
 * 
 */
function wrs_change_wrfs($m){
	
	$output = shell_exec('/wr/bin/wrfs_mnt.sh '.$m);
	
}

/*
 * It checks whether the ptpd daemon is running or not.
 * 
 * @author José Luis Gutiérrez <jlgutierrez@ugr.es>
 *
 * It checks whether the ptpd daemon is running or not by counting 
 * the number of entries in ps command.
 * 
 * @return ptp is true or false
 * 
 */
function wrs_check_ptp_status(){
	$output = intval(shell_exec("ps aux | grep -c ppsi"));
	return ($output>2) ? 1 : 0;
}

/*
 * It modifies filesize transfer value.
 * 
 * @author José Luis Gutiérrez <jlgutierrez@ugr.es>
 *
 * It modifies filesize transfer value in php.ini. Two variables must be
 * modified: upload_max_filesize and post_max_size
 * 
 * @param string $size New PHP sent filename value.
 * 
 */
function php_file_transfer_size($size){
	
	// We remove the blank space
	$size=trim($size);
	
		
	wrs_change_wrfs("rw");
	// We modify fist upload_max_filesize in php.ini
	$prev_size = shell_exec("cat ".$GLOBALS['phpinifile']." | grep upload_max_filesize | awk '{print $3}'");
	$prev_size=trim($prev_size);
	$cmd = "sed -i 's/upload_max_filesize = ".$prev_size."/upload_max_filesize = ".$size."M/g' ".$GLOBALS['phpinifile'];
	shell_exec($cmd);
	
	// We modify post_max_size in php.ini
	$prev_size = shell_exec("cat ".$GLOBALS['phpinifile']." | grep post_max_size | awk '{print $3}'");
	$prev_size=trim($prev_size);
	$cmd ="sed -i 's/post_max_size = ".$prev_size."/post_max_size = ".$size."M/g' ".$GLOBALS['phpinifile'];
	shell_exec($cmd);
	shell_exec("cat ".$GLOBALS['phpinifile']." >/usr/etc/php.ini"); //We store it in /usr/etc/php.ini copy. Just in case
	wrs_change_wrfs("ro");
	
	
	//echo '<p align=center>File upload size changed to '.$size.'</p>';	
}

/*
 * It modifies each endpoint configuration 
 * 
 * @author José Luis Gutiérrez <jlgutierrez@ugr.es>
 *
 * It modifies each endpoint configuration by using wrs_phytool program
 * in switch. 
 * Operations: show registers, modify registers, enable/disable 
 * transmission, lock and master/grandmaster configuration.
 * 
 * @param string $option1 New PHP sent filename value.
 * @param string $endpoint Endpoint to apply new configuration.
 * 
 */
function wr_endpoint_phytool($option1, $endpoint){
	
	$cont=0;
	
	// User wants to display endpoint's registers
	if(!strcmp($option1, "dump")){
	
		$output=shell_exec("/wr/bin/wr_phytool ".$endpoint." dump");
		$ports = explode(" ", $output);
		
		echo "<table border='0' align='center'>";
		echo '<tr>';
		echo '<th>'.$endpoint.' Register</th>';
		echo '<th>Value</th>';
		echo '</tr>';
		
		for($i=7; $i<20*2; $i=$i+2){
			echo '<tr>';
			echo '<th>R'.$cont.'</th>';
			echo '<th>'.substr($ports[($i)],0,10).'</th>';
			echo '</tr>';
			$cont++;
		}
		
		echo '</tr>';
		echo '</table>';
		
		//if (!strcmp($_POST['update'], "yes")){
			//echo 'aki stamos!';
		//}
	
	// User wants to modify endpoint's registers
	} else if(!strcmp($option1, "wr")){
		
		$output=shell_exec("/wr/bin/wr_phytool ".$endpoint." dump");
		$ports = explode(" ", $output);
		
		echo '<br>';
		echo '<center></center><form  method=POST>';
		echo "<table border='0' align='center'>";
		echo '<tr>';
		echo '<th>'.$endpoint.' Registers</th>';
		echo '<th><center>Value</center></th>';
		echo '</tr>';
		
		for($i=0; $i<18; $i++){
			echo '<tr>';
			echo '<th>R'.$i.'</th>';
			echo '<th><input type="text" name="r'.$i.'" value="'.$_POST['r'.$i].'"></th>';
			echo '</tr>';
			$cont++;
		}
		
		echo '</tr>';
		echo '</table>';
		echo '<input type="hidden" name="option1" value="wr">';
		echo '<input type="hidden" name="wr" value="yes">';
		echo '<input type="hidden" name="endpoint" value="'.$endpoint.'">';
		echo '<center><input type="submit" value="Update" class="btn"></center></form><center>';
		
		if(!empty($_POST['wr'])){
			for($i=0; $i<18 ; $i++){
				if (!empty($_POST['r'.$i])){
					$cmd = '/wr/bin/wr_phytool '.$_POST['endpoint'].' wr '.dechex($i).' '.$_POST['r'.$i].'';
					$output=shell_exec($cmd);
					echo $endpoint.':R'.$i.' modified';
				}
			}
		}
	
	// User wants to enable transmission on endpoint
	} else if (!strcmp($option1, "txcal1")){
		$output=shell_exec('/wr/bin/wr_phytool '.$endpoint.' txcal 1');
		echo $endpoint.' is now transmitting calibration';
	
	// User wants to disable transmission on endpoint
	} else if(!strcmp($option1, "txcal0")){
		$output=shell_exec('/wr/bin/wr_phytool '.$endpoint.' txcal 0');
		echo $endpoint.' stopped transmitting calibration';
	
	// User wants to lock endpoint
	} else if(!strcmp($option1, "lock")){
		$output=shell_exec('/wr/bin/wr_phytool '.$endpoint.' rt lock');
		echo 'Locking finished';
	
	// User wants to make endpoint master
	} else if(!strcmp($option1, "master")){
		$output=shell_exec('/wr/bin/wr_phytool '.$endpoint.' rt master');
		echo 'Mastering finished' ;
	
	// User wants to make endpoint grandmaster	
	} else if(!strcmp($option1, "gm")){
		$output=shell_exec('/wr/bin/wr_phytool '.$endpoint.' rt gm');
		echo 'Grandmastering finished' ;
		
		
	} else if(!strcmp($option1, "hal_conf")){
		
	}
}


function wr_show_endpoint_rt_show(){

		
		$output=shell_exec('/wr/bin/wr_phytool wr0 rt show');
		$rts = nl2br($output);
		echo $rts;
		
		/*//Show RTS State Dump
		$output=shell_exec('/wr/bin/wr_phytool wr0 rt show');
		$rts = explode(" ", $output);
		
		echo "<table border='1' align='center' vspace='1'>";
		echo '<tr>';
		echo '<th>Endpoint</th>';
		echo '<th>Setpoint</th>';
		echo '<th>PS Current</th>';
		echo '<th>PS loopback</th>';
		echo '<th>PS flags</th>';
		echo '</tr>';
		
		$cont = 0;
		
		//First line
		echo '<tr>';
		echo '<th>wr'.$cont.'</th>';
		echo '<th>'.$rts[14].'</th>';
		echo '<th>'.$rts[14+7].'</th>';
		echo '<th>'.$rts[14+16].'</th>';
		echo '<th>'.$rts[14+22].'</th>';
		echo '</tr>';
		$cont++;
					
		for($op = 39; $op < 300; $op=$op+30){
			
			
			echo '<tr>';
			echo '<th>wr'.$cont.'</th>';
			echo '<th>'.$rts[$op].'</th>';
			echo '<th>'.$rts[($op+9)].'</th>';
			echo '<th>'.$rts[($op+18)].'</th>';
			echo '<th>'.$rts[($op+27)].'</th>';
			echo '</tr>';
			$cont++;
			
		}
		
		for($op = 310; $op < 500; $op=$op+29){
			

			echo '<tr>';
			echo '<th>wr'.$cont.'</th>';
			echo '<th>'.$rts[$op-2].'</th>';
			echo '<th>'.$rts[($op+7)].'</th>';
			echo '<th>'.$rts[($op+16)].'</th>';
			echo '<th>'.$rts[($op+25)].'</th>';
			echo '</tr>';
			$cont++;
			
		}
		echo '</table>';*/
	
	
}

/*
 * It returns the max. filesize that can be upload to the switch.
 * 
 * @author José Luis Gutiérrez <jlgutierrez@ugr.es>
 *
 * 
 * 
 */
function wrs_php_filesize(){
	
	 $size=shell_exec("cat ".$GLOBALS['phpinifile']." | grep upload_max_filesize | awk '{print $3}'"); 
	 $size=substr($size, 0, -2);
	 return $size;
	
}

/*
 * It loads binaries to the FPGA, LM32 and firmware folder.
 * 
 * @author José Luis Gutiérrez <jlgutierrez@ugr.es>
 *
 * It runs: 
 * 	load-virtex for loading .bin to the FPGA
 * 	load-lm32 for loading .bin to the lm32 professor
 * 	
 * 
 * 
 */
function wrs_load_files(){
		
		
		// Loading  and executing binary file with load-virtex
		if (!empty($_FILES['fpgafile']['name'])){
			$uploaddir = '/tmp/';
			$uploadfile = $uploaddir . basename($_FILES['fpgafile']['name']);
			echo '<pre>';
			if ((!strcmp(extension($_FILES['fpgafile']['name']), "bin")) && move_uploaded_file($_FILES['fpgafile']['tmp_name'], $uploadfile)) {
				echo "<center>File is valid, and was successfully uploaded.</center>\n";
				
				print "</pre>";
			
				echo '<center>Loading FPGA binary '.$_FILES['fpgafile']['name'].', please wait for the system to reboot</center>';
				$str = shell_exec("/wr/bin/load-virtex ".$uploadfile); 
				echo $str;
				echo '<br>System is rebooting, please wait for 30 seconds';
				$str = shell_exec("reboot"); 
				
			} else {
				echo "<center>File is not valid, please upload a .bin file.</center>\n";
			}

			
			
		// Loading  and executing binary file with load-lm32
		} else if (!empty($_FILES['lm32file']['name'])){
			$uploaddir = '/tmp/';
			$uploadfile = $uploaddir . basename($_FILES['lm32file']['name']);
			echo '<pre>';
			if ((!strcmp(extension($_FILES['lm32file']['name']), "bin")) && move_uploaded_file($_FILES['lm32file']['tmp_name'], $uploadfile)) {
				echo "<center>File is valid, and was successfully uploaded.</center>\n";
				
				print "</pre>";
			
				echo '<center>Loading lm32 binary '.$_FILES['lm32file']['name'].',, please wait for the system to reboot</center>';
				$str = shell_exec("/wr/bin/load-lm32 ".$uploadfile); 
				echo $str;
				echo '<br>System is rebooting, please wait for 30 seconds';
				$str = shell_exec("reboot"); 
				
			}  else {
				echo "<center>File is not valid, please upload a .bin file</center>\n";
			}

			
		// Loading  and copying binary file to /tmp folder on the switch.
		} else if (!empty($_FILES['file']['name'])){
			$uploaddir = '/tmp/';
			$uploadfile = $uploaddir . basename($_FILES['file']['name']);
			echo '<pre>';
			if (/*(!strcmp(extension($_FILES['file']['name']), "bin")) &&*/ move_uploaded_file($_FILES['file']['tmp_name'], $uploadfile)) {
				echo "<center>File is valid, and was successfully uploaded to tmp folder\n";
			}  else {
				echo "<center>File is not valid, please upload a .bin file</center>\n";
			}

			echo "</pre>";
		
		} else if (!empty($_POST["size"])){
			php_file_transfer_size(htmlspecialchars($_POST["size"]));
			header ('Location: load.php');
		}

	
	
}

/*
 * Used for halting, rebooting and mounting partitions as read-only or
 * writable on switch.
 * 
 * @author José Luis Gutiérrez <jlgutierrez@ugr.es>
 *
 * Used for halting, rebooting and mounting partitions as read-only or
 * writable on switch.
 * 	
 * 
 */
function wrs_management(){
	
		$cmd =  htmlspecialchars($_POST["cmd"]); 
		
		
		if(!strcmp($cmd, "halt")){
			echo '<br><br><br></be>System is halting';
			$output = shell_exec($cmd);
		}else if (!strcmp($cmd, "reboot")){
			echo '<br><br><br>System is rebooting. Please wait 30 seconds.';
			$output = shell_exec($cmd);
		}else if (!strcmp($cmd, "rw")){
			$output = shell_exec("/wr/bin/wrfs_mnt.sh rw");
			echo '<br><br><br>Partition is now writable';
		}else if (!strcmp($cmd, "ro")){
			$output = shell_exec("/wr/bin/wrfs_mnt.sh ro");
			echo '<br><br><br>Partition is now READ-ONLY';
		}else if (!strcmp($cmd, "size")){
			php_file_transfer_size(htmlspecialchars($_POST["size"]));
			header('Location: firmware.php');
		}else if (!strcmp($cmd, "change")){
			modify_switch_mode();
			$mode = check_switch_mode(); 
			echo '<br><br><br>Switch is now '.$mode;
			header ('Location: management.php');
			
		} else if (!empty($_FILES['file']['name'])){
			$uploaddir = '/tmp/';
			$uploadfile = $uploaddir . basename($_FILES['file']['name']);
			echo '<pre>';
			if (move_uploaded_file($_FILES['file']['tmp_name'], $uploadfile)) {
				echo '<p align=center ><font color="red"><br>Upgrade procedure will take place after reboot.<br>Please do not switch off the device during flashing procedure.</font></p>';
				rename($uploadfile, "/update/".($_FILES['file']['name']));
				//Reboot switch
				shell_exec("reboot");
			}  else {
				echo "<center>Something went wrong. File was not uploaded.</center>\n";
			}

			echo "</pre>";
		} else if (!strcmp($cmd, "remoteflash")){
			
			echo '<p align=center>Downloading '.$ohwrlink.'</p>';
			
			$filename="/tmp/wr-switch-sw-v4.0-rc1-20140710_binaries.tar";
			$firmware="/update/wrs-firmware.tar";
			$ohwrlink="http://www.ohwr.org/attachments/download/3095/wr-switch-sw-v4.0-rc1-20140710_binaries.tar";
		
			file_put_contents($filename, file_get_contents($ohwrlink));
			rename($filename, $firmware);
			echo '<p align=center>File successfully downloaded. Rebooting.</p>';
			shell_exec("reboot");
			
		} else if (!empty($_FILES['ppsi_conf']['name'])){
			
			$uploaddir = $GLOBALS['etcdir'];
			$uploadfile = $uploaddir . basename($_FILES['ppsi_conf']['name']);
			echo '<pre>';
			if ((!strcmp($_FILES['ppsi_conf']['name'],$GLOBALS['ppsiconf']) && (!strcmp(extension($_FILES['ppsi_conf']['name']), "conf")) && move_uploaded_file($_FILES['ppsi_conf']['tmp_name'], $uploadfile))) {
				echo "<center>File is valid, and was successfully uploaded to ".$GLOBALS['etcdir']." folder\n";
			}  else {
				echo "<center>File is not valid, please upload a ".$GLOBALS['ppsiconf']." file</center>\n";
			}

			echo "</pre>";
			
		} else if (!empty($_FILES['sfp_conf']['name'])){
			
			$uploaddir = $GLOBALS['etcdir'];
			$uploadfile = $uploaddir . basename($_FILES['sfp_conf']['name']);
			echo '<pre>';
			if ((!strcmp($_FILES['sfp_conf']['name'],$GLOBALS['sfpdatabaseconf'])) && (!strcmp(extension($_FILES['sfp_conf']['name']), "conf")) && move_uploaded_file($_FILES['sfp_conf']['tmp_name'], $uploadfile)) {
				echo "<center>File is valid, and was successfully uploaded to ".$GLOBALS['etcdir']." folder\n";
			}  else {
				echo "<center>File is not valid, please upload a ".$GLOBALS['sfpdatabaseconf']." file</center>\n";
			}

			echo "</pre>";
			
		} else if (!empty($_FILES['snmp_conf']['name'])){
			
			$uploaddir = $GLOBALS['etcdir'];
			$uploadfile = $uploaddir . basename($_FILES['snmp_conf']['name']);
			echo '<pre>';
			if ((!strcmp($_FILES['snmp_conf']['name'],$GLOBALS['snmpconf'])) && (!strcmp(extension($_FILES['snmp_conf']['name']), "conf")) && move_uploaded_file($_FILES['snmp_conf']['tmp_name'], $uploadfile)) {
				echo "<center>File is valid, and was successfully uploaded to ".$GLOBALS['etcdir']." folder\n";
			}  else {
				echo "<center>File is not valid, please upload a ".$GLOBALS['snmpconf']." file</center>\n";
			}

			echo "</pre>";
			
		} else if (!empty($_FILES['hal_conf']['name'])){
			
			$uploaddir = $GLOBALS['etcdir'];
			$uploadfile = $uploaddir . basename($_FILES['hal_conf']['name']);
			echo '<pre>';
			if ((!strcmp($_FILES['hal_conf']['name'],$GLOBALS['wrswhalconf'])) && (!strcmp(extension($_FILES['hal_conf']['name']), "conf")) && move_uploaded_file($_FILES['hal_conf']['tmp_name'], $uploadfile)) {
				echo "<center>File is valid, and was successfully uploaded to ".$GLOBALS['etcdir']." folder\n";
			}  else {
				echo "<center>File is not valid, please upload a ".$GLOBALS['wrswhalconf']." file</center>\n";
			}

			echo "</pre>";
			
		} else if (!empty($_FILES['restore_conf']['name'])){
			
			$uploaddir = $GLOBALS['etcdir'];
			$uploadfile = $uploaddir . basename($_FILES['restore_conf']['name']);
			echo '<pre>';
			if ((!strcmp(extension($_FILES['restore_conf']['name']), "gz")) && move_uploaded_file($_FILES['restore_conf']['tmp_name'], $uploadfile)) {
				
				shell_exec("tar -xvf ".$uploadfile. " -C ". $GLOBALS['etcdir'] ); //untar the file
				
				echo "<center>Configuration restored sucessfully. Rebooting system.\n";
				shell_exec("reboot");
			}  else {
				echo "<center>File is not valid, please upload a .tar.gz file</center>\n";
			}

			shell_exec("rm $uploadfile");
			
			echo "</pre>";
			
		} else if (!strcmp($cmd, "Backup")){
			
			//Prepare backup (tar)
			$backupfile="backup".date("Y-m-d").".tar.gz";
			shell_exec("cd ".$GLOBALS['etcdir']."; tar -cvf ".$backupfile." *; mv ".$backupfile." /var/www/download/".$backupfile);
			$backupfile="/download/$backupfile";
			
			//Download the file
			header('Location: '.$backupfile);
			 
		} else if (!strcmp($cmd, "ntp")){
			
			$output = "ntpserver ". htmlspecialchars($_POST["ntpip"])."\n\n";
			
			if (file_exists($GLOBALS['etcdir'].$GLOBALS['wrdateconf'])) {
				
				$current = file_get_contents($GLOBALS['etcdir'].$GLOBALS['wrdateconf']);
				
				if(substr_count($current,"ntpserver")>0){
					
					$current = shell_exec("sed '/ntpserver/d' ".$GLOBALS['etcdir'].$GLOBALS['wrdateconf']);
					$current = str_replace("\n", "", $current);
				}
				
				$current .= $output;
				
				$file=$GLOBALS['etcdir'].$GLOBALS['wrdateconf'];
				unlink($GLOBALS['etcdir'].$GLOBALS['wrdateconf']);
				$file = fopen($GLOBALS['etcdir'].$GLOBALS['wrdateconf'],"w+");
				fwrite($file,$current);
				fclose($file);
				
			} else {
				
				$file = fopen("/tmp/".$GLOBALS['wrdateconf'],"w+");
				fwrite($file,$output);
				fclose($file);
				
				//We move the file to /wr/etc/
				copy('/tmp/'.$GLOBALS['wrdateconf'], $GLOBALS['etcdir'].$GLOBALS['wrdateconf']);
			}
			
			//Set UTC
			$UTC=htmlspecialchars($_POST["utc"]); 
			$_SESSION['utc']=$UTC;				
			
			//Export to /etc/profile --> when /etc/profile remains saved...
			$utc_exists = shell_exec("cat ".$GLOBALS['profilefile']." | grep -c TZ");
			$utc_prev = shell_exec("cat ".$GLOBALS['profilefile']." | grep TZ");
			$utc_prev=trim($utc_prev);
			if($utc_exists>0){ 
				$sed_cmd = 'sed -i "s/'.$utc_prev.'/export TZ='.$_SESSION['utc'].'/g" '.$GLOBALS['profilefile'];
				shell_exec ($sed_cmd);
			}else{ // Add TZ to profile file
				shell_exec ('echo "export TZ='.$_SESSION['utc'].'" >>'.$GLOBALS['profilefile']);	
			}
			
			echo '<br><p align=center>Rebooting...</p>';
			
			//Reboot the switch after 1s
			usleep(1000000);
			shell_exec("reboot");
			
			
			
		} else if (!strcmp($cmd, "backup-wrs")){
			
			//Backup wrs firmware
			
		}  else if (!strcmp($cmd, "snmp")){
			
			if(check_snmp_status()){ //It is running
				
				//Stop SNMP
				shell_exec("killall snmpd");
				
			}else{ //Not running
				
				shell_exec("/etc/init.d/S80snmp > /dev/null 2>&1 &");
				
			}
			
			header('Location: management.php');
		}
		
		
		
}


/**
     * Download file
     *
     * @param string $path
     * @param string $type
     * @param string $name
     * @param bool $force_download
     * @return bool
     */
    function download($path, $name = '', $type = 'application/octet-stream', $force_download = true) {

        if (!is_file($path) || connection_status() !== 0);

        if($force_download) {
            header("Cache-Control: public");
        } else {
            header("Cache-Control: no-store, no-cache, must-revalidate");
            header("Cache-Control: post-check=0, pre-check=0", false);
            header("Pragma: no-cache");
        }

        header("Expires: ".gmdate("D, d M Y H:i:s", mktime(date("H")+2, date("i"), date("s"), date("m"), date("d"), date("Y")))." GMT");
        header("Last-Modified: ".gmdate("D, d M Y H:i:s")." GMT");
        header("Content-Type: $type");
        header("Content-Length: ".(string)(filesize($path)));

        $disposition = $force_download ? 'attachment' : 'inline';

        if(trim($name) == '') {
            header("Content-Disposition: $disposition; filename=" . basename($path));
        } else {
            header("Content-Disposition: $disposition; filename=\"" . trim($name)."\"");
        }

        header("Content-Transfer-Encoding: binary\n");

        if ($file = fopen($path, 'rb')) {
            while(!feof($file) and (connection_status()==0)) {
                print(fread($file, 1024*8));
                flush();
            }
            fclose($file);
        }

        return((connection_status() == 0) && !connection_aborted());
    }  

/*
 * This function configures the PTP daemon.
 * 
 * @author José Luis Gutiérrez <jlgutierrez@ugr.es>
 *
 * It configures PTP daemon:
 * - By default (-A -c)
 * - By changing the following values:
 * 	
 * 
 */
function wrs_ptp_configuration(){
	
	
	echo '<center>';
	if(!empty($_POST["b"])){
		$cmd .= " -b ".htmlspecialchars($_POST["b"]); 
		echo '<br>Network Interface bound to '.htmlspecialchars($_POST["b"]);
	} 
	if (!empty($_POST["u"])){
		$cmd .= " -u ".htmlspecialchars($_POST["u"]); 
		echo '<br>Unicast sent to '.htmlspecialchars($_POST["u"]);
	} 
	if (!empty($_POST["i"])){
		$cmd .= " -i ".htmlspecialchars($_POST["i"]); 
		echo '<br>PTP Domain number is now '.htmlspecialchars($_POST["i"]);
	} 
	if (!empty($_POST["n"])){
		$cmd .= " -n ".htmlspecialchars($_POST["n"]); 
		echo '<br>Announce Interval set to '.htmlspecialchars($_POST["n"]);
	} 
	if (!empty($_POST["y"])){
		$cmd .= " -y ".htmlspecialchars($_POST["y"]); 
		echo '<br>Sync Interval set to '.htmlspecialchars($_POST["y"]);
	} 
	if (!empty($_POST["r"])){
		$cmd .= " -r ".htmlspecialchars($_POST["r"]); 
		echo '<br>Clock accuracy set to '.htmlspecialchars($_POST["r"]);
	} 
	if (!empty($_POST["v"])){
		$cmd .= " -v ".htmlspecialchars($_POST["v"]); 
		echo '<br>Clock Class is now set to '.htmlspecialchars($_POST["v"]);
	} 
	if (!empty($_POST["p"])){
		$cmd .= " -p ".htmlspecialchars($_POST["p"]); 
		echo '<br>Priority changed to '.htmlspecialchars($_POST["p"]);
	} 
	
	if(!strcmp($_POST['cmd'],"ppsiupdate")){
		
		if(wrs_check_ptp_status()){ //PPSi is enabled.
			shell_exec("killall ppsi"); 	
		}else{  //PPSi is disabled.
			$ptp_command = "/wr/bin/ppsi > /dev/null 2>&1 &";
			$output = shell_exec($ptp_command); 		
		}
		header('Location: ptp.php');
		
	}
	if (!empty($_POST["clkclass"])){
		$old_value= rtrim(shell_exec("cat ".$GLOBALS['etcdir'].$GLOBALS['ppsiconf']." | grep class "));
		$new_value="clock-class ".htmlspecialchars($_POST["clkclass"]); 
		$sed = 'sed -i "s/'.$old_value.'/'.$new_value.'/g" '.$GLOBALS['etcdir'].$GLOBALS['ppsiconf'];echo $sed;
		shell_exec($sed);
		echo '<br>Clock Class changed to '.htmlspecialchars($_POST["clkclass"]);
	} 
	if (!empty($_POST["clkacc"])){
		$old_value= rtrim(shell_exec("cat ".$GLOBALS['etcdir'].$GLOBALS['ppsiconf']." | grep accuracy "));
		$new_value="clock-accuracy ".htmlspecialchars($_POST["clkacc"]);
		$sed ='sed -i "s/'.$old_value.'/'.$new_value.'/g" '.$GLOBALS['etcdir'].$GLOBALS['ppsiconf'];echo $sed;
		shell_exec($sed);
		echo '<br>Clock Accuracy changed to '.htmlspecialchars($_POST["clkacc"]);
	} 
	if ((!empty($_POST["clkclass"])) || !empty($_POST["clkacc"])){
		// Redirect & relaunch.
		echo '<br>Clock values changed. Rebooting PPSi daemon.</br';
			
		//We must relaunch ptpd too. (by default)
		shell_exec("killall ppsi"); 
		$ptp_command = "/wr/bin/ppsi > /dev/null 2>&1 &";
		$output = shell_exec($ptp_command); 
		
		//Relaunching wrsw_hal to commit endpoint changes
		//shell_exec("killall wrsw_hal");
		//shell_exec("/wr/bin/wrsw_hal -c ".$GLOBALS['etcdir']."wrsw_hal.conf > /dev/null 2>&1 &");
		
		header('Location: ptp.php');
		exit;
	}
	echo '</center>';
	
	
}

/*
 * Checks whether $WRS_MANAGEMENT exists with the wr_management program
 * 
 * @author José Luis Gutiérrez <jlgutierrez@ugr.es>
 *
 * Checks whether $WRS_MANAGEMENT exists, if not, it points to the 
 * program by default (/wr/bin/wr_management)
 * 	
 * 
 */
function wrs_env_sh(){

	$output = shell_exec("echo $WRS_MANAGEMENT");
	if(file_exists($output)){
		$sh=$output;
	}else{
		$sh="/wr/bin/wr_management";
	}
	return $sh;

}

function wrs_vlan_configuration($input){
	
	//Stop previous daemon and delete configuration file.
	shell_exec('killall wrsw_rtud_new');
	shell_exec('rm /tmp/vlan.conf');
	
	$wrsw_rtud= '/wr/bin/wrsw_rtud_new -w 1 ';
	for ($id=0; $id<18; $id++){
		$wrsw_rtud .=  $input[$id].' ';
		
		//echo 'Endpoint '.$id.' added to VLAN'.$input[$id];
		//echo '<br>';
		shell_exec( 'echo '.$input[$id].' >>/tmp/vlan.conf');
	}
	
	$wrsw_rtud .= " > /dev/null 2>&1 &";
	shell_exec($wrsw_rtud);
	
}

/*
 * It gets the text that the help window should desplay.
 * 
 * @author José Luis Gutiérrez <jlgutierrez@ugr.es>
 * 	
 * 
 */
function wrs_vlan_display(){
	
	$rtu_running=shell_exec('ps aux | grep -c wrsw_rtud_new');
	$rtu_running=(int)$rtu_running;
	
	if($rtu_running >2){
		$vlan_config = shell_exec('cat /tmp/vlan.conf');
		
		echo "<br><table border='1' align='center' class='altrowstable' id='alternatecolor'>";
		echo '<tr><td><b><center>Current Configuration </center></b></td></tr>';
		
		echo '<tr><th><center>'.$vlan_config.'</center></th></tr>';
		echo '</tr></table><br>';
	}
	
}

function wrs_display_help($help_id, $name){
	
	if(!strcmp($help_id, "dashboard")){
		$message = "<p>
				<table border=0 align=center>
					<tr>
						<td><img src='./img/master.png' width='20' ></td>
						<td>The violet connector means that the endpoint is master</td>
					</tr>
					<tr>
						<td><img src='./img/slave.png' width='20'></td>
						<td>The blue connector means that the endpoint is slave</td>
					</tr>
					<tr>
						<td><img src='./img/linkdown.png' width='20'></td>
						<td>The gray connector means that the endpoint is not connected</td>
					</tr>
					<tr>
						<td><img src='./img/unlocked.png' width='20'></td>
						<td>The unlocked icon means that the endpoint is not locked.</td>
					</tr>
					<tr>
						<td><img src='./img/locked.png' width='20'></td>
						<td>The locked icon means that the endpoint is locked.</td>
					</tr>
					<tr>
						<td><img src='./img/check.png' width='20'></td>
						<td>The green check icon means that the endpoint is calibrated.</td>
					</tr>
					<tr>
						<td><img src='./img/uncheck.png' width='20'></td>
						<td>The red cross icon means that the endpoint is not calibrated.</td>
					</tr>
				
				
				</table>
				</p>";
	} else if (!strcmp($help_id, "load")){
		$message = "<p>Loading files: <br>
					- <b>Load FPGA File</b>: Loads a .bin file for the gateware on the FPGA.<br>
					- <b>Load LM32 File</b>: Loads a .bin file into the lm32 processor.<br>
					</p>";
	} else if (!strcmp($help_id, "endpoint")){
		$message = "<p>It is used to configure each point of the switch with different parameters as well as wrs_phytool program does. <br>
						First <b>select an enpoint</b>. <br>
						Then, select an option from the list: <br>
							- <b>Enable Calibration Transmission</b>: enables calibration transmission on endpoint<br>
							- <b>Disable Calibration Transmission</b>: disables calibration transmission on endpoint<br>
							- <b>See registers</b>: displays each endpoint registers (R0-R16) <br>
							- <b>Modify registers</b>: used to modify endpoint registers (R0-R16) <br>
							- <b>Lock endpoint</b>: used to lock endpoint <br>
							- <b>Make switch master</b>: Switch works as master <br>
							- <b>Make switch grandmaster</b>: Switch works as grandmaster<br>
							</p>";
	} else if (!strcmp($help_id, "login")){
		$message = "<p>login</p>";
	} else if (!strcmp($help_id, "logout")){
		$message = "<p>logout</p>";
	} else if (!strcmp($help_id, "management")){
		$message = "<p>
			Options: <br>
			- <b>Change mode:</b> Changes switch mode to Master/GrandMaster <br>
			- <b>Reboot switch</b>: Reboots the switch <br>
			- <b>Net-SNMP Server</b>: Enables/Disables remote management using SNMP <br>
			- <b>NTP Server</b>: Sets the IP address of an external NTP server. By default it is configured as UTC, please use the second box to change it. This change is done on the webserver, not in the switch command line environment.<br>
			- <b>Load Configuration Files</b>: You can upload individual configuration files to the switch (ppsi.conf, wrsw_hal.conf, snmp.conf, sfp_database.conf or a .tar.gz file with all of them.<br>
			- <b>Backup Configuration Files</b>: Downloads a tar.gz file with all configuration files of the switch.<br>
			</p>"; 
	} else if (!strcmp($help_id, "ptp")){
		$message = "<p><b>Enable or disable PPSi service. <br>
					<b>Changing Clock CLass and Clock Accuracy fields modifies ppsi.conf file for those values and relanches the service again.</b>. <br></p>";
	} else if (!strcmp($help_id, "console")){
		$message = "<p>This is a switch console emulator windows. Use it as if you were using a ssh session.</p>";
	} else if (!strcmp($help_id, "gateware")){
		
		$msg = shell_exec("/wr/bin/shw_ver -g");
		$msg = explode("\n", $msg);
		for($i=0; $i<5; $i++){
			
			$message .= "<p>".$msg[$i]."<br></p>";
		}
		
	}  else if (!strcmp($help_id, "file")){
		$msg = shell_exec("cat ".$GLOBALS['etcdir'].$name);
		$msg = explode("\n", $msg);
		for($i=0; $i<count($msg); $i++){
			
			$message .= $i.":   ".$msg[$i]."<br>";
		}
		
	} else if (!strcmp($help_id, "endpointmode")){
		$message = "<br><b>Change endpoint mode to master/slave/auto by clicking on one of the items.</b><br>";
		$message .= "<b>It modifies both wrsw_hal.conf and ppsi.conf files</b>";
	} else if (!strcmp($help_id, "snmp")){
		$message = "<p align=left>List of public SNMP OIDs</p><br>";
		$message .= shell_exec("snmpwalk -v1 -c public localhost");
		$message = str_replace("\n","<br>",$message);
		
	} else if (!strcmp($help_id, "vlan")){
		
		$message = "<br><b>Add new VLANs to the WRS</b>";
		$message .= "<br><b>- VID --> VLAN ID in rtud</b>";
		$message .= "<br><b>- FID --> Assign FID to configured VLAN</b>";
		$message .= "<br><b>- DROP --> Enable/Disable drop frames on VLAN</b>";
		$message .= "<br><b>- PRIO --> Sets Priority</b>";
		$message .= "<br><b>- MASK --> Mask for ports belonging to configured VLAN</b>";
		$message .= "<br><br>If you want to assign port to VLANs, please add VLANs first and then click on <strong>Assign Ports to VLANs</strong>. ";
		
	} else if (!strcmp($help_id, "vlanassignment")){
		
		$message = "<br><b>Assign ports to created VLANs</b>";
		$message .= "<br><b>VLANs ID --> VLANs ID already created in rtud</b>";
		$message .= "<br><b>Mode --> Sets mode for the endpoint:</b>";
		$message .= "<br><b><ul><li>Access --> tags untagged frames, drops tagged frames not belinging to configured VLAN</b></li>";
		$message .= "<br><b><li>Trunk --> passes only tagged frames, drops all untagged frames</b></li>";
		$message .= "<br><b><li>Disable --> passess all frames as is</b></li>";
		$message .= "<br><b><li>Unqualified Port --> passess all frames regardless VLAN config</b></li></ul>";
		$message .= "<br><b>Priority --> sets priority for retagging</b>";
		$message .= "<br><b>Mask --> sets untag mask for port</b>";
		
		
	} else if (!strcmp($help_id, "network")){
		$message = "<br><b>Set a DHCP network interface configuration or a static one.</b>";
		$message = "<br><b>If you set a static configuration, you have to define: </b>";
		$message .= "<br><b><ul><li>IP Address --> IP Address of your switch</b></li>";
		$message .= "<br><b><li>Netmask --> Netmask</b></li>";
		$message .= "<br><b><li>Network--> IP Address of your network</b></li>";
		$message .= "<br><b><li>Broadcast --> Broadcast address</b></li>";
		$message .= "<br><b><li>Gateway--> Gateway of the switch network</b></li></ul>";
		$message .= "<br><br><b>NOTE: This network configuration only works for NAND-flashed switches. If you are using a NFS server, the configurtion is set by default in busybox and it is not possible to be changed.</b>";
		
	} else if (!strcmp($help_id, "firmware")){
		$message = "<p>Firmware features: <br>
					- <b>Flash firmware</b>: It flashes a new firmware to the switch. Do it under your own risk.<br>
					</p>";
	}
	
	
	
	
	echo $message;
	
}

/*
 * Obtains the file extension that will be loaded into the switch.
 * 
 * @author José Luis Gutiérrez <jlgutierrez@ugr.es>
 * 	
 * 
 */
function extension($filename){
    return substr(strrchr($filename, '.'), 1);
}

/*
 * Obtains the content of wrsw_hal_file
 *  
 * @author José Luis Gutiérrez <jlgutierrez@ugr.es>
 * 	
 * @return $file: string containing endpoints master/slave
 * 
 */
function parse_wrsw_hal_file(){
	
	$file =  shell_exec('cat '.$GLOBALS['etcdir'].'wrsw_hal.conf | grep wr_');
	$file =  str_replace("mode =", "", $file);
	$file =  str_replace('"', "", $file);
	$file =  str_replace(';', "", $file);
	$file =  str_replace('wr_', "", $file);
	$file = explode(" ", $file);
	return $file;
}

function parse_ppsi_conf_file(){
	
	$file =  shell_exec('cat '.$GLOBALS['etcdir'].'ppsi.conf | grep role');
	$file =  str_replace("role", "", $file);
	$file = explode(" ", $file);
	return $file;
	
}

/*
 * Obtains the content the switch mode from wrsw_hal.conf file
 *  
 * @author José Luis Gutiérrez <jlgutierrez@ugr.es>
 * 	
 * @return true for GrandMaster mode, false for Master mode.
 * 
 */
function check_switch_mode(){
	$status = shell_exec("cat ".$GLOBALS['etcdir']."wrsw_hal.conf | grep -c GrandMaster");
	
	if($status>0){
		return "GrandMaster";
	} else {
		return "Master";
	}

}

/*
 * Obtains the current mode of the switch from wrsw_hal.conf file
 * and changes it from master to Grandmaster and vicecersa
 *  
 * @author José Luis Gutiérrez <jlgutierrez@ugr.es>
 * 	
 * 
 */
function modify_switch_mode(){
	
	if (!strcmp(check_switch_mode(), "GrandMaster")){
		$cmd = 'sed -i "s/mode = \"GrandMaster\"/mode = \"Master\"/g" '.$GLOBALS['etcdir'].'wrsw_hal.conf';
	}else{
		$cmd = 'sed -i "s/mode = \"Master\"/mode = \"GrandMaster\"/g" '.$GLOBALS['etcdir'].'wrsw_hal.conf';
	}
	shell_exec($cmd);
	
}

function wrs_modify_endpoint_mode($endpoint, $mode){
		
	switch ($mode) {
		case "master":
			$new_mode = "slave";
			break;
		case "slave":
			$new_mode = "auto";
			break;
		case "auto":
			$new_mode = "master";
			break;
	}
	
	
	
	// ### Code for ppsi.conf ###
	if($endpoint>0 && $endpoint<19 && (((!strcmp($mode, "slave"))) || ((!strcmp($mode, "master"))) || ((!strcmp($mode, "auto"))))){
		
		
		$file = $GLOBALS['etcdir'].$GLOBALS['ppsiconf']; 
		$lines = file($file);
		$output = "";
		$count = 0;
		$end_aux = $endpoint;
		
		if($endpoint == 1){ //Slave port
			$endpoint= "port slave";
		}else{ //Master ports
			$endpoint= "port master".($end_aux-1)."\n";
			
		}
		
		$found = false;
		
		foreach($lines as $line_num => $line)
		{
			if(substr_count($line, $endpoint)>0){
				$found = true;
			}
			
			if($found)$count++;
			
			
			if($count==3){
				$output.='role '.$new_mode."\n";
				$count = 0;
				$found = false;
			}else{
				$output.=$line;
			}
		}
		
		//We save the changes in a temporarely file in /tmp
		$file = fopen("/tmp/".$GLOBALS['ppsiconf'],"w+");
		fwrite($file,$output);
		fclose($file);
		
		//We move the file to /wr/etc/
		copy('/tmp/'.$GLOBALS['ppsiconf'], $GLOBALS['etcdir'].$GLOBALS['ppsiconf']);
		
		//echo '<br><br><br><br><br><br><br><br><center>Endpoint wr'.$end_aux.' is now '.$new_mode.'</center>';
	}else{
		echo '<br>Wrong parameters for endpoint-mode';
	}
	
	
	
	// ##### old code for wrsw_hal.conf ######
	switch ($mode) {
		case "master":
			$new_mode = "wr_slave";
			break;
		case "slave":
			$new_mode = "wr_master";
			break;
		case "auto":
			$new_mode = "wr_slave";
			break;
	}
	
	echo $mode, "---", $new_mode;
	
	if($endpoint>0 && $endpoint<19 && (((!strcmp($mode, "slave"))) || ((!strcmp($mode, "master"))) || ((!strcmp($mode, "auto"))))){
		
		
		$file = $GLOBALS['etcdir'].$GLOBALS['wrswhalconf'];
		$lines = file($file);
		$output = "";
		$count = 0;
		$end_aux = $endpoint;
		$endpoint = 'wr'.($endpoint-1).' =';
		$found = false;
		
		foreach($lines as $line_num => $line)
		{
			if(substr_count($line, $endpoint)>0){
				$found = true;
			}
			
			if($found)$count++;
			
			
			if($count==6){
				$output.='		mode = "'.$new_mode.'";'."\n";
				$count = 0;
				$found = false;
			}else{
				$output.=$line;
			}
		}
		
		//We save the changes in a temporarely file in /tmp
		$file = fopen("/tmp/".$GLOBALS['wrswhalconf'],"w+");
		fwrite($file,$output);
		fclose($file);
		
		//We move the file to /wr/etc/
		copy('/tmp/'.$GLOBALS['wrswhalconf'], $GLOBALS['etcdir'].$GLOBALS['wrswhalconf']);

	}else{
		echo '<br>Wrong parameters for endpoint-mode';
	}
	
}

function session_is_started(){
	
	ob_start();
	
	$login_url = "./index.php";
	
	if (!isset($_SESSION['myusername'])) {
		echo '<br><br><br><center>Please <a href="' . $login_url . '">login.</center></a>';
		exit;
	}
	
	
}

function parse_mask2ports($vlanmask){
	$vlanmask = str_replace("0x","", $vlanmask);
	$bin = decbin(hexdec($vlanmask)); 
	$bin = strrev($bin);
	$size = strlen($bin);
	$counter = 0;
	$ports = "";
	
	for($i=0; $i<18; $i++){
		if($bin[$i]=="1"){
			$ports .= "wr".($i+1)." "; 
			$counter++;
			if($counter==4){
				$ports .= "<br>";
				$counter = 0;
			}
		}
		
	}
	
	
    return $ports; 
	
}
	
?>
