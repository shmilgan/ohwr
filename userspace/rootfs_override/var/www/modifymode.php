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
<h1 class="title"> Endpoint Mode Configuration <a href='help.php?help_id=dashboard' onClick='showPopup(this.href);return(false);'><img align=right src="./img/question.png"></a></h1>

	<?php session_is_started() ?>
	
	<?php

		$endpoint = $_GET["wr"];
		$mode = $_GET["mode"];
		
		wrs_change_wrfs("rw");
		wrs_modify_endpoint_mode($endpoint, $mode);
		wrs_change_wrfs("ro");
		
	?>
	<hr>
	<FORM align="right" action="endpointmode.php" method="post">
	<INPUT type="submit" value="Back" class="btn">
    </FORM>

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
