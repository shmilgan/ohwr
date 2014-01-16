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
<h1 class="title">White-Rabbit Switch Console <a href='help.php?help_id=console' onClick='showPopup(this.href);return(false);'><img align=right src="./img/question.png"></a></h1>

	<?php session_is_started() ?>

	<FORM action="terminal.php" method="POST" accept-charset="UTF-8">
	Unix Command: <input type="text" name="cmd">
	<input type="submit" value="Enter" class="btn">
	</FORM>
	

	<?php
	$cmd = htmlspecialchars($_POST["cmd"]);
	$output = shell_exec($cmd);
	
	echo '<div align="center"> <div id="preview" style= "BORDER-RIGHT: #000 1px solid; PADDING-RIGHT: 0px; 
		BORDER-TOP: #000 1px solid; PADDING-LEFT: 2px; PADDING-BOTTOM: 2px; WORD-SPACING: 1px; OVERFLOW: scroll; 
		BORDER-LEFT: #000 1px solid; WIDTH: 100%; PADDING-TOP: 1px; 
		BORDER-BOTTOM: #000 2px solid; HEIGHT: 350px; TEXT-ALIGN: left"> 
		<p>$'.$cmd.':<br>'.$output.'</p> </div></div>';

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
