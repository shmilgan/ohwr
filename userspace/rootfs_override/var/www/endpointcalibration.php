<?php include 'functions.php'; include 'head.php'; ?>
<body id="epcalib">
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
<h1 class="title">Endpoint Calibration<a href='http://www.ohwr.org/projects/white-rabbit/wiki/Calibration' target='_blank'><img align=right src="./img/question.png"></a></h1>
<br>

<?php session_is_started() ?>


	<?php
		// Warning message
		echo "<hr>
				<center>NOTE: If you do not know how to calibrate endpoints
				please click on <a href='http://www.ohwr.org/projects/white-rabbit/wiki/Calibration'
				target='_blank'> here </a>*<hr><br>";

		$formatID = "alternatecolor";
		$class = "altrowstablesmall firstcol";
		$infoname = "Endpoint Configuration";
		//$size = "6";

		$header = array ("WR port","Protocol","Tx","Rx","Mode","Fiber");
		$matrix = array ("key=CONFIG_PORT00_PARAMS,".$_SESSION["KCONFIG"]["CONFIG_PORT00_PARAMS"],
							"key=CONFIG_PORT01_PARAMS,".$_SESSION["KCONFIG"]["CONFIG_PORT01_PARAMS"],
							"key=CONFIG_PORT02_PARAMS,".$_SESSION["KCONFIG"]["CONFIG_PORT02_PARAMS"],
							"key=CONFIG_PORT03_PARAMS,".$_SESSION["KCONFIG"]["CONFIG_PORT03_PARAMS"],
							"key=CONFIG_PORT04_PARAMS,".$_SESSION["KCONFIG"]["CONFIG_PORT04_PARAMS"],
							"key=CONFIG_PORT05_PARAMS,".$_SESSION["KCONFIG"]["CONFIG_PORT05_PARAMS"],
							"key=CONFIG_PORT06_PARAMS,".$_SESSION["KCONFIG"]["CONFIG_PORT06_PARAMS"],
							"key=CONFIG_PORT07_PARAMS,".$_SESSION["KCONFIG"]["CONFIG_PORT07_PARAMS"],
							"key=CONFIG_PORT08_PARAMS,".$_SESSION["KCONFIG"]["CONFIG_PORT08_PARAMS"],
							"key=CONFIG_PORT09_PARAMS,".$_SESSION["KCONFIG"]["CONFIG_PORT09_PARAMS"],
							"key=CONFIG_PORT10_PARAMS,".$_SESSION["KCONFIG"]["CONFIG_PORT10_PARAMS"],
							"key=CONFIG_PORT11_PARAMS,".$_SESSION["KCONFIG"]["CONFIG_PORT11_PARAMS"],
							"key=CONFIG_PORT12_PARAMS,".$_SESSION["KCONFIG"]["CONFIG_PORT12_PARAMS"],
							"key=CONFIG_PORT13_PARAMS,".$_SESSION["KCONFIG"]["CONFIG_PORT13_PARAMS"],
							"key=CONFIG_PORT14_PARAMS,".$_SESSION["KCONFIG"]["CONFIG_PORT14_PARAMS"],
							"key=CONFIG_PORT15_PARAMS,".$_SESSION["KCONFIG"]["CONFIG_PORT15_PARAMS"],
							"key=CONFIG_PORT16_PARAMS,".$_SESSION["KCONFIG"]["CONFIG_PORT16_PARAMS"],
							"key=CONFIG_PORT17_PARAMS,".$_SESSION["KCONFIG"]["CONFIG_PORT17_PARAMS"],
							);
		print_multi_form($matrix, $header, $formatID, $class, $infoname, $size);

		if(process_multi_form($matrix)){
			save_kconfig();
			apply_kconfig();

			header ('Location: endpointcalibration.php');
		}

		$formatID = "alternatecolor1";
		$class = "altrowstablesmall firstcol";
		$infoname = "Available Fibers";
		$size = "10";

		$header = array ("#","Alpha");

		$matrix = array ("id=0,key=CONFIG_FIBER00_PARAMS,".$_SESSION["KCONFIG"]["CONFIG_FIBER00_PARAMS"],
							"id=1,key=CONFIG_FIBER01_PARAMS,".$_SESSION["KCONFIG"]["CONFIG_FIBER01_PARAMS"],
							"id=2,key=CONFIG_FIBER02_PARAMS,".$_SESSION["KCONFIG"]["CONFIG_FIBER02_PARAMS"],
							"id=3,key=CONFIG_FIBER03_PARAMS,".$_SESSION["KCONFIG"]["CONFIG_FIBER03_PARAMS"]);

		print_multi_form($matrix, $header, $formatID, $class, $infoname, $size);
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
