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
<h1 class="title">VLAN Configuration</h1>

	<?php
		echo '<table align=center border="1" class="altrowstable" id="alternatecolor">';
		echo '<tr><th>Endpoint</th><th>VLANs</th></tr>';
		for($i = 0; $i < 18; $i++){
			echo '<tr>';
			echo '<td>wr'.$i.'</td>';
			//Print here all the vlans the endpoint belongs to. in <th> </th>
			
			//Show the Vlan option button
			echo '<td>';
			echo '<form action="exe_program.php" method=POST><div>
					<select name="cmd">';
					
			for($op = 0; $op < 18; $op++){
			
				  echo '<option value="VLAN'.$op.'">Vlan'.$op.'</option>';							
				
			}
			echo '</select>
					<input type="submit" value="Add VLAN">
					</form>';
			echo '</td>';
				
			echo '</tr>';
		}
		echo '</table>';

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
