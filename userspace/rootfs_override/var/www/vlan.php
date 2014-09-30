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
	
		
	
		echo '<center><strong>Existing VLANs</strong></center><hr>';
		$tmp_vlan_file="/tmp/vlans.conf";
		$vlans = shell_exec("/wr/bin/wrs_vlans --list >".$tmp_vlan_file);
		$vlans = shell_exec("cat ".$tmp_vlan_file." |  sed -n '/ /s/ \+/ /gp'");
		$vlans = explode("\n", $vlans);
		
		echo '<table class="altrowstable firstcol" id="alternatecolor" width="100%">';
		echo '<tr class="sub" align=center><th>Vlan ID</th><th>FID</th><th>Ports</th><th>Drop?</th><th>Priority</th><th>Action</th></tr>';
		$counter = 0;
		foreach($vlans as $line){
			$counter++;
			
			if($counter>=2 && !empty($line)){
				$line = explode(" ", $line);
				if(strcmp($line[3],"0x")){
					
					echo '<tr align=center><td bgcolor="'.$vlancolor[$line[1]].'">VLAN '.$line[1].'</td><td>'.$line[2].'</td><td>'.parse_mask2ports($line[3]).'</td><td>'.$line[4].'</td><td>'.$line[5].'</td><td><A HREF="delvlan.php?vlan='.$line[1].'.">Delete</A></td></tr>';
				}else{
					echo '<tr align=center><td>'.$line[1].'</td><td>'.$line[2].'</td><td>'.parse_mask2ports($line[3].$line[4]).'</td><td>'.$line[5].'</td><td>'.$line[6].'</td><td><A HREF="delvlan.php?vlan='.$line[1].'.">Delete</A></td></tr>';
				}
				
			}
         
		}
		//Form for a new one:
		echo '<tr align=center>
			<FORM method="POST" action="newvlan.php" "ENCTYPE="multipart/form-data">
				<td align=center><INPUT type="text" size="3" name="vid" ></td>
				<td align=center><INPUT type="text" size="3"name="fid" ></td>
				<td align=center><INPUT type="text" size="5" name="mask" ></td>
				<td align=center>
					<select name="drop">
					  <option value="1">YES</option>
					  <option selected="selected" value="0">NO</option>
					</select>
				</td>
				<td align=center>
					<select name="prio">
					  <option selected="selected"value=""></option>
					  <option value="0">0</option>
					  <option value="1">1</option>
					  <option value="2">2</option>
					  <option value="3">3</option>
					  <option value="4">4</option>
					  <option value="5">5</option>
					  <option value="6">6</option>
					  <option value="7">7</option>
					</select>
				</td>
				<td align=center><INPUT type=submit value="Add VLAN" class="btn"></td>
			</form>
		</tr>';
		echo '</table>';
		?>
		
		<br><p align="right"><A HREF="delvlan.php?vlan=all" onclick="return confirm('You are deleting all VLANs, are you sure?')">Delete All VLANs</A></p>
		
		<?php
		//Display Port2Vlan assignment
		echo '<br><br>';
		echo '<center><strong>Port2Vlan assignments</strong></center><hr>';
		echo '<table class="altrowstable firstcol" id="alternatecolor1" width="100%">';
		echo '<tr><th>Port</strong></th><th>QMode</th><th>Priority</th><th>VLAN ID</th><th>MAC Address</th></tr>';
		
		$tmp_vlan_file="/tmp/port2vlan.conf";
		$vlans = shell_exec("/wr/bin/wrs_vlans --elist >".$tmp_vlan_file);
		$vlans = shell_exec("cat ".$tmp_vlan_file." |  sed -n '/ /s/ \+/ /gp'");
		$vlans = explode("\n", $vlans);
		
		$counter = 0;
		foreach($vlans as $line){
			$counter++;
			
			if($counter>=2 && !empty($line)){
				$line = explode(" ", $line);
					
				echo '<tr align=center><td>WR'.($line[1]+1).'</td><td>'.$line[2]." (".$line[3].')</td><td>'.($line[5]).'</td><td bgcolor="'.$vlancolor[$line[6]].'">VLAN '.$line[6].'</td><td>'.$line[7].'</td></td></tr>';

			}
         
		}
		echo '<tr><td></td><td></td><td></td><td></td><td align=center><FORM method="POST" action="port2vlan.php"><INPUT type=submit align=center value="Edit Table" class="btn"></form></td></tr>';
		echo '</table>';
		
		?>
		
		
		<div id="bottommsg"><hr><p align="right"><A HREF="port2vlan.php">Assign Ports to VLANs</A></p></div>
		
		
	


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
