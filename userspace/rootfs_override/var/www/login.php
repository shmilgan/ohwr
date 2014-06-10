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
			$users = shell_exec('cat /etc/users');
			$users = explode(" ", $users);
			$username = $_POST["login"];
			$password = $_POST["password"];
			if ((!strcmp($username, "root")) && (!strcmp($password, ""))){
				session_start(); 
				$_SESSION["myusername"] = $username;
				
				
				echo 'Logged in as '.$_SESSION["myusername"];
				header('Location: index.php');
			}else{
				echo 'Invalid Username or Password';
			}

		}
			//if(isset($_SESSION["user_id"])) {
			//header("Location:index.php");
		
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
