<?php

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
	$cmd = wrs_env_sh();
	$str = shell_exec($cmd." ports");
	$ports = explode(" ", $str);

	// We parse and show the information comming from each endpoint.
	echo "<table border='0' align='center'  vspace='1'>";
	echo '<tr><th><h1 align=center>White-Rabbit Switch Manager</h1></th></tr>';
	echo '</table>';
	echo "<table border='0' align='center' vspace='15'>";
	
	echo '<tr>';
	for($i=1; $i<18*4; $i=$i+4){
		
		if (strstr($ports[($i-1)],"up")){
			if (!strcmp($ports[($i)],"Master")){
				echo '<th>'."<IMG SRC='img/master.png' align=left ,   width=40 , hight=40 , border=0 , alt='master'>".'</th>';
			}else{
				echo  '<th>'."<IMG SRC='img/slave.png' align=left ,  width=40 , hight=40 , border=0 , alt='slave'>".'</th>';
			}
		}else{
			echo '<th>'."<IMG SRC='img/linkdown.png' align=left ,  width=40 , hight=40 , border=0 , alt='down'>".'</th>';
		}
		
	}
	echo '</tr>';
	
	echo '<tr>';
	for($i=1; $i<18*4; $i=$i+4){
		
		if (!strstr($ports[($i+1)],"NoLock")){
			echo '<th>'."<IMG SRC='img/locked.png' align=center ,  width=15 , hight=15 , border=0 , alt='locked'>";
		}else{
			echo '<th>'."<IMG SRC='img/unlocked.png' align=center ,  width=15 , hight=15 , border=0 , alt='unlocked'>";
		}
		if (!strstr($ports[($i+2)],"Uncalibrated")){
			echo "<IMG SRC='img/check.png' align=center ,  width=15 , hight=15 , border=0 , alt='check'>".'</th>';
		}else{
			echo "<IMG SRC='img/uncheck.png' align=center ,  width=15 , hight=15 , border=0 , alt='uncheck'>".'</th>';
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
	
	echo "<table border='1' align='left' class='altrowstable' id='alternatecolor'>";
	echo '<tr><td> <b><center>Switch Info </center></b></td></tr>';

	$str = shell_exec("uname -n");
	if(strcmp($str,"(none)")) shell_exec("/bin/busybox hostname -F /etc/hostname");
	echo '<tr><th><b>Hostname:</b></th><th><center>'; $str = shell_exec("uname -n"); echo $str; echo '</center></th></tr>';
	echo '<tr><th> <b>IP Address:</b> </th><th><center>'; $ip = shell_exec("ifconfig eth0 | grep 'inet addr:' | cut -d: -f2 | awk '{ print $1}'"); echo $ip;  echo '</center></th></tr>';
	echo '<tr><th> <b>OS Release:</b> </th><th><center>'; $str = shell_exec("uname -r"); echo $str; echo '</center></th></tr>';
	echo '<tr><th> <b>OS name:</b> </th><th><center>'; $str = shell_exec("uname -s"); echo $str; echo '</center></th></tr>';
	echo '<tr><th> <b>OS Version:</b> </th><th><center>'; $str = shell_exec("uname -v"); echo $str; echo '</center></th></tr>';
	echo '<tr><th> <b>PCB Version:</b> </th><th><center>'; $str = shell_exec("/wr/bin/shw_ver -p"); echo $str;  echo '</center></th></tr>';
	echo '<tr><th> <b>FPGA:</b> </th><th><center>'; $str = shell_exec("/wr/bin/shw_ver -f"); echo $str; echo '</center></th></tr>';
	echo '<tr><th> <b>Compiling time:</b> </th><th><center>'; $str = shell_exec("/wr/bin/shw_ver -c"); echo $str; echo '</center></th></tr>';
	echo '<tr><th> <b>White-Rabbit Date:</b></th><th><center>'; $str = shell_exec("/wr/bin/wr_date get"); echo $str; echo '</center></th></tr>';
	echo '<tr><th> <b>PTP:</b> </th><th><center>';  echo wrs_check_ptp_status() ? 'On' : 'Off'; echo '</center></th></tr>';
	echo '<tr><th> <b>Max. Filesize Upload: </b></th><th><center>'; echo shell_exec("cat /etc/php.ini | grep upload_max_filesize | awk '{print $3}'"); echo '</center></th></tr>';
	echo '</table>';
	
	

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
	$output = intval(shell_exec("ps | grep -c ptpd"));
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
	
	// We modify fist upload_max_filesize in php.ini
	$prev_size = shell_exec("cat /etc/php.ini | grep upload_max_filesize | awk '{print $3}'");
	$prev_size=trim($prev_size);
	$cmd = "sed -i 's/upload_max_filesize = ".$prev_size."/upload_max_filesize = ".$size."M/g' /etc/php.ini";
	shell_exec($cmd);
	
	// We modify post_max_size in php.ini
	$prev_size = shell_exec("cat /etc/php.ini | grep post_max_size | awk '{print $3}'");
	$prev_size=trim($prev_size);
	$cmd ="sed -i 's/post_max_size = ".$prev_size."/post_max_size = ".$size."M/g' /etc/php.ini";
	shell_exec($cmd);
	
	echo '<p align=center>File upload size changed to '.$size.'</p>';	
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
		
		if (!strcmp($_POST['update'], "yes")){
			echo 'aki stamos!';
		}
	
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
	
	 $size=shell_exec("cat /etc/php.ini | grep upload_max_filesize | awk '{print $3}'"); 
	 $size=substr($size, 0, -2);
	 echo $size;
	
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
				
			}  else {
				echo "<center>File is not valid, please upload a .bin file</center>\n";
			}

			
		// Loading  and copying binary file to /wr/lib/firmware folder on the switch.
		} else if (!empty($_FILES['file']['name'])){
			$uploaddir = '/wr/lib/firmware/';
			$uploadfile = $uploaddir . basename($_FILES['file']['name']);
			echo '<pre>';
			if ((!strcmp(extension($_FILES['file']['name']), "bin")) && move_uploaded_file($_FILES['file']['tmp_name'], $uploadfile)) {
				echo "<center>File is valid, and was successfully uploaded to firmware folder\n";
			}  else {
				echo "<center>File is not valid, please upload a .bin file</center>\n";
			}

			print "</pre>";
		
		} else if (!empty($_POST["size"])){
			php_file_transfer_size(htmlspecialchars($_POST["size"]));
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
		}
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
		echo '<br>Network Interface binded to '.htmlspecialchars($_POST["b"]);
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
	if ((!empty($_POST["daemongroup"])) && (!strcmp(htmlspecialchars($_POST["daemongroup"]),"On"))){
		$cmd = shell_exec("/wr/bin/ptpd -A "); 
		echo '<center>PTPd enabled with default values!</center>';
	}
	if ((!empty($_POST["daemongroup"])) && (!strcmp(htmlspecialchars($_POST["daemongroup"]),"Off"))){
		$cmd = shell_exec("killall ptpd"); 
		echo '<center>PTPd stopped!</center>';
	}
	if(!empty($cmd)){
		shell_exec("killall ptpd"); 
		$ptp_command = "/wr/bin/ptpd -c ".$cmd. "  > /dev/null 2>&1 &";
		$output = shell_exec($ptp_command); 
		echo '<center>PTP initialized.</center>';

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

function wrs_display_help($help_id){
	
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
					- <b>Load firmware</b>: It moves a binary file into the /wr/lib/firmware folder<br>
					- <b>PHP Filesize</b>: It changes the max. size of the files that can be uploaded to the switch (2 MegaBytes by default)<br>
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
							- <b>Make endpoint master</b>: it chages endpoint state to master <br>
							- <b>Make endpoint grandmaster</b>: it chages endpoint state to grandmaster<br>
							</p>";
	} else if (!strcmp($help_id, "login")){
		$message = "<p>login</p>";
	} else if (!strcmp($help_id, "logout")){
		$message = "<p>logout</p>";
	} else if (!strcmp($help_id, "management")){
		$message = "<p>
			Options: <br>
			- <b>Halt switch</b>: shut down switch. <br>
			- <b>Reboot switch</b>: it reboots the switch <br>
			- <b>Mounting partitions</b>: Switch filefolders are read-only by default (except /tmp folder). If you want to make all directories writable use the remount function as writable. <br>
			- <b>PHP Filesize</b>: It changes the max. size of the files that can be uploaded to the switch (2 MegaBytes by default)<br>
		</p>";
	} else if (!strcmp($help_id, "ptp")){
		$message = "<p>The update button is used to stop or run the ptp daemon. It runs with the <b>default options (-A -c)</b> <br>
					<b>If you want to run a ptp daemon with a specific configuration, use the text boxes and the 'Submit Configuration' button</b>. <br></p>";
	} else if (!strcmp($help_id, "vlan")){
		$message = "<p>vlan</p>";
	} else if (!strcmp($help_id, "console")){
		$message = "<p>This is a switch console emulator windows. Use it as if you were using a ssh session.</p>";
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

	
?>
