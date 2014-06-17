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
<h1 class="title">Port-VLAN Assigment <a href='help.php?help_id=vlanassignment' onClick='showPopup(this.href);return(false);'><img align=right src="./img/question.png"></a></h1> 

	<?php session_is_started() ?>
	<?php $_SESSION['advance']=""; ?>
	
	<?php
	
		// Get VLANS
		echo '<center><strong>Port-VLAN List</strong></center><hr>';
		$tmp_vlan_file="/tmp/vlans.conf";
		$vlans = shell_exec("/wr/bin/wrsw_vlans --list >".$tmp_vlan_file);
		$vlans = shell_exec("cat ".$tmp_vlan_file." |  sed -n '/ /s/ \+/ /gp'");
		$vlans = explode("\n", $vlans);
		$name_vlans="";
		$counter = 0;
		foreach($vlans as $line){
			$counter++;
			
			if($counter>=3 && !empty($line)){
				$line = explode(" ", $line);
				$name_vlans .= $line[1]." ";
			}
         
		}
		$name_vlans = explode(" ", $name_vlans);
		
		
		echo '<table align=center border="1" class="altrowstable" id="alternatecolor">';
		echo '<tr align=center><th>Endpoint</th><th>VLANs ID</th><th>Mode</th><th>Priority</th><th>Mask</th></tr>';
		echo '<form method=POST>';
		
		for($i = 0; $i < 18; $i++){
			echo '<tr>';
			echo '<th><center><b>wr'.($i+1).'</b></center></th>';
			
			//Show the Vlan option button
			echo '<th>';
			echo '<select name=vlan'.($i).'>';
					
			
			foreach($name_vlans as $vlan){
				if(!empty($vlan[0])){
				  echo '<option class="btn" value="'.$vlan[0].$vlan[1].'"><center>ID'.$vlan[0].$vlan[1].'</center></option>';	
				}						
				
			}
			echo '<option class="btn" selected="selected" value="disabled"><center>Disabled</center></option>';	
			echo '</select>'; // end Vlan ID assignation
					
			echo '</th>';
			
			echo '<th>'; // Mode selection
				echo '<select name=mode'.($i).'>';
				echo '<option class="btn" selected="selected" value="0"><center>Access</center></option>';	
				echo '<option class="btn"  value="1"><center>Trunk</center></option>';	
				echo '<option class="btn"  value="2"><center>VLAN Disabled</center></option>';	
				echo '<option class="btn" value="3"><center>Unqualified port</center></option>';	
				echo '</select>'; // end mode 
			
			echo '</th>';
			
			echo '<th>'; // Priority selection
				echo '<select name=prio'.($i).'>';
				echo '<option class="btn" selected="selected" value="0"><center>0</center></option>';	
				echo '<option class="btn"  value="1"><center>1</center></option>';	
				echo '<option class="btn"  value="2"><center>2</center></option>';	
				echo '<option class="btn" value="3"><center>3</center></option>';	
				echo '<option class="btn" value="4"><center>4</center></option>';	
				echo '</select>'; // end Priority 
			
			echo '</th>';
			
			echo '<th align=center><INPUT type="text" size="3" name="mask'.$i.'" ></th>';

			echo '</tr>';
		}
		
		echo '<tr><th></th><th></th><th></th><th></th><th><input type="submit" value="Add VLANs" class="btn" name="updatevlan" ></th></tr>
					</form>';	
			
		
		echo '</table>';
		
		
		//Parse input and run the command
		if (!empty($_POST['updatevlan'])){
			$vlan_cmd= "/wr/bin/wrsw_vlans ";
			
			for($i = 0; $i < 18; $i++){
				if(strcmp($_POST['vlan'.$i],"disabled")){ //VLAN selected
					$vlan_cmd .= " --ep ".$i;
					$vlan_cmd .= " --emode ".$_POST['mode'.$i];
					$vlan_cmd .= " --eprio ".$_POST['prio'.$i];
					$vlan_cmd .= " --evid ".$_POST['vlan'.$i];
					if(!empty($_POST['mask'.$i]))$vlan_cmd .= " --eumask ".$_POST['mask'.$i];
					$output = shell_exec($vlan_cmd);
					echo '<br><p><center>Port WR'.($i+1).' added to VLAN'.$_POST['vlan'.$i].'</center></p>';
					
				}else{
					if(!strcmp($_POST['mode'.$i],"2")){ //Disable VLAN for endpoint
						$vlan_cmd .= " --ep ".$i;
						$vlan_cmd .= " --emode ".$_POST['mode'.$i];
						$output = shell_exec($vlan_cmd);
						echo '<br><p><center>VLAN removed for port WR'.($i+1).'</center></p>';
					}
						
				}
				
				$vlan_cmd= "/wr/bin/wrsw_vlans ";
			}
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
