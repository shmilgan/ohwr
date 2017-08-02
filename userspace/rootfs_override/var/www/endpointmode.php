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
		$modes = parse_endpoint_modes();

		echo '<table class="altrowstable" id="alternatecolor" style="width:100%;text-align:center">';
		for($i = 0; $i < 9; $i++){
			echo '<tr>';
			echo '<th>wri'.($i+1).'</td>';
			echo '<td><a href="modifymode.php?wri='.($i+1).'&mode='.$modes[$i].'">'.$modes[$i].'</a></th>';

			echo '<th>wri'.($i+10).'</th>';
			echo '<td><a href="modifymode.php?wri='.($i+10).'&mode='.$modes[$i+9].'">'.$modes[$i+9].'</a></td>';
			echo '</tr>';
		}
		echo '</table>';
		echo '<br>';

		echo '<p align="right">Go to <a href="endpointconfiguration.php">advanced mode</a></p>';

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
