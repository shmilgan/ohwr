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
<h1 class="title">Switch Management <a href='help.php?help_id=management' onClick='showPopup(this.href);return(false);'><img align=right src="./img/question.png"></a></h1>

	<?php session_is_started() ?>
	<?php $_SESSION['advance']=""; ?>

	<table border="0" align="center">	
		
	</table>

	<table border="0" align="center">	
		<tr>
			<form  method="post">
			<th><center>Switch Mode: (<?php  $str = check_switch_mode(); echo $str; ?>) </center><input type="hidden" name="cmd" value="change"></th>
			<th><input type="submit" value="Change Mode" class="btn"></th>
			</form>
		</tr>
		<tr>
			<form  method="post">
			<th><center>Reboot system: </center><input type="hidden" name="cmd" value="reboot"></th>
			<th><input type="submit" value="Reboot switch" class="btn"></th>
			</form>
		</tr>
		</tr><th> </th><th> </th><tr></tr><th> </th><th> </th><tr>
		</tr><th> </th><th> </th><tr></tr><th> </th><th> </th><tr>
		<hr>
		<tr>
			<form  method="post">
			<th>NTP Server:   <INPUT type="text" name="ntpip" value="<?php  $str = check_ntp_server(); echo $str; ?>"> </th>
			<input type="hidden" name="cmd" value="ntp">
			<th><select name="utc" >
					<?php 
						$selected_utc=$_SESSION['utc'];
						$selected_utc=str_replace("UTC","",$selected_utc);
						for($op = -12; $op < 0; $op++){
							
							if($selected_utc==$op){
								echo '<option selected="UTC'.$op.'" class="btn" value="UTC'.($op).'"><center>UTC'.($op).'</center></option>';
							}else{
								echo '<option  class="btn" value="UTC'.($op).'"><center>UTC'.($op).'</center></option>';
							}
														
						}
						if(!strcmp($selected_utc,"")){
							echo '<option selected="UTC" class="btn" value="UTC"><center>UTC</center></option>';	
						}else{
							echo '<option class="btn" value="UTC"><center>UTC</center></option>';	
						}
						
						for($op = 1; $op < 15; $op++){
							if($selected_utc==$op){
								echo '<option elected="UTC'.$op.'" class="btn" value="UTC+'.($op).'"><center>UTC+'.($op).'</center></option>';
							}else{
								echo '<option class="btn" value="UTC+'.($op).'"><center>UTC+'.($op).'</center></option>';		
							}					
						}
					?>
			</th>
			<th><input type="submit" value="Add NTP Server" class="btn"></th>
			</form>
		</tr>
		
	</table>
	<hr>
	<br><br>
	<center><p><strong>Load configuration files</strong></p></center>
	<table border="1" align="center" class='altrowstable' id='alternatecolor'>	
		<tr>
		<FORM method="POST" ENCTYPE="multipart/form-data">
			<th>PPSi Config </th>
			<th ><INPUT type=file name="ppsi_conf" ></th>
			<th><INPUT type=submit value="Load" class="btn" ><INPUT type=hidden name=MAX_FILE_SIZE  VALUE= <?php wrs_php_filesize();?>000></th>
		</form>
		</tr>
		<tr>
		<FORM method="POST" ENCTYPE="multipart/form-data">
			<th>SFP Config </th>
			<th ><INPUT type=file name="sfp_conf" ></th>
			<th><INPUT type=submit value="Load" class="btn" ><INPUT type=hidden name=MAX_FILE_SIZE  VALUE= <?php wrs_php_filesize();?>000></th>
		</form>
		</tr>
		<tr>
		<FORM method="POST" ENCTYPE="multipart/form-data">
			<th>SNMP Config</th>
			<th ><INPUT type=file name="snmp_conf" ></th>
			<th><INPUT type=submit value="Load" class="btn" ><INPUT type=hidden name=MAX_FILE_SIZE  VALUE= <?php wrs_php_filesize();?>000></th>
		</form>
		</tr>
		<tr>
		<FORM method="POST" ENCTYPE="multipart/form-data">
			<th>HAL Config </th>
			<th ><INPUT type=file name="hal_conf" ></th>
			<th><INPUT type=submit value="Load" class="btn" ><INPUT type=hidden name=MAX_FILE_SIZE  VALUE= <?php wrs_php_filesize();?>000></th>
		</form>
		</tr>
		<tr>
			<th></th>
			<th></th>
			<th></th>
		</tr>
		<tr>
		<FORM method="POST" ENCTYPE="multipart/form-data">
			<th>Restore from backup</th>
			<th ><INPUT type=file name="restore_conf" ></th>
			<th><INPUT type=submit value="Load" class="btn" ><INPUT type=hidden name=MAX_FILE_SIZE  VALUE= <?php wrs_php_filesize();?>000></th>
		</form>
		</tr>

	</table>
	
	<br><br><br><br>
	<FORM align="right" method="POST" ENCTYPE="multipart/form-data">
			<th>Backup Configuration files to your computer </th>
			<input type="hidden" name="cmd" value="Backup">
			<th><INPUT type=submit value="Backup" class="btn" ></th>
	</form>
	
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
