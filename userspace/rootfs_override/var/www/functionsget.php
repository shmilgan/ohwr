<?php

/*
 * Obtain info from wr_mon
 *
 * @author Anne M. <anne@sevensols.com>
 *
 */

include 'functions.php';

#Save info to temp file so we dont have to execute command at each call
function createTempDataFile(){
	shell_exec("/wr/bin/wr_mon -w > /tmp/wrinfo"); 
}

createTempDataFile();

#Obtain the temperatures 
function getTemperatures($sub_id = -1){
	$temperatures=shell_exec("cat /tmp/wrinfo | grep TEMP");
	$temperatures = split(" ", $temperatures);
	if($sub_id != -1){
		$temperatures = $temperatures[$sub_id];
	}
	return $temperatures;
}

#Obtain the wr servo timing
function getTiming(){
	$ports = shell_exec("cat /tmp/wrinfo | grep SERVO");
        $ports = explode(" ", $ports);
        $txt = $ports;
        return $txt;
}

#Obtain wr date
function getWrDate(){
	$wr_date =  str_replace("\n","<br>", shell_exec("/wr/bin/wr_date -n get |tail -2"));
	return $wr_date;
}

#Obtain the wr info as html table
function getTablePortStatus(){
	ob_start();
        draw_table();
        $sfp = ob_get_contents();
        ob_end_clean();
	return $sfp;
}

#Obtain the wr info
function getPortStatus(){
	$wrinfo=shell_exec("cat /tmp/wrinfo  | grep wri");
	$wrinfo = explode("\n", $wrinfo);
	return $wrinfo;
}

?>
