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
<h1 class="title">Firmware Management <a href='help.php?help_id=firmware' onClick='showPopup(this.href);return(false);'><img align=right src="./img/question.png"></a></h1>

	<?php session_is_started() ?>
	<?php $_SESSION['advance']="";?>

	<table border="0" align="center">	
		<tr>
			<FORM method="POST" ENCTYPE="multipart/form-data" onsubmit="return confirm('Are you sure you want to upload and flash a new firmware?');">
			<th ><INPUT type=file name="file" ></th>
			<th><INPUT type=submit value="Flash Firmware" class="btn" ><INPUT type=hidden name=MAX_FILE_SIZE  VALUE=<?php echo wrs_php_filesize();?>000></th>
			</FORM>
		</tr><tr></tr><tr></tr><tr>
			<FORM method="POST" ENCTYPE="multipart/form-data" >
			<th align=center>Download a backup of the entire Switch</th>
			<th><INPUT type=submit value="Backup Firmware" class="btn"><input type="hidden" name="cmd" value="backup-wrs"></th>
			</FORM>
		</tr>
	</table>
	
	
	<?php 
		wrs_change_wrfs("rw");
		wrs_management();
		wrs_change_wrfs("ro");
	
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
