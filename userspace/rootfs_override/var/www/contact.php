<?php include 'functions.php'; include 'head.php'; ?>
<body id="contact">
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
<h1 class="title">About</h1>

	<?php $_SESSION['advance']=""; ?>
	
	<p><strong>WRSW OS: <?php $str = shell_exec("uname -r"); echo $str;  ?> </strong></p>
	<p><strong><?php $str = shell_exec("/wr/bin/shw_ver -g"); $str = str_replace("\n","<br>",$str); 
		$str=str_replace("Reading GW info","",$str); echo $str; ?></strong></p><p>&nbsp;</p>
	<br>
	<center><p align=right><strong>Open Hardware Repository  <a href="http://www.ohwr.org/projects/white-rabbit/wiki">http://www.ohwr.org/projects/white-rabbit/wiki</a> </strong></p></strong></p><p>&nbsp;</p>
	<p align=right><strong>White-Rabbit Mailing List <a href="mailto:white-rabbit-dev@ohwr.org?subject=[White-Rabbit Switch Local Management Tool]">(white-rabbit-dev@ohwr.org)</a> </strong></p></center>
	
	<br><br><br><br><br>
	<center>
	<IMG SRC="img/cern.jpg" WIDTH=80  ALT="CERN">
	<IMG SRC="img/7s.png" WIDTH=80  ALT="Seven Solutions">
	<IMG SRC="img/ugr.gif" WIDTH=140 ALT="University of Granada">
	</center>
	
	<br><br>
	<p align=right><strong>Developers: </strong>Alessandro Rubini, Tomasz Wlostowski, Benoit Rat, Federico Vega, Grzegorz Daniluk, Maciej Lipinski, Jose Luis Gutierrez</p>

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
