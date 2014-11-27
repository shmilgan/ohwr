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
	
	<FORM method="POST">
	<table id="daemon" border="0" align="center">	
			<tr>
				<th align=center>PPSi Daemon: </th>
				<input type="hidden" name="cmd" value="ppsiupdate">
				<th><INPUT type="submit" value="<?php echo (wrs_check_ptp_status()) ? 'Disable PPSi' : 'Enable PPSi'; ?>" class="btn"></th>	
			</tr>
	</table>
						
	</FORM>


	<FORM method="POST">
		<table border="0" align="center">	
			<tr>
				<th align=left>Clock Class: </th>
				<th><INPUT type="text" STYLE="text-align:center;" size="10" name="clkclass" value="<?php echo shell_exec("cat ".$GLOBALS['etcdir'].$GLOBALS['ppsiconf']." | grep class | awk '{print $2}'");?>" ></th>
			</tr>
			<tr>
				<th align=left>Clock Accuracy: </th>
				<th><INPUT type="text" STYLE="text-align:center;" size="10" name="clkacc" value="<?php echo shell_exec("cat ".$GLOBALS['etcdir'].$GLOBALS['ppsiconf']." | grep accuracy | awk '{print $2}'");?>"></th>
			</tr>
<!--
			<tr>
				<th align=left>Network Interface Binding: </th>
				<th><INPUT type="text" name="b" ></th>
			</tr>
			<tr>
				<th align=left>Unicast to Address: </th>
				<th><INPUT type="text" name="u" ></th>
			</tr>
			<tr>
				<th align=left>PTP Domain Number: </th>
				<th><INPUT type="text" name="i" ></th>
			</tr>
			<tr>
				<th align=left>Announce Interval: </th>
				<th><INPUT type="text" name="n" ></th>
			</tr>
			<tr>
				<th align=left>Sync Interval: </th>
				<th><INPUT type="text" name="y" ></th>
			</tr>
			<tr>
				<th align=left>Clock Accuracy: </th>
				<th><INPUT type="text" name="r" ></th>
			</tr>
			<tr>
				<th align=left >Clock Class: </th>
				<th><INPUT type="text" name="v" ></th>
			</tr>
			<tr>
				<th align=left>Priority: </th>
				<th><INPUT type="text" name="p" ></th>
			</tr>
-->
		</table>
		<hr>
		<p align=right><INPUT align="right" type="submit" value="Update & Relaunch" class="btn last"></p>
		</FORM>
		
		<div id="bottommsg">
		<hr>
		<p align="right">Click <A HREF="endpointmode.php">here</A> to modify endpoint mode configuration</p>
		</div>
	
		
	<?php
		wrs_ptp_configuration();

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
