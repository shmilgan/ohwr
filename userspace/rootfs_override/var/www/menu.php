<ul>
	<li><a href="index.php"> Dashboard </a></li> 
	<?php if(file_exists('/wr/bin/wrsw_rtud_new')) {echo '<li><a href="vlan.php"> VLAN Configuration </a></li>';}?>
	<li><a href="ptp.php"> PTP Configuration </a></li>
	<li><a href="endpoint.php"> Endpoint Configuration </a></li>
	<li><a href="management.php"> Switch Management </a></li>
	<li><a href="load.php"> LM32 & FPGA </a></li>
	<li><a href="terminal.php"> Console </a></li>
</ul>




