<?php

function wrs_header_ports(){
		
	$cmd = wrs_env_sh();
	$str = shell_exec($cmd." ports");
	$ports = explode(" ", $str);

	//echo "<a href='index.php'><IMG SRC='img/wr_logo.png' align=left , vspace=10 , width=80 , hight=100 , border=0 , alt='White Rabbit'></a>";
	
	//echo '<p>&nbsp</p>';

	echo "<table border='0' align='center'  vspace='1'>";
	echo '<tr><th><h1 align=center>White-Rabbit Switch Manager</h1></th></tr>';
	echo '</table>';
	echo "<table border='0' align='center' vspace='15'>";
	
	echo '<tr>';
	for($i=1; $i<18*4; $i=$i+4){
		
		if (strstr($ports[($i-1)],"up")){
			if (strcmp($ports[($i)],"Master")){
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
	//echo "<a href='http://www.ohwr.org/projects/wr-switch-sw'><IMG SRC='img/ohr.png' align=right , width=70 , hight=100 , border=0 , alt='OHR'></a>";
	echo '</tr>';
	
	echo '</table>';

}


function wrs_main_info(){
	
	//echo '<br>';
	echo "<table border='1' align='left' class='altrowstable' id='alternatecolor'>";

	//echo '<tr><h1><center>Switch Information</center></h1></tr>';
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
	echo '<tr><th> <b>Max. Filesize Upload: </b></th><th><center>'; echo shell_exec("cat /etc/php.ini | grep upload_max_filesize | awk '{print $3}'"); echo '</center></th></tr>';
	
	echo '</table>';
	
	

}

function wrs_switch_status_info(){
	
	echo '<br>';
	echo "<table border='1' align='left'>";

	echo '<tr><h1><center>Switch Status</center></h1></tr>';
	echo '<tr><th> <b>DHCP Server:</b></th><th>'; $str = shell_exec("/wr/bin/wr_management dhcp status"); echo $str; echo '</th></tr>';
	echo '<tr><th> <b>Switch Fans:</b> </th><th>'; $str = shell_exec("/wr/bin/wr_management fan status"); echo $str; echo '</th></tr>';
	echo '<tr><th> <b>PTP:</b> </th><th>';  echo wrs_check_ptp_status() ? 'On' : 'Off'; echo '</th></tr>';
	echo '<tr><th> <b>White-Rabbit Date:</b></th><th>'; $str = shell_exec("/wr/bin/wr_date get"); echo $str; echo '</th></tr>';
	echo '<tr><th> <b>Max. Filesize Upload: </b></th><th>'; echo shell_exec("cat /etc/php.ini | grep upload_max_filesize | awk '{print $3}'"); echo '</th></tr>';
	
	echo '</table>';
	
}

function wrs_check_writeable(){

	$output = shell_exec('mount | grep "(ro,"');
	echo (!isset($output) || trim($output) == "") ? "<br>WRS mounted as rw" : "<br><font color='red'>WARNING: WRS is mounted as ro, please contact the maintainer</font>";

}

function wrs_check_ptp_status(){
	$output = intval(shell_exec("ps | grep -c ptpd"));
	return ($output>2) ? 1 : 0;
	//$output = shell_exec('/wr/bin/wr_management ptp status');
	//return $output;
}

function php_file_transfer_size($size){
	
	$size=trim($size);
	
	$prev_size = shell_exec("cat /etc/php.ini | grep upload_max_filesize | awk '{print $3}'");
	$prev_size=trim($prev_size);
	$cmd = "sed -i 's/upload_max_filesize = ".$prev_size."/upload_max_filesize = ".$size."M/g' /etc/php.ini";
	shell_exec($cmd);
	
	$prev_size = shell_exec("cat /etc/php.ini | grep post_max_size | awk '{print $3}'");
	$prev_size=trim($prev_size);
	$cmd ="sed -i 's/post_max_size = ".$prev_size."/post_max_size = ".$size."M/g' /etc/php.ini";
	shell_exec($cmd);
	
	echo 'File upload size changed to '.$size;	
}

function wr_endpoint_phytool($option1, $endpoint){
	
	$cont=0;
	
	if(!strcmp($option1, "dump")){
	
		$output=shell_exec("/wr/bin/wr_phytool ".$endpoint." dump");
		$ports = explode(" ", $output);
		
		//echo '<form  method=POST>';
		echo "<table border='0' align='center'>";
		echo '<tr>';
		echo '<th>'.$endpoint.' Register</th>';
		echo '<th>Value</th>';
		echo '</tr>';
		
		for($i=7; $i<20*2; $i=$i+2){
			echo '<tr>';
			echo '<th>R'.$cont.'</th>';
			echo '<th>'.substr($ports[($i)],0,10).'</th>';
			//echo '<th><input type="text" name="r'.$cont.'" value="'.substr($ports[($i)],0,10).'"></th>';
			//echo '<input type="hidden" name="update" value="yes">';
			echo '</tr>';
			$cont++;
		}
		
		echo '</tr>';
		echo '</table>';
		//echo '<input type="submit" value="Update">	</form>';
		
		if (!strcmp($_POST['update'], "yes")){
			echo 'aki stamos!';
		}
		
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
		
	} else if (!strcmp($option1, "txcal1")){
		$output=shell_exec('/wr/bin/wr_phytool '.$endpoint.' txcal 1');
		echo $endpoint.' is now transmitting calibration';
	} else if(!strcmp($option1, "txcal0")){
		$output=shell_exec('/wr/bin/wr_phytool '.$endpoint.' txcal 0');
		echo $endpoint.' stopped transmitting calibration';
				
	} else if(!strcmp($option1, "lock")){
		$output=shell_exec('/wr/bin/wr_phytool '.$endpoint.' rt lock');
		echo 'Locking finished';
		
	} else if(!strcmp($option1, "master")){
		$output=shell_exec('/wr/bin/wr_phytool '.$endpoint.' rt master');
		echo 'Mastering finished' ;
		
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

function wrs_php_filesize(){
	
	 $size=shell_exec("cat /etc/php.ini | grep upload_max_filesize | awk '{print $3}'"); 
	 $size=substr($size, 0, -2);
	 echo $size;
	
}

function wrs_load_files(){
		
		if (!empty($_FILES['fpgafile']['name'])){
			$uploaddir = '/tmp/';
			$uploadfile = $uploaddir . basename($_FILES['fpgafile']['name']);
			echo '<pre>';
			if (move_uploaded_file($_FILES['fpgafile']['tmp_name'], $uploadfile)) {
				echo "File is valid, and was successfully uploaded.\n";
			} 

			print "</pre>";
			
			echo 'Loading FPGA binary '.$_FILES['fpgafile']['name'].', please wait for the system to reboot';
			$str = shell_exec("/wr/bin/load-virtex ".$uploadfile); 
			echo $str;
			
			
		} else if (!empty($_FILES['lm32file']['name'])){
			$uploaddir = '/tmp/';
			$uploadfile = $uploaddir . basename($_FILES['lm32file']['name']);
			echo '<pre>';
			if (move_uploaded_file($_FILES['lm32file']['tmp_name'], $uploadfile)) {
				echo "File is valid, and was successfully uploaded.\n";
			} 

			print "</pre>";
			
			echo 'Loading lm32 binary '.$_FILES['lm32file']['name'].',, please wait for the system to reboot';
			$str = shell_exec("/wr/bin/load-lm32 ".$uploadfile); 
			echo $str;
		
		} else if (!empty($_FILES['file']['name'])){
			$uploaddir = '/wr/lib/firmware/';
			$uploadfile = $uploaddir . basename($_FILES['file']['name']);
			echo '<pre>';
			if (move_uploaded_file($_FILES['file']['tmp_name'], $uploadfile)) {
				echo "File is valid, and was successfully uploaded to firmware folder\n";
			} 

			print "</pre>";
		
		}

	
	
}

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
			echo '<br><br><br>Partition mounted as rw';
		}else if (!strcmp($cmd, "ro")){
			$output = shell_exec("/wr/bin/wrfs_mnt.sh ro");
			echo '<br><br><br>Partition mounted as ro';
		}else if (!strcmp($cmd, "size")){
			php_file_transfer_size(htmlspecialchars($_POST["size"]));
		}
}

function wrs_env_sh(){

	$output = shell_exec("echo $WRS_MANAGEMENT");
	if(file_exists($output)){
		$sh=$output;
	}else{
		$sh="/wr/bin/wr_management";
	}
	return $sh;

}
	
?>
