<?php include 'functions.php'; include 'head.php'; ?>
<body id="epmode">
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
<h1 class="title">Endpoint Mode Configuration<a href='help.php?help_id=endpointmode' onClick='showPopup(this.href);return(false);'><img align=right src="./img/question.png"></a></h1>

	<?php session_is_started() ?>
	<?php $_SESSION['advance']=""; ?>
    
	<?php
		//Load all 
		$modes = parse_wrsw_hal_file();
				
		echo '<table align=center border="1" class="altrowstable" id="alternatecolor">';
		//echo '<tr><th><center>Endpoint</center></th><th><center>Mode</center></th></tr>';
		for($i = 0; $i < 9; $i++){
			echo '<tr>';
			echo '<td><center><b>wr'.($i+1).'</b></center></td>';
			echo '<td><center><a href="modifymode.php?wr='.($i+1).'&mode='.$modes[$i+1].'">'.$modes[$i+1].'</a></center></td>';
			
			echo '<td><center><b>wr'.($i+1+9).'</b></center></td>';
			echo '<td><center><a href="modifymode.php?wr='.($i+1+9).'&mode='.$modes[$i+1+9].'">'.$modes[$i+1+9].'</a></center></td>';
			echo '</tr>';
			
		}
		
		//echo '</tr>';
		
		echo '</table>';
		echo '<br>';
		
		//wrs_check_writeable();
		

	?>
	<br>
	<hr>
	<FORM align="right" method="post">
	<input type="hidden" name="hal" value="hal">
	<INPUT type="submit" value="Reboot Hal daemon" class="btn">
    </FORM>
    
    <?php
		if (!empty($_POST["hal"])){
			
			//Relaunching wrsw_hal to commit endpoint changes
			shell_exec("killall wrsw_hal");
			shell_exec("/wr/bin/wrsw_hal -c /wr/etc/wrsw_hal.conf > /dev/null 2>&1 &");
			
			//We must relaunch ptpd too. (by default)
			shell_exec("killall ptpd"); 
			$ptp_command = "/wr/bin/ptpd -A -c  > /dev/null 2>&1 &";
			$output = shell_exec($ptp_command); 
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
