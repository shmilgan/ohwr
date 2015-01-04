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
<h1 class="title">Network Management <a href='help.php?help_id=network' onClick='showPopup(this.href);return(false);'><img align=right src="./img/question.png"></a></h1>

	<?php session_is_started() ?>
	<?php $_SESSION['advance']=""; ?>
	
	
	<?php
			
		$formatID = "alternatecolor";
		$class = "altrowstable firstcol";
		$infoname = "DNS Configuration";
		$format = "table";
		$section = "WRS_FORMS";
		$subsection = "DNS_SETUP";
		
		print_form($section, $subsection, $formatID, $class, $infoname, $format);

		$modified = process_form($section, $subsection);
				
		if($modified){
			save_kconfig();
			apply_kconfig();
			header ('Location: network.php');
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
