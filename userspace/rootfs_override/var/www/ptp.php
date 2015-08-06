<?php include 'functions.php'; include 'head.php'; ?>
<body id="ptp">
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
<h1 class="title">WR-PPSi Configuration <a href='help.php?help_id=ptp' onClick='showPopup(this.href);return(false);'><img align=right src="./img/question.png"></a></h1>

	<?php session_is_started() ?>
	<?php $_SESSION['advance']=""; ?>

	<center>
	<FORM method="POST">
		<table id="mode" border="0" align="center">
				<tr>
					<td align=left><b>Switch Mode:</b></td>
					<td><input type="radio" name="modegroup" value="GM" <?php if(!strcmp(check_switch_mode(), "GM")) echo "checked"; ?>
					> GrandMaster <br>
					<input type="radio" name="modegroup" value="BC" <?php if(!strcmp(check_switch_mode(), "BC")) echo "checked"; ?>
					> Boundary Clock <br>
						<input type="radio" name="modegroup" value="FM"<?php if(!strcmp(check_switch_mode(), "FM")) echo "checked"; ?>
					> Free-Running Master <br></td>
					<td><INPUT type="submit" value="Change" class="btn"></td>
				</tr>
		</table>
	</FORM>
	<hr>
	<table border="0" align="center">
		<tr>
			<form  method="post" <?php if(!strcmp(check_switch_mode(), "BC"))
			echo 'onsubmit="return confirm(\'Please notice that UTC time from a NTP Server is overriden by White-Rabbit for Boundary Clocks. Do you want to proceed?\');"'; ?> >
			<th><center>NTP Server:</center></th><th><INPUT type="text" STYLE="text-align:center;" name="ntpip" value="<?php  $str = check_ntp_server(); echo $str; ?>"> </th>
			<th><input type="hidden" name="cmd" value="ntp">
			<th><input type="submit" value="Add NTP Server" class="btn"></th>
			</form>
		</tr>
	</table>
	<hr>

	<?php
		$PPSi_daemon = (wrs_check_ptp_status()) ? 'ENABLED' : 'DISABLED';
		$PPSi_boot = ($_SESSION["KCONFIG"]["CONFIG_PPSI"]=="y") ? 'ENABLED' : 'DISABLED';

		/* This code enables or disables PPSi, not shown at the moment.
		 * It might be something for the future
		echo '
		<FORM method="POST">
		<table id="daemon" border="0" align="center">
			<tr>
				<th align=center>PPSi Daemon: </th>
				<input type="hidden" name="cmd" value="ppsiupdate">
				<th><INPUT type="submit" value='.$PPSi_daemon.' class="btn"></th>
			</tr>
		</table>
		</FORM>';

		echo'
		<FORM method="POST">
		<table id="daemon" border="0" align="center">
			<tr>
				<th align=center>PPSi @ Boot: </th>
				<input type="hidden" name="cmd" value="ppsibootupdate">
				<th><INPUT type="submit" value='.$PPSi_boot.' class="btn"></th>
			</tr>
		</table>
		</FORM>';*/

		$formatID = "alternatecolor";
		$class = "altrowstable firstcol";
		$infoname = "PPSi Parameters";
		$format = "table";
		$section = "WRS_FORMS";
		$subsection = "CONFIG_PPSI";

		$_SESSION["WRS_FORMS"]["CONFIG_PPSI"][CONFIG_PPSI_00]["value"]=
			shell_exec("cat /wr/etc/ppsi-pre.conf | grep clock-class | awk '{print $2}'");
		$_SESSION["WRS_FORMS"]["CONFIG_PPSI"][CONFIG_PPSI_01]["value"]=
			shell_exec("cat /wr/etc/ppsi-pre.conf | grep clock-accuracy | awk '{print $2}'");;

		print_form($section, $subsection, $formatID, $class, $infoname, $format);

		wrs_ptp_configuration();
		wrs_management();

		if ((!empty($_POST["modegroup"]))){

			if (!strcmp(htmlspecialchars($_POST["modegroup"]),"GM")){
				$_SESSION["KCONFIG"]["CONFIG_TIME_GM"]="y";
				if(!empty($_SESSION["KCONFIG"]["CONFIG_TIME_BC"]))
					$_SESSION["KCONFIG"]["CONFIG_TIME_BC"]="n";
				if(!empty($_SESSION["KCONFIG"]["CONFIG_TIME_FM"]))
					$_SESSION["KCONFIG"]["CONFIG_TIME_FM"]="n";
			}

			if (!strcmp(htmlspecialchars($_POST["modegroup"]),"BC")){
				$_SESSION["KCONFIG"]["CONFIG_TIME_BC"]="y";
				if(!empty($_SESSION["KCONFIG"]["CONFIG_TIME_GM"]))
					$_SESSION["KCONFIG"]["CONFIG_TIME_GM"]="n";
				if(!empty($_SESSION["KCONFIG"]["CONFIG_TIME_FM"]))
					$_SESSION["KCONFIG"]["CONFIG_TIME_FM"]="n";
			}

			if (!strcmp(htmlspecialchars($_POST["modegroup"]),"FM")){
				$_SESSION["KCONFIG"]["CONFIG_TIME_FM"]="y";
				if(!empty($_SESSION["KCONFIG"]["CONFIG_TIME_GM"]))
					$_SESSION["KCONFIG"]["CONFIG_TIME_GM"]="n";
				if(!empty($_SESSION["KCONFIG"]["CONFIG_TIME_BC"]))
					$_SESSION["KCONFIG"]["CONFIG_TIME_BC"]="n";
			}
			save_kconfig();
			apply_kconfig();
			load_kconfig();
			header('Location: ptp.php');
		}
	?>
	<div id="bottommsg">
	<hr>
	<p align="right">Click <A HREF="endpointmode.php">here</A> to modify endpoint mode configuration</p>
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
