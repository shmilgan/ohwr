<ul>
	<li><a href="index.php"> Dashboard </a></li> 


<?php 
//session_start();
if (!isset($_SESSION['myusername'])) {

echo '</ul><br><hr>';
echo '<div class="login">
      <h3>Login</h3>
      <form method="post" action="login.php">
        <input type="text" name="login" value="" placeholder="Username" size="15">
        <input type="password" name="password" value="" placeholder="Password" size="15">
        <!--<p class="remember_me">
          <label>
            <input type="checkbox" name="remember_me" id="remember_me">
            Remember me on this computer
          </label>
        </p>-->
        <p class="submit"><input type="submit" name="commit" value="Login" class="btn"></p>
      </form>
</div>';
}else{
	
	//The rest of the menu for logged users
	echo '<li><a href="ptp.php"> PPSi Setup </a></li>';
	echo '<li><a href="endpointmode.php">  Endpoint Mode</a></li>';
	echo '<li><a href="vlan.php"> VLAN Setup </a></li>';
	echo '<li><a href="management.php"> Switch Management </a></li>';
	echo '<li><a href="advance.php"> Advanced Mode </a></li>';
	if(!empty($_SESSION['advance'])){
		echo '<ul class="advance">';
		echo '<li><a href="sfpcalibration.php">SFP Calibration</a></li>';
		echo '<li><a href="endpoint.php">Endpoint Tool </a></li>';
		echo '<li><a href="endpointcalibration.php">Endpoint Calibration</a></li>';
		echo '<li><a href="load.php">LM32 & FPGA</a></li>';
		echo '<li><a href="terminal.php">Virtual Console</a></li>';
		echo '<li><a href="firmware.php">Firmware</a></li>';
		echo '</ul>';
	}
	echo '</ul><br><hr>';
	
	echo '<b>User: <font color="blue"><a href="change_passwd.php">'.$_SESSION["myusername"].'</a> </font></b>';
	echo '<a href="./logout.php">(logout)</a>';
}
?>
<hr>




