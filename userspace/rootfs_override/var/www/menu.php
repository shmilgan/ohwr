<ul>
	<li><a href="index.php"> Dashboard </a></li> 


<?php 
//session_start();
if (!isset($_SESSION['myusername'])) {

echo '</ul><br><hr>';
echo '<div class="login">
      <h3>Login</h3>
      <form method="post" action="login.php">
        <p><input type="text" name="login" value="" placeholder="Username"></p>
        <p><input type="password" name="password" value="" placeholder="Password"></p>
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
	if(file_exists('/wr/bin/wrsw_rtud_new')) {echo '<li><a href="vlan.php"> VLAN Configuration </a></li>';}
	echo '<li><a href="ptp.php"> PTP Configuration </a></li>';
	echo '<li><a href="endpointmode.php">  Endpoint Mode</a></li>';
	echo '<li><a href="endpoint.php"> Endpoint Tool </a></li>';
	echo '<li><a href="load.php"> LM32 & FPGA </a></li>';
	echo '<li><a href="management.php"> Switch Management </a></li>';
	//echo '<li><a href="administration.php"> Switch Administration </a></li>';
	echo '<li><a href="terminal.php"> Console </a></li>';
	echo '</ul><br><hr>';
	
	echo '<b>User: <font color="blue">'.$_SESSION["myusername"].' </font></b>';
	echo '<a href="./logout.php">(logout)</a>';
}
?>
<hr>




