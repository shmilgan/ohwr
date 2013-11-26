<?php

function wrs_main_info(){
	
	echo "<table border='1' align='left'>";



	echo '<tr><h1><center>Switch Information</center></h1></tr>';
	echo '<tr><th><b>Hostname:</b></th><th>'; $str = shell_exec("uname -n"); echo $str; echo '</th></tr>';
	echo '<tr><th> <b>IP Address:</b> </th><th>'; $ip = shell_exec("ifconfig eth0 | grep 'inet addr:' | cut -d: -f2 | awk '{ print $1}'"); echo $ip;  echo '</th></tr>';
	echo '<tr><th> <b>OS Release:</b> </th><th>'; $str = shell_exec("uname -r"); echo $str; echo '</th></tr>';
	echo '<tr><th> <b>OS name:</b> </th><th>'; $str = shell_exec("uname -s"); echo $str; echo '</th></tr>';
	echo '<tr><th> <b>OS Version:</b> </th><th>'; $str = shell_exec("uname -v"); echo $str; echo '</th></tr>';
	echo '<tr><th> <b>PCB Version:</b> </th><th>'; $str = shell_exec("/wr/bin/shw_ver -p"); echo $str;  echo '</th></tr>';
	echo '<tr><th> <b>FPGA:</b> </th><th>'; $str = shell_exec("/wr/bin/shw_ver -f"); echo $str; echo '</th></tr>';
	echo '<tr><th> <b>Compiling time:</b> </th><th>'; $str = shell_exec("/wr/bin/shw_ver -c"); echo $str; echo '</th></tr>';
	
	echo '</table>';

}
	
?>
