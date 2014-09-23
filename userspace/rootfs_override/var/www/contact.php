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
	
	<div>
		<?php
			$msg = shell_exec("/wr/bin/wrs_version -g");
			$msg = explode("\n", $msg);
			$message .= "<ul>";

			$message .= "<li>WRSW OS:".shell_exec("uname -r")."</li>";
			for($i=0; $i<5; $i++){

				$message .= "<li>".$msg[$i]."</li>";
			}
			$message .= "</ul>";
			echo $message;

		?>
	</div>
	<br>
	
	<div>
		<center><p align=right><strong>Open Hardware Repository  <a href="http://www.ohwr.org/projects/white-rabbit/wiki">http://www.ohwr.org/projects/white-rabbit/wiki</a> </strong></p></strong></p><p>&nbsp;</p>
		<p align=right><strong>White-Rabbit Mailing List <a href="mailto:white-rabbit-dev@ohwr.org?subject=[White-Rabbit Switch Local Management Tool]">(white-rabbit-dev@ohwr.org)</a> </strong></p></center>
	</div>

	<br><br><br><br>

	<div id="logo">
		<center>
		 <IMG SRC="img/cern.jpg" WIDTH=80  ALT="CERN">
		 <IMG SRC="img/7s.png" WIDTH=80  ALT="Seven Solutions">
		 <IMG SRC="img/ugr.gif" WIDTH=140 ALT="University of Granada">
		</center>
	</div>
	
	<div>
		<p align=right><strong>Developers: </strong>Alessandro Rubini, Tomasz Wlostowski, Benoit Rat, Federico Vega, Grzegorz Daniluk, Maciej Lipinski, Jose Luis Gutierrez</p>
	</div>

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
