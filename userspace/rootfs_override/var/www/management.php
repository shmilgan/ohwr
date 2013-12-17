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
<h1 class="title">Switch Management <a href='help.php?help_id=management' onClick='showPopup(this.href);return(false);'><img align=right src="./img/question.png"></a></h1>

	<table border="0" align="center">	
		<tr>
			<form  method="post">
			<th><center>Halt system: </center><input type="hidden" name="cmd" value="halt"></th>
			<th><input type="submit" value="Halt switch" class="btn"></th>
			</form>
		</tr>
		<tr>
			<form  method="post">
			<th><center>Reboot system: </center><input type="hidden" name="cmd" value="reboot"></th>
			<th><input type="submit" value="Reboot switch" class="btn"></th>
			</form>
		</tr>
		</tr><th> </th><th> </th><tr></tr><th> </th><th> </th><tr>
		<tr>
			</form>
			<form method="post">
			<th><center>Mount partition as writable: </center><input type="hidden" name="cmd" value="rw"></th>
			<th><input type="submit" value="Remount"  class="btn"></th>
			</form>
		</tr>
		<tr>
			</form>
			<form method="post">
			<th><center>Mount partition as read-only: </center><input type="hidden" name="cmd" value="ro"></th>
			<th><input type="submit" value="Remount" class="btn"></th>
			</form>
		</tr>
		</tr><th> </th><th> </th><tr></tr><th> </th><th> </th><tr>
		<tr>
			<form method="post">
			<th>Change PHP File Size Upload: <INPUT type="text" name="size" > </th>
			<input type="hidden" name="cmd" value="size">
			<th><input type="submit" value="Change" class="btn"></th>
			</form>
		</tr>
	</table>
	
	
	
	
	<?php 
		wrs_management();
	
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
