<?php include 'functions.php'; include 'head.php'; ?>
<body id="sfpcalib">
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
<h1 class="title">SFP Database <a href='http://www.ohwr.org/projects/white-rabbit/wiki/Calibration' target='_blank'><img align=right src="./img/question.png"></a></h1>

<?php session_is_started() ?>

	<br>
	

	<?php 
	
		$file_init = '-- The VENDOR_SERIAL field can be set to an empty string to provide class-level'."\n";
		$file_init .= '-- information as opposed to device-level information.'."\n\n";
		$file_init .= 'sfpdb = {'."\n";
			
		$file_end = '}'."\n";;
		
		
		if (empty($_POST['number'])){
			echo '<div><form method="post">
				Number of new SPFs to be added: <INPUT type="text" value="2" name="number" > 
				<input type="submit" value="Add" class="btn">
				</form></div>';
				
			echo "<div id='bottommsg'>
				<hr>
				NOTE: This action will delete the previous sfp_database.conf file. <a href='showfile.php?help_id=file&name=sfp_database.conf'
				onClick='showPopup(this.href);return(false);'> (see current configuration) </a>
				<center>**If you do not know how to calibrate SFPs please click on <a href='http://www.ohwr.org/projects/white-rabbit/wiki/Calibration' target='_blank'> here </a>**</center>
				</div>";
		}else{
			
			echo '*all fields are mandatory<br><br>';
			echo '<form method="post">';
			echo "<div><table class='altrowstable firstcol' id='alternatecolor'>";
			//echo '<tr><td> <b><center> </center></b></td></tr>';

			
			if (($_POST['number']!=2)){
				for($i=0; $i<$_POST['number']; $i++){

					echo '<tr><th>SFP '.($i+1).'</th></tr>';
					echo '<tr><td>Part Number</td><td><INPUT type="text" name="sfpnumber'.$i.'" > </td></tr>';
					echo '<tr><td>Alpha</td><td><INPUT type="text" name="alpha'.$i.'" > </td></tr>';
					echo '<tr><td>Delta Tx</td><td><INPUT type="text" name="tx'.$i.'" > </td></tr>';
					echo '<tr><td>Delta Rx</td><td><INPUT type="text" name="rx'.$i.'" > </td></tr>';
					
				}
			}else{
				
				for($i=0; $i<$_POST['number']; $i++){

					echo '<tr><th>SFP '.($i+1).'</th></tr>';
					
					if($i==0){
						echo '<tr><td>Part Number</td><td><INPUT type="text" value = "AXGE-1254-0531" name="sfpnumber'.$i.'" > </td></tr>';
						echo '<tr><td>Alpha</td><td><INPUT type="text" value="2.67871791665542e-04" name="alpha'.$i.'" > </td></tr>';
						echo '<tr><td>Delta Tx</td><td><INPUT type="text" value="10" name="tx'.$i.'" > </td></tr>';
						echo '<tr><td>Delta Rx</td><td><INPUT type="text" value="10" name="rx'.$i.'" > </td></tr>';
					}else{
						echo '<tr><td>Part Number</td><td><INPUT type="text" value = "AXGE-3454-0531" name="sfpnumber'.$i.'" > </td></tr>';
						echo '<tr><td>Alpha</td><td><INPUT type="text" value="-2.67800055584799e-04" name="alpha'.$i.'" > </td></tr>';
						echo '<tr><td>Delta Tx</td><td><INPUT type="text" value="10"  name="tx'.$i.'" > </td></tr>';
						echo '<tr><td>Delta Rx</td><td><INPUT type="text" value="10"  name="rx'.$i.'" > </td></tr>';
					}
					
				}
			}
			echo '</table>';
			echo '<input type="hidden" name="newconf" value="newconf">';
			echo '<input type="hidden" name="newnumber" value="'.$_POST['number'].'">';
			echo '<input type="submit" value="Create new file & Reboot" class="btn last">';
			echo '</form>';
			echo '</div>';
			
		}
		
		if (!empty($_POST['newconf'])){
			$outputfile = $file_init;
			$error = false;
			for($i=0; $i<$_POST['newnumber'] && (!$error); $i++){
				
				if(empty($_POST['sfpnumber'.$i]) || empty($_POST['alpha'.$i]) || empty($_POST['tx'.$i]) || empty($_POST['rx'.$i])){
					$error = true;
				}
				$outputfile .= "  {"."\n";
				$outputfile .= "\t".'part_num = "'.$_POST['sfpnumber'.$i].'",'."\n";
				$outputfile .= "\t"."alpha = ".$_POST['alpha'.$i].','."\n";
				$outputfile .= "\t"."delta_tx = ".$_POST['tx'.$i].','."\n";
				$outputfile .= "\t"."delta_rx = ".$_POST['rx'.$i]."\n";
				$outputfile .= "  },"."\n\n";
			}
			$outputfile .= $file_end;
			
		}
			

		if(!empty($_POST['newconf']) && !$error){
			//We save the changes in a temporarely file in /tmp
			$file = fopen("/tmp/sfp_database.conf","w+");
			fwrite($file,$outputfile);
			fclose($file);
			
			//We move the file to /wr/etc/
			copy('/tmp/sfp_database.conf',$GLOBALS['etcdir'].'sfp_database.conf');
			
			echo '<center><font color="green">File successfully created. Rebooting switch. </font></center>';
			
			wrs_reboot();
			
		}else if(!empty($_POST['newconf']) && $error){
			echo '<center><font color="red">WARNING: Conf. file not created. Please fill in all fields</font></center>';
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
