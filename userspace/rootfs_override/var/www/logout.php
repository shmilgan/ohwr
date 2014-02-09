<?php include 'functions.php'; include 'head.php'; ?>
<body>
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
<h1 class="title">Dashboard <a href='help.php?help_id=logout' onClick='showPopup(this.href);return(false);'><img align=right src="./img/question.png"></a></h1>

<?php $_SESSION['advance']=""; ?>

	<?php
		//ob_start();

		unset($_SESSION["myusername"]);
		unset($_SESSION["mypassword"]);
		echo '<br><br><center><h3>Logged out</h3></center>';
		header('Location: index.php');
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
