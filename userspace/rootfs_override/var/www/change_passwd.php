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
<h1 class="title">User Administration</h1>

	<?php session_is_started() ?>
	<?php $_SESSION['advance']=""; ?>

	<table class="altrowstable" id="alternatecolor" border="0" align="center">	

			<form method="post">
			<tr><td>Username: </td><td><INPUT type="text" name="user" value="root" readonly></td></tr>
			<tr><td>Old Password: </td><td><INPUT type="password" name="oldpasswd" > </td></tr>
			<tr><td>New Password: </td><td> <INPUT type="password" name="newpasswd" > </td></tr>
			<tr><td>Confirm Password: </td><td><INPUT type="password" name="confirmpasswd" > </td></tr>
			<tr><td></td><td  align="center"><input type="submit" value="Change Password" class="btn"></td></tr>
			</form>

	</table>

	<?php
		//Change user password

		if(!(!empty($_POST["oldpasswd"]) || !empty($_POST["newpasswd"]) || !empty($_POST["confirmpasswd"]))){
			echo '<br><br><p align="center">*Please fill all fields.</p>';	
		}else{

			$username = $_POST["user"];
			$oldpassword = $_POST["oldpasswd"];
			$newpasswd = $_POST["newpasswd"];
			$confirmpasswd = $_POST["confirmpasswd"];

			/* Changing the password from the web interface will always save
			 * the password encrypted for security reasons...
			 * */
			if(!empty($_SESSION['KCONFIG']['CONFIG_ROOT_PWD_IS_ENCRYPTED'])){
				/* Previous password was encrypted */
				/* password shall be here: ROOT_PWD_CYPHER */
				$dotconfig_passwd = $_SESSION['KCONFIG']['CONFIG_ROOT_PWD_CYPHER'];
				$salt = get_encrypt_salt($dotconfig_passwd);
				$method = get_encrypt_method($dotconfig_passwd);
				$rounds = get_encrypt_rounds($dotconfig_passwd);
				$oldpassword = encrypt_password($oldpassword, $salt, $rounds, $method);
			}else{
				/* previous password was not encrypted */
				/* password shall be here: ROOT_PWD_CLEAR */
				$dotconfig_old_passwd = $_SESSION['KCONFIG']['CONFIG_ROOT_PWD_CLEAR'];
			}

			if(!strcmp($newpasswd,$confirmpasswd)==0){
				echo '<br><br><div id="alert" align="center">New and confirm password are different.</div>';
				exit;
			}else{
				$method = "CRYPT_MD5";
				$rounds = "";
				$salt = substr(substr( "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ" ,
							mt_rand( 0 ,50 ) ,1 ) .substr( md5( time() ), 1), 4, 8);
				$newpasswd = encrypt_password($newpasswd, $salt, $rounds, $method);
			}

			if(strcmp($newpasswd,"")==0){ /* using mkpasswd it can never be NULL */
				echo '<br><br><div id="alert" align="center">Something went wrong.</div>';
				exit;
			}

			if(!strcmp($dotconfig_passwd,$oldpassword)==0){
				echo '<br><br><div id="alert" align="center">Old password was not correct.</div>';
				exit;
			}else{ /* Save to dotconfig... */
				if(!empty($_SESSION['KCONFIG']['CONFIG_ROOT_PWD_IS_ENCRYPTED'])){
					$_SESSION['KCONFIG']['CONFIG_ROOT_PWD_CYPHER'] = $newpasswd;
				}else{ /* previous was not encrypted */
					$_SESSION['KCONFIG']['CONFIG_ROOT_PWD_IS_ENCRYPTED']="y";
					$_SESSION['KCONFIG']['CONFIG_ROOT_PWD_CYPHER']=$newpasswd;
					check_add_existing_kconfig("CONFIG_ROOT_PWD_IS_ENCRYPTED=");
					check_add_existing_kconfig("CONFIG_ROOT_PWD_CYPHER=");
					delete_from_kconfig("CONFIG_ROOT_PWD_CLEAR=");
				}
				save_kconfig();
				apply_kconfig();
				load_kconfig();
				header('Location: logout.php');
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
