<?php
session_start();
if (!isset($_SESSION['myusername'])) {

echo '<ul>
	<li><a href="index.php"> Dashboard </a></li>
	<li><a href="contact.php"> Contact Us </a></li>
</ul>';

}else{

echo '<ul>
	<li><a href="index.php"> Dashboard </a></li>
	<li><a href="ptp.php"> PTP Configuration </a></li>
	<li><a href="endpoint.php"> Endpoint Configuration </a></li>
	<li><a href="endpointmode.php">  Endpoint Mode</a></li>
	<li><a href="management.php"> Switch Management </a></li>
	<li><a href="contact.php"> Contact Us </a></li>
</ul>';
	
	
}

?>
