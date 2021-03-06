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

			/* User shall always be "root" (by the moment...) */
			load_kconfig();
			$username = $_POST["login"];
			$password = $_POST["password"];
			
			if(!empty($_SESSION['KCONFIG']['CONFIG_ROOT_PWD_IS_ENCRYPTED'])){
				/* password is here: ROOT_PWD_CYPHER */
				$dotconfig_passwd = $_SESSION['KCONFIG']['CONFIG_ROOT_PWD_CYPHER'];
				$salt = get_encrypt_salt($dotconfig_passwd);
				$method = get_encrypt_method($dotconfig_passwd);
				$rounds = get_encrypt_rounds($dotconfig_passwd);
				$password = encrypt_password($password, $salt, $rounds, $method);
			}else{ /* password is here: ROOT_PWD_CLEAR */
				$dotconfig_passwd = $_SESSION['KCONFIG']['CONFIG_ROOT_PWD_CLEAR'];
			}
			
			if ((strcmp($username,"root")==0) && (strcmp($dotconfig_passwd, $password) == 0)){
				session_start();
				$_SESSION["myusername"] = $username;

				echo 'Logged in as '.$_SESSION["myusername"];
				header('Location: index.php');
			}else{
				echo '<div id="alert"><center>Invalid Username or Password</center></div>';
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
