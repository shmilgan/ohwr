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
<h1 class="title">PTP Configuration <a href='help.php?help_id=ptp' onClick='showPopup(this.href);return(false);'><img align=right src="./img/question.png"></a></h1>

	<?php session_is_started() ?>
	<?php $_SESSION['advance']=""; ?>
	
	<FORM method="POST">
	<table border="0" align="center">	
			<tr>
				<th align=left>PTP Daemon: </th>
				<th><input type="radio" name="daemongroup" value="On" <?php echo (wrs_check_ptp_status()) ? 'checked' : ''; ?> > On <br>
					<input type="radio" name="daemongroup" value="Off" <?php echo (wrs_check_ptp_status()) ? '' : 'checked'; ?> > Off <br>
				<th><INPUT type="submit" value="Update" class="btn"></th>	
			</tr>
	</table>
						
	</FORM>

	<FORM method="POST">
		<table border="0" align="center">	
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
				<th><INPUT type="submit" value="Submit Configuration" class="btn"></th>
			</tr>

		</table>
		</FORM>
		
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
