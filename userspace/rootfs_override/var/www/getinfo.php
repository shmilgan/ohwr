<?php

/*
 * Send dynamic information
 *
 * @author Anne M. <anne@sevensols.com>
 *
 */

include 'json.php';
include 'functionsget.php';

$data = array();

array_push($data, getTemperatures(1));
array_push($data, getWrDate());
array_push($data, getTablePortStatus());
array_push($data, getTiming());

echo __json_encode($data);

?>
