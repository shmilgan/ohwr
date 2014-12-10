<?php include 'functions.php'; include 'head.php'; ?>
<body id="advance">
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
<h1 class="title">Switch Management <a href='help.php?help_id=management' onClick='showPopup(this.href);return(false);'><img align=right src="./img/question.png"></a></h1>

<?php session_is_started() ?>

	
	<?php 
		$_SESSION['advance']="yes";
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
