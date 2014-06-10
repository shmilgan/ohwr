<?php include 'functions.php'; include 'head.php'; ?>
<body id="admin">
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
<h1 class="title">Switch Administration <a href='help.php?help_id=dashboard' onClick='showPopup(this.href);return(false);'><img align=right src="./img/question.png"></a></h1>

	<?php session_is_started() ?>
	
	
	<FORM align="center" method="post">
	<b>Download configuration files to PC: </b><INPUT type="submit" value="Download" class="btn">
	<input type="hidden" name="download" value="hal">
    </FORM>
    
    <br>
    
    <FORM align="center" action="endpointmode.php" method="post">
	<b>Upload configuration files to PC: </b><INPUT type="submit" value="Upload" class="btn">
	<input type="hidden" name="upload" value="hal">
    </FORM>
	
	<?php
		if (!empty($_POST["download"])){
			$filename = "configuration.tar";
			$filepath = "/tmp/";

			// http headers for zip downloads
			header("Pragma: public");
			header("Expires: 0");
			header("Cache-Control: must-revalidate, post-check=0, pre-check=0");
			header("Cache-Control: public");
			header("Content-Description: File Transfer");
			header("Content-type: application/octet-stream");
			header("Content-Disposition: attachment; filename=\"".$filename."\"");
			header("Content-Transfer-Encoding: binary");
			header("Content-Length: ".filesize($filepath.$filename));
			ob_end_flush();
			@readfile($filepath.$filename);
		}
		
		if (!empty($_POST["upload"])){
			
			
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
