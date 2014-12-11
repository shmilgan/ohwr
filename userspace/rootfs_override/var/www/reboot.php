<?php include 'functions.php'; include 'head.php'; ?>
<body id="management">
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

	<?php session_is_started() ?>
	
	<div id="rebootmsg">
		<img src="./img/loader.gif">
		<div id="rebootingtext">...Saving changes...</div>
	</div>
	
	<div id="rebooting"></div>
	
	<div id="rebootwrlogo">
		<img alt="open clipart http://www.ryanlerch.org/" src="./img/ryanlerch_The_White_Rabbit.png">
		<p>Alice: How long is forever?<br>White Rabbit: Sometimes, just a nanosecond.</p>
	</div>
	

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
