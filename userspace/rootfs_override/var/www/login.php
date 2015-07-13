<?php include 'functions.php'; include 'head.php'; ?>
<body id="login">
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
<h1 class="title">Login <a href='help.php?help_id=login' onClick='showPopup(this.href);return(false);'><img align=right src="./img/question.png"></a></h1>

	<?php
		ob_start();

		$message="";
		if(count($_POST)>0) {

			//If /etc/phpusers does not exist we create the file and "admin" "" user&pass
			if (!file_exists($GLOBALS['phpusersfile'])) {
				$username = "admin";
				$password = "";
				$salt="wrs4.0salt";
				$pass = $password;
				$hash = md5($pass); // md5 hash #1
				$hash_md5 = md5($salt.$pass); // md5 hash with salt #2
				$hash_md5_double = md5(sha1($salt.$pass)); // md5 hash with salt & sha1 #3
				$output= $username." ".$hash_md5_double."\n";
				$file = fopen($GLOBALS['phpusersfile'],"w+");
				fwrite($file,$output);
				fclose($file);
			}

			$username = $_POST["login"];
			$password = $_POST["password"];
			$saved_hash = shell_exec("cat ".$GLOBALS['phpusersfile']." | grep '".$username."' | awk '{print $2}'");
			$saved_user = shell_exec("cat ".$GLOBALS['phpusersfile']." | grep '".$username."' | awk '{print $1}'");
			$saved_user = preg_replace('/\s+/', '', $saved_user);
			$saved_hash = str_replace("\n","",$saved_hash);
			$user_exists = shell_exec("cat ".$GLOBALS['phpusersfile']." | grep -c ".$username);

			$salt="wrs4.0salt";
			$pass = $password;
			$hash = md5($pass); // md5 hash #1
			$hash_md5 = md5($salt.$pass); // md5 hash with salt #2
			$hash_md5_double = md5(sha1($salt.$pass)); // md5 hash with salt & sha1 #3

			if (!strcmp($hash_md5_double,$saved_hash) && $user_exists>0 && (strcmp($saved_user, $username) == 0)){
				session_start();
				$_SESSION["myusername"] = $username;

				echo 'Logged in as '.$_SESSION["myusername"];
				header('Location: index.php');
			}else{
				echo 'Invalid Username or Password';
			}

		}
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
