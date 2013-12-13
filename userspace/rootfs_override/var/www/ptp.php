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
<h1 class="title">PTP Configuration</h1>

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

		if(!empty($_POST["b"])){
			$cmd = shell_exec("/wr/bin/ptpd -b ".htmlspecialchars($_POST["b"])); 
			echo '<br>Network Interface binded to '.htmlspecialchars($_POST["b"]);
		} 
		if (!empty($_POST["u"])){
			$cmd = shell_exec("/wr/bin/ptpd -u ".htmlspecialchars($_POST["u"])); 
			echo '<br>Unicast sent to '.htmlspecialchars($_POST["u"]);
		} 
		if (!empty($_POST["i"])){
			$cmd = shell_exec("/wr/bin/ptpd -i ".htmlspecialchars($_POST["i"])); 
			echo '<br>PTP Domain number is now '.htmlspecialchars($_POST["i"]);
		} 
		if (!empty($_POST["n"])){
			$cmd = shell_exec("/wr/bin/ptpd -n ".htmlspecialchars($_POST["n"])); 
			echo '<br>Announce Interval set to '.htmlspecialchars($_POST["n"]);
		} 
		if (!empty($_POST["y"])){
			$cmd = shell_exec("/wr/bin/ptpd -y ".htmlspecialchars($_POST["y"])); 
			echo '<br>Sync Interval set to '.htmlspecialchars($_POST["y"]);
		} 
		if (!empty($_POST["r"])){
			$cmd = shell_exec("/wr/bin/ptpd -r ".htmlspecialchars($_POST["r"])); 
			echo '<br>Clock accuracy set to '.htmlspecialchars($_POST["r"]);
		} 
		if (!empty($_POST["v"])){
			$cmd = shell_exec("/wr/bin/ptpd -v ".htmlspecialchars($_POST["v"])); 
			echo '<br>Clock Class is now set to '.htmlspecialchars($_POST["v"]);
		} 
		if (!empty($_POST["p"])){
			$cmd = shell_exec("/wr/bin/ptpd -p ".htmlspecialchars($_POST["p"])); 
			echo '<br>Priority changed to '.htmlspecialchars($_POST["p"]);
		} 
		if ((!empty($_POST["daemongroup"])) && (!strcmp(htmlspecialchars($_POST["daemongroup"]),"On"))){
			$cmd = shell_exec("/wr/bin/ptpd -A"); 
			echo 'PTPd enabled and daemonized!';
		}
		if ((!empty($_POST["daemongroup"])) && (!strcmp(htmlspecialchars($_POST["daemongroup"]),"Off"))){
			$cmd = shell_exec("killall ptpd"); 
			echo 'PTPd killed!';
		}

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
