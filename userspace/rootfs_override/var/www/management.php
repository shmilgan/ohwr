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

	<center>

	<table border="0" align="center">
	<tr>
		<form  method="post">
			<th>Reboot system: </th>
			<input type="hidden" name="cmd" value="reboot">
			<th><input type="submit" value="Reboot switch" class="btn">	</th>
		</form>
	</center>
	</tr>
<!--
	<tr>
		<form  method="post">
			<th align=left>Net-SNMP Server: </th>
			<input type="hidden" name="cmd" value="snmp">
			<th><INPUT type="submit" STYLE="text-align:center;" value="<?php echo (check_snmp_status()) ? 'Disable SNMP' : 'Enable SNMP'; ?>" class="btn"></th>
		</form>
	</tr>
-->
	<tr>
		<form  method="post">
			<th align=left>System Monitor: </th>
			<input type="hidden" name="cmd" value="monit">
			<th><INPUT type="submit" STYLE="text-align:center;" value="<?php echo (check_monit_status()) ? 'Disable Monitor' : 'Enable Monitor'; ?>" class="btn"></th>
		</form>
	</tr>
	</table>

	<hr>
	<br><br>
	<center><p><strong>Dot-config source during start:</strong></p></center>
	<?php
		if((!empty($_POST["dotconfig_source"]))){
			/* remove all possible sources from the file */
			delete_from_kconfig("CONFIG_DOTCONF_SOURCE_LOCAL=");
			delete_from_kconfig("CONFIG_DOTCONF_SOURCE_REMOTE=");
			delete_from_kconfig("CONFIG_DOTCONF_SOURCE_TRY_DHCP=");
			delete_from_kconfig("CONFIG_DOTCONF_SOURCE_FORCE_DHCP=");
			delete_from_kconfig("CONFIG_DOTCONF_URL=");
			/* remove all possible sources from the _SESSION */
			$_SESSION["KCONFIG"]["CONFIG_DOTCONF_SOURCE_LOCAL"]="";
			$_SESSION["KCONFIG"]["CONFIG_DOTCONF_SOURCE_REMOTE"]="";
			$_SESSION["KCONFIG"]["CONFIG_DOTCONF_SOURCE_TRY_DHCP"]="";
			$_SESSION["KCONFIG"]["CONFIG_DOTCONF_SOURCE_FORCE_DHCP"]="";
			$_SESSION["KCONFIG"]["CONFIG_DOTCONF_URL"]="";
			/* assembly new source name */
			$new_dot_source = "CONFIG_DOTCONF_".$_POST["dotconfig_source"];
			/* add new source to session and file */
			$_SESSION["KCONFIG"][$new_dot_source]="y";
			check_add_existing_kconfig($new_dot_source."=");
			/* add URL for remote if source is remote */
			if (!strcmp($_POST["dotconfig_source"], "SOURCE_REMOTE")) {
				$_SESSION["KCONFIG"]["CONFIG_DOTCONF_URL"]=$_POST["dotconfig_URL"];
				check_add_existing_kconfig("CONFIG_DOTCONF_URL=");
			}
			save_kconfig();
		}

		echo '<FORM method="POST">
			<table class="altrowstable" id="alternatecolor" width="100%">
				<tr>
					<td align=center width=25%>
					<select name=dotconfig_source>
						<option name=dotconfig_source value="SOURCE_LOCAL"';  if(!strcmp(wrs_dotconf_source_setup(), "source_local")) echo "selected";
							echo '> Local </option>';
						echo '<option name=dotconfig_source value="SOURCE_REMOTE"';  if(!strcmp(wrs_dotconf_source_setup(), "source_remote")) echo "selected";
						echo '> Remote </option>';
						echo '<option name=dotconfig_source value="SOURCE_TRY_DHCP"';  if(!strcmp(wrs_dotconf_source_setup(), "source_try_dhcp")) echo "selected";
						echo '> Try DHCP </option>';
						echo '<option name=dotconfig_source value="SOURCE_FORCE_DHCP"';  if(!strcmp(wrs_dotconf_source_setup(), "source_force_dhcp")) echo "selected";
						echo '> Force DHCP </option>;
					</select>
					<td align=center width=50%><INPUT type=text name="dotconfig_URL" title="Remote URL for dot-config file" VALUE='; echo $_SESSION['KCONFIG']['CONFIG_DOTCONF_URL']; echo '></td>
					<td align=center width=25%><INPUT type="submit" value="Change" class="btn"></td>
				</tr>
			</table>
			</FORM>
			<br>';
	?>

	<center><p><strong>Load dot-config from local computer</strong></p></center>
	<table class='altrowstable' id='alternatecolor1' width="100%">
		<tr>
		<FORM method="POST" ENCTYPE="multipart/form-data" onsubmit="return confirm('Are you sure you want to upload a dotconfig file? \nThis could result in the malfunction of the switch.');">
			<td ><INPUT type=file name="kconfig" ></td>
			<td align=center><INPUT type=submit value="Load dot-config" class="btn" ><INPUT type=hidden name=MAX_FILE_SIZE  VALUE= <?php wrs_php_filesize();?>000></td>
		</form>
		</tr>
	</table>
	</center>

	<br>
	<center>
	<FORM method="POST" ENCTYPE="multipart/form-data">
			Backup configuration file to your computer
			<input type="hidden" name="cmd" value="Backup">
			<INPUT type=submit value="Backup" class="btn" >
	</form>
	</center>

	<div id="bottommsg">
	<hr>
	<p align="right">Click <A HREF="showfile.php?help_id=dotconfig" target="_blank">here</A> to display the current dotconfig file</p>
	</div>

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
