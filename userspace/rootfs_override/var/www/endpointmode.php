<?php include 'functions.php'; include 'head.php'; ?>
<body id="epmode">
<div class="main">
<div class="page">
<div class="header" >
<script type="text/javascript" src="js/dropmodes.js"></script>
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

		$names = array("slave", "auto", "non_wr", "none", "master"); 

		echo '<table class="altrowstable" id="alternatecolor" style="width:100%;text-align:center">';
		for($i = 0; $i < 9; $i++){
			echo '<tr>';
			echo '<th>wri' .($i+1). '</th>';

			$tmp = $i+1;
			echo '<td><select name="selected" id="selected-'.$tmp.'" class="drop">';
			for ($j=0; $j<sizeof($names);$j++){
				if($modes[$i] == $names[$j]){
					echo '<option selected="selected" ">'.$modes[$i].'</option>';
				}
				else{
					echo '<option value='. $names[$j] .'>'. $names[$j] .'</option>';
				}
			} 
               
			echo '</select></td>';

			echo '<th>wri'.($i+10).'</th>';

			$tmp2 = $i+10;
			
			echo '<td><select name="selected" id="selected-'.$tmp2.'" class="drop">';
			
			for ($j=0; $j<sizeof($names);$j++){
				if($modes[$i+9] == $names[$j]){
					echo '<option selected="selected" ">'.$modes[$i].'</option>';
				}
				else{
					echo '<option value='. $names[$j] .'>'. $names[$j] .'</option>';
				}
			} 
			
			echo '</select></td>';
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
