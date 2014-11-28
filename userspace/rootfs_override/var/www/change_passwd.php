<?php include 'functions.php'; include 'head.php'; ?>
<body id="ptp">
<div class="main">
<div class="page">
<div class="header" >
<!--<h1>White-Rabbit Switch Tool</h1>-->
<div class="header-ports" ><?php wrs_header_ports(); ?></div>
<div class="topmenu">
	<?php include 'topmenu.php' ?>
</div>
</div>
<div class="content">
<div class="leftpanel">
<h2>Main Menu</h2>
	<?php include 'menu.php' ?>
</div>
<div class="rightpanel">
<div class="rightbody">
<h1 class="title">User Administration <a href='help.php?help_id=network' onClick='showPopup(this.href);return(false);'><img align=right src="./img/question.png"></a></h1>

	<?php session_is_started() ?>
	<?php $_SESSION['advance']=""; ?>
	
	<table border="0" align="left">	
		
			<form method="post">
			<tr><th>Username: </th><th><INPUT type="text" name="user" value="admin" readonly></th></tr>
			<tr><th>Old Password: </th><th><INPUT type="password" name="oldpasswd" > </th></tr>
			<tr><th>New Password: </th><th> <INPUT type="password" name="newpasswd" > </th></tr>
			<tr><th>Confirm Password: </th><th><INPUT type="password" name="confirmpasswd" > </th></tr>
			<tr><th></th><th  align="center"><input type="submit" value="Change Password" class="btn"></th></tr>
			</form>
		
	</table>
	
	
	<?php 
		//Change user password
		
		$success=false;
		if( empty($_POST['user'])){
			echo '<br><br><br><p align=center>Please fill fields.<br></p>';
				
		}else{
			$saved_hash = shell_exec("cat ".$GLOBALS['phpusersfile']." | grep ".$_POST["user"]." | awk '{print $2}'");
			$saved_hash = str_replace("\n","",$saved_hash);
			
			$username = $_POST["user"];
			$oldpassword = $_POST["oldpasswd"];
			$newpasswd = $_POST["newpasswd"];
			$confirmpasswd = $_POST["confirmpasswd"];
			
			//First confirm old password
			$salt="wrs4.0salt";
			$pass = $oldpassword;
			$hash = md5($pass); // md5 hash #1 
			$hash_md5 = md5($salt.$pass); // md5 hash with salt #2 
			$hash_md5_double = md5(sha1($salt.$pass)); // md5 hash with salt & sha1 #3 
			
			
			if (!strcmp($hash_md5_double, $saved_hash) && !strcmp($newpasswd, $confirmpasswd) && !strcmp($_POST["user"],$_SESSION['myusername'])){ //old password is correct && new and confirm are the same
				//set the new one
				
				$pass = $confirmpasswd;
				$hash = md5($pass); // md5 hash #1 
				$hash_md5 = md5($salt.$pass); // md5 hash with salt #2 
				$hash_md5_double = md5(sha1($salt.$pass)); // md5 hash with salt & sha1 #3 
				
				//Save in file
				//We save the changes in a temporarely file in /tmp
				$old_value=$username." ".$saved_hash;
				$new_value=$username." ".$hash_md5_double;

				$output = shell_exec('cat '.$GLOBALS['phpusersfile'].' | sed -i "s/'.$old_value.'/'.$new_value.'/g" '.$GLOBALS['phpusersfile']); //replace password for the user
				
				//$file = fopen("/etc/phpusers","w+");
				//fwrite($file,$output);
				//fclose($file);
				
				
				
				$success=true;
				echo '<br><br><br><p align=center>Password changed.<br></p>';
			}else{
				$success=false;
				echo '<br><br><br><p align=center>Error changing password.<br></p>';
			}
			
			
		}
		
		if($success) header('Location: logout.php');
	
	?>

</div>
</div>
</div>
<div class="footer">
	<?php include 'footer.php' ?>
</div>
</div>
</div>
</body>
</html>
