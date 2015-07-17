<?php
session_start();
if (!isset($_SESSION['myusername'])) {

echo '<ul>
	<li><a href="index.php" '.echoSelectedClassIfRequestMatches("index").'> Dashboard </a></li>
	<li><a href="contact.php" '.echoSelectedClassIfRequestMatches("contact").'> About </a></li>
</ul>';

}else{

echo '<ul>
	<li '.echoSelectedClassIfRequestMatches("index").'><a href="index.php"> Dashboard </a></li>
	<li '.echoSelectedClassIfRequestMatches("ptp").'><a href="ptp.php"> WR-PPSi Setup </a></li>
	<li '.echoSelectedClassIfRequestMatches("vlan").'><a href="vlan.php"> VLAN Setup </a></li>
	<li '.echoSelectedClassIfRequestMatches("endpointmode").'><a href="endpointmode.php">  Endpoint Mode</a></li>
	<li '.echoSelectedClassIfRequestMatches("management").'><a href="management.php"> Switch Management </a></li>
	<li '.echoSelectedClassIfRequestMatches("contact").'><a href="contact.php"> About </a></li>

</ul>';


}
?>
