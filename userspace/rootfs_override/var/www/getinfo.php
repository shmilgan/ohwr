<?php

/*
 * Send dynamic information
 *
 * @author Anne M. <anne@sevensols.com>
 *
 */

include 'json.php';
include 'functions.php';

$data = array();

#Obtain the temperatures using the last line of (wr-mon -w)
$temperatures=shell_exec("/wr/bin/wr_mon -w | tail -1");
$arr = split(" ", $temperatures);
$temperatures = $arr[1];
array_push($data, $temperatures);

#Obtain wr date
$wr_date =  str_replace("\n","<br>", shell_exec("/wr/bin/wr_date -n get |tail -2"));
array_push($data, $wr_date);

$sfp = updateSfp();
array_push($data, $sfp);

$timing = getTimingParams();
array_push($data, $timing);

echo __json_encode($data);

function getTimingParams(){
	$ports = shell_exec("/wr/bin/wr_mon -w | tail -2 | head -1");
        $ports = explode(" ", $ports);
	$txt = $ports;
	return $txt;
}

function updateSfp(){	
	ob_start();
	draw_table();
	$txt = ob_get_contents();
	ob_end_clean();
	return $txt;
}

?>
