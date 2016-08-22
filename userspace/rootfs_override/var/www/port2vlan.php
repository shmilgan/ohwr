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
		$vlans = shell_exec("/wr/bin/wrs_vlans --list >".$tmp_vlan_file);
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

		// Get Previous assignment
		$tmp_assign_file="/tmp/port2vlan.conf";
		$vlans_assignment = shell_exec("/wr/bin/wrs_vlans --plist >".$tmp_assign_file);
		$vlans_assignment = shell_exec("cat ".$tmp_assign_file." |  sed -n '/ /s/ \+/ /gp'");
		$vlans_assignment = explode("\n", $vlans_assignment);

		echo '<table align=center border="1" class="altrowstable" id="alternatecolor">';
		echo '<tr class="sub" align=center><th>Endpoint</th><th>VLAN ID</th><th>Mode</font></th><th>Priority</th><th>Mask</th></tr>';
		echo '<form method=POST>';

		for($i = 0; $i < 18; $i++){
			$single_line = explode(" ",$vlans_assignment[$i+1]); //info per endpoint line

			echo '<tr>';
			echo '<th><center><b>'.($single_line[0]).'</b></center></th>';

			//Show the Vlan option button
			echo '<th>';
			//echo '<select name=vlan'.($i).'>';

			/*foreach($name_vlans as $vlan){
				if(!empty($vlan[0])){
					echo '<option class="btn" value="'.$vlan[0].$vlan[1].'"><center>ID'.$vlan[0].$vlan[1].'</center></option>';
				}
			}*/
			echo '<input  STYLE="background-color:'.$vlancolor[$single_line[5]].';text-align:center;" size="5" type="text" value="'.$single_line[5].'" name="vlan'.($i).'">';
			//echo '<option class="btn" selected="selected" value="disabled"><center>Disabled</center></option>';
			//echo '</select>'; // end Vlan ID assignation

			echo '</th>';

			echo '<th>'; // Mode selection
				echo '<select name=mode'.($i).'>';
				echo '<option class="btn" '; echo (!strcmp($single_line[1],"0")) ? 'selected="selected"' : ''; echo ' value="0"><center>Access</center></option>';
				echo '<option class="btn" '; echo (!strcmp($single_line[1],"1")) ? 'selected="selected"' : ''; echo ' value="1"><center>Trunk</center></option>';
				echo '<option class="btn" '; echo (!strcmp($single_line[1],"2")) ? 'selected="selected"' : ''; echo ' value="2"><center>VLAN Disabled</center></option>';
				echo '<option class="btn" '; echo (!strcmp($single_line[1],"3")) ? 'selected="selected"' : ''; echo ' value="3"><center>Unqualified port</center></option>';
				echo '</select>'; // end mode

			echo '</th>';

			echo '<th>'; // Priority selection
				echo '<select name=prio'.($i).'>';
				echo '<option class="btn"  '; echo (!strcmp($single_line[4],"0")) ? 'selected="selected"' : ''; echo 'value="0"><center>0</center></option>';
				echo '<option class="btn"  '; echo (!strcmp($single_line[4],"1")) ? 'selected="selected"' : ''; echo 'value="1"><center>1</center></option>';
				echo '<option class="btn"  '; echo (!strcmp($single_line[4],"2")) ? 'selected="selected"' : ''; echo 'value="2"><center>2</center></option>';
				echo '<option class="btn"  '; echo (!strcmp($single_line[4],"3")) ? 'selected="selected"' : ''; echo 'value="3"><center>3</center></option>';
				echo '<option class="btn"  '; echo (!strcmp($single_line[4],"4")) ? 'selected="selected"' : ''; echo 'value="4"><center>4</center></option>';
				echo '<option class="btn"  '; echo (!strcmp($single_line[4],"5")) ? 'selected="selected"' : ''; echo 'value="5"><center>5</center></option>';
				echo '<option class="btn"  '; echo (!strcmp($single_line[4],"6")) ? 'selected="selected"' : ''; echo 'value="6"><center>6</center></option>';
				echo '<option class="btn"  '; echo (!strcmp($single_line[4],"7")) ? 'selected="selected"' : ''; echo 'value="7"><center>7</center></option>';

				echo '</select>'; // end Priority

			echo '</th>';

			echo '<th align=center><INPUT type="text" size="8" name="mask'.$i.'" ></th>';

			echo '</tr>';
		}

		echo '<tr><th></th><th></th><th></th><th></th><th align=center><input type="submit" value="Update" class="btn" name="updatevlan" ></th></tr>
					</form>';

		echo '</table>';

		echo '<br>'.$_POST['mode0'];
		//Parse input and run the command
		if (!empty($_POST['updatevlan'])){
			$vlan_cmd= "/wr/bin/wrs_vlans ";

			for($i = 0; $i < 18; $i++){
				//if(strcmp($_POST['vlan'.$i],"disabled")){ //VLAN selected
					$vlan_cmd .= " --port ".($i+1);
					$vlan_cmd .= " --pmode ".$_POST['mode'.$i];
					$vlan_cmd .= " --pprio ".$_POST['prio'.$i];
					if(!empty($_POST['vlan'.$i])){$vlan_cmd .= " --pvid ".$_POST['vlan'.$i];}
					if(!empty($_POST['mask'.$i])){$vlan_cmd .= " --pumask ".$_POST['mask'.$i];}
					$output = shell_exec($vlan_cmd);
					echo $vlan_cmd;
					echo '<br><p><center>Port wri'.($i+1).' added to VLAN'.$_POST['vlan'.$i].'</center></p>';

				//}else{
					if(!strcmp($_POST['mode'.$i],"2")){ //Disable VLAN for endpoint
						$vlan_cmd .= " --port ".($i+1);
						$vlan_cmd .= " --pmode ".$_POST['mode'.$i];
						$output = shell_exec($vlan_cmd);
						echo '<br><p><center>VLAN removed for port wri'.($i+1).'</center></p>';
					}

				//}

				$vlan_cmd= "/wr/bin/wrs_vlans ";
			}
			/* redirect to vlan.php */
			header('Location: vlan.php');
		}

	?>
	<br><br><FORM align="right" method="POST" action="vlan.php" ENCTYPE="multipart/form-data">
			<INPUT type=submit value="Go back" class="btn" >
	</form>

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
