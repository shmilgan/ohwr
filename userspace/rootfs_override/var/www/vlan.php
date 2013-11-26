<?php include 'title.php'; ?>

<body>

<div class="page">
    <div class="left-bar">
        <div class="menu">
			<?php include 'menu.php'; ?>
        </div>
    </div>
    <div class="right-bar">
        <div class="header"><?php include 'header.php'; ?></div>
        <div class="content">
			
			<?php
				echo '<table align=center border="1">';
				echo '<tr><td>Endpoint</td><td>VLANs</td></tr>';
				for($i = 0; $i < 18; $i++){
					echo '<tr>';
					echo '<th>wr'.$i.'</th>';
					//Print here all the vlans the endpoint belongs to. in <th> </th>
					
					//Show the Vlan option button
					echo '<th>';
					echo '<form action="exe_program.php" method=POST><div>
							<select name="cmd">';
							
					for($op = 0; $op < 18; $op++){
					
						  echo '<option value="VLAN'.$op.'">Vlan'.$op.'</option>';							
						
					}
					echo '</select>
							<input type="submit" value="Add VLAN">
							</form>';
					echo '</th>';
						
					echo '</tr>';
				}
				echo '</table>';
        
			?>
       
        <div class="footer"><?php include 'footer.php'; ?></div>
        
    </div>
</div>
</body>
</html>
