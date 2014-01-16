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
<h1 class="title">Load LM32 & FPGA Files <a href='help.php?help_id=load' onClick='showPopup(this.href);return(false);'><img align=right src="./img/question.png"></a></h1>

	<?php session_is_started() ?>

	<table border="0" align="center">	
	
	<tr>
		<th><FORM method="POST" ENCTYPE="multipart/form-data" >
								  <INPUT type=file name="fpgafile"  >
								  <INPUT type=submit Value="Load FPGA bin" class="btn" >
								  <INPUT type=hidden name=MAX_FILE_SIZE  VALUE=<?php wrs_php_filesize();?>000>
		</FORM></th>
	</tr>
	<tr>
		 <th ><FORM method="POST" ENCTYPE="multipart/form-data">
								 <INPUT type=file name="lm32file" >
								  <INPUT type=submit value="Load lm32 bin" class="btn">
								  <INPUT type=hidden name=MAX_FILE_SIZE  VALUE=<?php wrs_php_filesize();?>000>
		</FORM></th>
	</tr>
	<tr>
		<th ><FORM method="POST" ENCTYPE="multipart/form-data">
								  <INPUT type=file name="file" >
								  <INPUT type=submit value="Load firmware" class="btn" >
								  <INPUT type=hidden name=MAX_FILE_SIZE  VALUE= <?php wrs_php_filesize();?>000>
		</FORM></th>
	</tr>
	</table>
	
	<br><br><br><center>Max. filesize is now <?php echo shell_exec("cat /etc/php.ini | grep upload_max_filesize | awk '{print $3}'"); 
			?></center>
			<form method="post">
			Change PHP File Size Upload: <INPUT type="text" name="size" > 
			<input type="submit" value="Change" class="btn">
			</form>
	
	
	<?  
		wrs_load_files();

		echo '<center>';
			wrs_check_writeable();
		echo '</center>';
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
