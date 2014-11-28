<?php include 'functions.php'; include 'head.php'; ?>
<body id="load">
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
		<th><FORM method="POST" ENCTYPE="multipart/form-data" onsubmit="return confirm('Are you sure you want to upload a new FPGA binary file? \nThis could result in the malfunction of the switch.');">
								  <INPUT type=file name="fpgafile"  >
								  <INPUT type=submit Value="Load FPGA bin" class="btn" >
								  <INPUT type=hidden name=MAX_FILE_SIZE  VALUE=<?php wrs_php_filesize();?>000>
		</FORM></th>
	</tr>
	<tr>
		 <th ><FORM method="POST" ENCTYPE="multipart/form-data" onsubmit="return confirm('Are you sure you want to upload a new lm32 binary file? \nThis could result in the malfunction of the switch.');">
								 <INPUT type=file name="lm32file" >
								  <INPUT type=submit value="Load lm32 bin" class="btn">
								  <INPUT type=hidden name=MAX_FILE_SIZE  VALUE=<?php wrs_php_filesize();?>000>
		</FORM></th>
	</tr>
	</table>
	
<!--
	<br><br><br><center>Max. filesize is now <?php //echo shell_exec("cat /etc/php.ini | grep upload_max_filesize | awk '{print $3}'"); 
			?></center>
			
			<table border="0" align="center">	
			<tr>
			
			<form align="center" method="post">
			<th>New PHP Filesize: </th><th><INPUT type="text" name="size" > </th>
			<th><input type="submit" value="Change" class="btn"></th>
			</tr>
			</form>
			</table>
-->	

	<div id="bottommsg">
		<center>
		*NOTE: After loading a FPGA or lm32 binary the switch will reboot.
		</center>
	</div>
	
	<?  
		wrs_load_files();

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
