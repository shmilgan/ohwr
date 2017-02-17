<?php

/*
 * Send wr dynamic information
 *
 * @author Anne M. <anne@sevensols.com>
 *
 */

include 'json.php';

$data = array();

#Obtain the wr info
$wrinfo=shell_exec("/wr/bin/wr_mon -w  | head -19 | tail -18");
$wrinfo = explode("\n", $wrinfo);
array_push($data, $wrinfo);

#Obtain the temperatures using the last line of (wr-mon -w)
$temperatures = shell_exec("/wr/bin/wr_mon -w | tail -1");
$temperatures = explode(" ",$temperatures);
array_push($data, $temperatures);

#Obtain wr date
$wr_date = shell_exec("/wr/bin/wr_date -n get |tail -2");
$wr_date = explode("\n",$wr_date);
array_push($data, $wr_date);

$ports = shell_exec("/wr/bin/wr_mon -w | tail -2 | head -1");
$ports = explode(" ", $ports);
array_push($data, $ports);

echo __json_encode($data);

?>
