<?php include 'functions.php'; include 'head.php'; ?>
<body id="vlan">
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
<h1 class="title">VLAN Configuration <a href='help.php?help_id=vlan' onClick='showPopup(this.href);return(false);'><img align=right src="./img/question.png"></a></h1> 

	<?php session_is_started() ?>
	<?php $_SESSION['advance']=""; ?>
	
	<?php
		echo '<table align=center border="1" class="altrowstable" id="alternatecolor">';
		echo '<tr><th>Endpoint</th><th>VLANs</th></tr>';
		for($i = 0; $i < 18; $i++){
			echo '<tr>';
			echo '<td><center><b>wr'.($i+1).'</b></center></td>';
			
			//Show the Vlan option button
			echo '<td>';
			echo '<form method=POST><div>
					<select name='.($i).'>';
					
			for($op = 0; $op < 18; $op++){
			
				  echo '<option class="btn" value="'.($op+1).'"><center>Vlan'.($op+1).'</center></option>';							
				
			}
			echo '</select>';
					
			echo '</td>';

			
		}
		echo '<input type="hidden" value=cmd name=cmd>';
		echo '<p align="right"><input type="submit" value="Add VLANs" class="btn" ></p>
					</form>';	
			echo '</tr>';
		
		echo '</table>';
		
		
		//Parse input and run the command
		if (!empty($_POST['cmd'])){
			$input = $_POST;
			wrs_vlan_configuration($input);
		}
		
		wrs_vlan_display();

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
