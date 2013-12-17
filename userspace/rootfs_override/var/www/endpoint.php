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
<h1 class="title">Endpoint Configuration <a href='help.php?help_id=endpoint' onClick='showPopup(this.href);return(false);'><img align=right src="./img/question.png"></a></h1>

	<?php
		echo '<form  method=POST>
				Select an Endpoint:	<select name="endpoint" class="sec">';
					
		for($op = 0; $op < 18; $op++){
			
			echo '<option value="wr'.$op.'">wr'.$op.'</option>';							
				
		}
		
		echo '</select>';
	
		echo '<select name="option1" class="sec">';
		echo '<option value="txcal1">Enable Calibration Transmission</option>';	
		echo '<option value="txcal0">Disable Calibration Transmission</option>';	
		echo '<option value="dump">See Registers</option>';	
		echo '<option value="wr">Modify Registers</option>';	
		//echo '<option value="rt">Show Flags</option>';	
		echo '<option value="lock">Lock Endpoint</option>';	
		echo '<option value="master">Make Endpoint Master</option>';	
		echo '<option value="gm">Make Endpoint GrandMaster</option>';	
			
			
		echo '</select>
					<input type="submit" value="Go!" class="btn">
					</form>';	
		
		
		
		//Second option levels:
		$option1=htmlspecialchars($_POST['option1']);
		$endpoint=htmlspecialchars($_POST['endpoint']);
		
		//Calling phytool.
		if(!empty($option1)){
			wr_endpoint_phytool($option1, $endpoint);					
		}
		
		echo '<br><hr><br>';
		wr_show_endpoint_rt_show();
		echo '<br><hr><br>';
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
