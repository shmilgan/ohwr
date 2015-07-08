<?php include 'functions.php'; include 'head.php'; ?>
<body id="network">
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
<h1 class="title">WRS Aux Clock<a href='help.php?help_id=auxclk' onClick='showPopup(this.href);return(false);'><img align=right src="./img/question.png"></a></h1>

	<?php session_is_started() ?>
		
	<?php
	
	
		$formatID = "alternatecolor";
		$class = "altrowstable firstcol";
		$infoname = "Aux. Clock Info";
		$format = "table";
		$section = "WRS_FORMS";
		$subsection = "CONFIG_WRSAUXCLK";

		print_form($section, $subsection, $formatID, $class, $infoname, $format);

		$modified = process_form($section, $subsection);

		if($modified){
			save_kconfig();
			apply_kconfig();
			shell_exec("/etc/init.d/wrs_auxclk restart > /dev/null 2>&1 &");
			header("Location: auxclk.php");
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
