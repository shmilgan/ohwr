<?php include 'functions.php'; include 'head.php'; ?>
<body id="sfpcalib">
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
<h1 class="title">SFP Database <a href='http://www.ohwr.org/projects/white-rabbit/wiki/Calibration' target='_blank'><img align=right src="./img/question.png"></a></h1>

<?php session_is_started() ?>

	<br>
	

	<?php 
	
		$formatID = "alternatecolor";
		$class = "altrowstable firstcol";
		$infoname = "SFP Calibration";
		$size = "12";
		
		$header = array ("SFP Name","Tx","Rx","wl_txrx"); 
		$matrix = array();
		
		for($i = 0; $i < 18; $i++){
			$endpoint = intval($i);
			$endpoint = sprintf("%02s", $endpoint);
			$endpoint = strval($endpoint);
			
			if(!empty($_SESSION["KCONFIG"]["CONFIG_SFP".$endpoint."_PARAMS"])){
				array_push($matrix, "key=CONFIG_SFP".$endpoint."_PARAMS,".$_SESSION["KCONFIG"]["CONFIG_SFP".$endpoint."_PARAMS"]);
				$last = $i;
			}
		}
		
		if(!empty($_GET["add"])){
			$last++;
			if($last<10){
				$last = sprintf("%02s", $last);
				$last = strval($last);
				array_push($matrix, "key=CONFIG_SFP".$last."_PARAMS,name=empty,tx=empty,rx=empty,wl_txrx=empty");
			}else{
				echo "<center>There is only space for 9 SFP configurations.</center>";
			}
		}
		
		print_multi_form($matrix, $header, $formatID, $class, $infoname, $size);
		
		echo '<hr><p align="right">Click <A HREF="sfpcalibration.php?add=y">here</A> to a new SFP</p>';
		
		if(process_multi_form($matrix)){
			save_kconfig();
			apply_kconfig();
			
			header ('Location: sfpcalibration.php');
			
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
