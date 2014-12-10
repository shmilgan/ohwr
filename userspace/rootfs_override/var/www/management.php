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
	<tr>
	<center>
		<form  method="post">
			<th>Switch Mode: (<?php  $str = check_switch_mode(); echo $str; ?>)</th>
			<input type="hidden" name="cmd" value="change">
			<th><input type="submit" value="Change Mode" class="btn"></th>
		</form>
	</tr>
	<tr>
		<form  method="post">
			<th>Reboot system: </th>
			<input type="hidden" name="cmd" value="reboot">
			<th><input type="submit" value="Reboot switch" class="btn">	</th>
		</form>
	</center>
	</tr>
	<tr>
		<form  method="post">
			<th align=left>Net-SNMP Server: </th>
			<input type="hidden" name="cmd" value="snmp">
			<th><INPUT type="submit" STYLE="text-align:center;" value="<?php echo (check_snmp_status()) ? 'Disable SNMP' : 'Enable SNMP'; ?>" class="btn"></th>	
		</form>
	</tr>
	</table>
		

	<table border="0" align="center">	
		<tr>
			<form  method="post">
			<th><center>NTP Server:</center></th><th><INPUT type="text" STYLE="text-align:center;" name="ntpip" value="<?php  $str = check_ntp_server(); echo $str; ?>"> </th>
			<th><input type="hidden" name="cmd" value="ntp">
			<select name="utc" >
					<?php 
						$selected_utc=$_SESSION['utc'];
						$selected_utc=str_replace("UTC","",$selected_utc);
						$selected_utc=trim($selected_utc);
						for($op = -12; $op < 0; $op++){
							
							if($selected_utc==$op && !empty($selected_utc)){
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
							if($selected_utc==$op && !empty($selected_utc)){
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
	<table class='altrowstable' id='alternatecolor' width="100%">
		<tr>
		<FORM method="POST" ENCTYPE="multipart/form-data">
			<td class="first">PPSi Config </td>
			<td ><INPUT type=file name="ppsi_conf" ></td>
			<td><INPUT type=submit value="Load" class="btn" ><INPUT type=hidden name=MAX_FILE_SIZE  VALUE= <?php wrs_php_filesize();?>000></td>
		</form>
		</tr>
		<tr>
		<FORM method="POST" ENCTYPE="multipart/form-data">
			<td class="first">SFP Config </td>
			<td ><INPUT type=file name="sfp_conf" ></td>
			<td><INPUT type=submit value="Load" class="btn" ><INPUT type=hidden name=MAX_FILE_SIZE  VALUE= <?php wrs_php_filesize();?>000></td>
		</form>
		</tr>
		<tr>
		<FORM method="POST" ENCTYPE="multipart/form-data">
			<td class="first">SNMP Config</td>
			<td ><INPUT type=file name="snmp_conf" ></td>
			<td><INPUT type=submit value="Load" class="btn" ><INPUT type=hidden name=MAX_FILE_SIZE  VALUE= <?php wrs_php_filesize();?>000></td>
		</form>
		</tr>
		<tr>
		<FORM method="POST" ENCTYPE="multipart/form-data">
			<td class="first">HAL Config </td>
			<td ><INPUT type=file name="hal_conf" ></td>
			<td><INPUT type=submit value="Load" class="btn" ><INPUT type=hidden name=MAX_FILE_SIZE  VALUE= <?php wrs_php_filesize();?>000></td>
		</form>
		</tr>
		<tr>
		<FORM method="POST" ENCTYPE="multipart/form-data">
			<td class="first">Restore from backup</td>
			<td ><INPUT type=file name="restore_conf" ></td>
			<td><INPUT type=submit value="Load" class="btn" ><INPUT type=hidden name=MAX_FILE_SIZE  VALUE= <?php wrs_php_filesize();?>000></td>
		</form>
		</tr>

	</table>
	
	<br><br><br><br>
	<center>
	<FORM align="center" method="POST" ENCTYPE="multipart/form-data">
			<th>Backup Configuration files to your computer </th>
			<input type="hidden" name="cmd" value="Backup">
			<th><INPUT type=submit value="Backup" class="btn" ></th>
	</form>
	</center>
	
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
