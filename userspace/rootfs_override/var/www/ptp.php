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
<h1 class="title">PPSi Configuration <a href='help.php?help_id=ptp' onClick='showPopup(this.href);return(false);'><img align=right src="./img/question.png"></a></h1>

	<?php session_is_started() ?>
	<?php $_SESSION['advance']=""; ?>
	
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
