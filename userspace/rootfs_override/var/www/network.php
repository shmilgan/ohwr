<?php include 'functions.php'; include 'head.php'; ?>
<body id="network">
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
<h1 class="title">Network Management <a href='help.php?help_id=network' onClick='showPopup(this.href);return(false);'><img align=right src="./img/question.png"></a></h1>

	<?php session_is_started() ?>
	<?php $_SESSION['advance']=""; ?>

	<?php

		if((empty($_POST["networkgroup"]))){
			
			echo '<FORM method="POST">
			<table id="daemon" border="0" align="center">	
					<tr>
						<td align=left>eth0 Setup: </td>
						<td><input type="radio" name="networkgroup" value="DHCPONLY"';  if(!strcmp(wrs_interface_setup(), "dhcponly")) echo "checked";
						echo ' > DHCP Only <br>
						<input type="radio" name="networkgroup" value="DHCPONCE"';  if(!strcmp(wrs_interface_setup(), "dhcponce")) echo "checked";
						echo ' > DHCP + Static<br>
							<input type="radio" name="networkgroup" value="STATIC"';  if(!strcmp(wrs_interface_setup(), "static")) echo "checked";
						echo ' > Static IP<br></td>
						<td><INPUT type="submit" value="Change" class="btn"></td>	
					</tr>
			</table>				
			</FORM>
			<br>';
	
			$formatID = "alternatecolor";
			$class = "altrowstable firstcol";
			$infoname = "Current eth0";
			$format = "table";
			$section = "WRS_FORMS";
			$subsection = "NETWORK_SETUP";
			
			print_info($section, $subsection, $formatID, $class, $infoname, $format);
			
			echo '<br>';
			$formatID = "alternatecolor1";
			$class = "altrowstable firstcol";
			$infoname = "DNS Configuration";
			$format = "table";
			$section = "WRS_FORMS";
			$subsection = "DNS_SETUP";
			
			print_info($section, $subsection, $formatID, $class, $infoname, $format);
			
			echo '<hr><p align="right">Click <A HREF="dns.php">here</A> to modify DNS configuration</p>';
		}

		if ((!empty($_POST["networkgroup"])) && (!strcmp(htmlspecialchars($_POST["networkgroup"]),"DHCPONLY"))){
			
			$_SESSION["KCONFIG"]["CONFIG_ETH0_DHCP"]="y";
			
			check_add_existing_kconfig("CONFIG_ETH0_DHCP=");
			
			delete_from_kconfig("CONFIG_ETH0_DHCP_ONCE=");
			delete_from_kconfig("CONFIG_ETH0_STATIC=");
			delete_from_kconfig("CONFIG_ETH0_IP=");
			delete_from_kconfig("CONFIG_ETH0_MASK=");
			delete_from_kconfig("CONFIG_ETH0_NETWORK=");
			delete_from_kconfig("CONFIG_ETH0_BROADCAST=");
			delete_from_kconfig("CONFIG_ETH0_GATEWAY=");

			save_kconfig();
			apply_kconfig();
			
			$formatID = "alternatecolor";
			$class = "altrowstable firstcol";
			$infoname = "Current eth0";
			$format = "table";
			$section = "WRS_FORMS";
			$subsection = "NETWORK_SETUP";
			
			print_info($section, $subsection, $formatID, $class, $infoname, $format);
			
			echo '<br><div id="alert"><center>"DHCP Only" is now set for eth0<br>
				Changes will take place after reboot.</center></div>';
			echo '<form action="reboot.php">
						<INPUT style="float: right;" type="submit" value="Reboot Now" class="btn last">
						</form>';
		}
		
		if ((!empty($_POST["networkgroup"])) && (!strcmp(htmlspecialchars($_POST["networkgroup"]),"DHCPONCE"))){

			echo '<p>Please enter the static setup in case DHCP fails: </p><br>';
			
			echo '<FORM method="POST">
					<table border="0" align="center" class="altrowstable" id="alternatecolor">	
						<tr>
							<td>IP Address: </td>
							<td><INPUT type="text" value="192.168.1.10" name="ip" ></td>
						</tr>
						<tr>
							<td>Netmask: </td>
							<td><INPUT type="text" value="255.255.255.0" name="netmask" ></td>
						</tr>
						<tr>
							<td>Broadcast: </td>
							<td><INPUT type="text" value="192.168.1.255" name="broadcast" ></td>
						</tr>
						<tr>
							<td>Network: </td>
							<td><INPUT type="text" value="192.168.1.0" name="network" ></td>
						</tr>
						<tr>
							<td>Gateway: </td>
							<td><INPUT type="text" value="192.168.1.1" name="gateway" ></td>
						</tr>
					</table>
					<INPUT type="submit" value="Save New Configuration" class="btn last">
					<INPUT type="hidden" value="DHCPONCE" name="dhcp">
					</FORM>';
		}
		

		if ((!empty($_POST["networkgroup"])) && (!strcmp(htmlspecialchars($_POST["networkgroup"]),"STATIC"))){

			echo '<FORM method="POST">
					<table border="0" align="center" class="altrowstable" id="alternatecolor">	
						<tr>
							<td>IP Address: </td>
							<td><INPUT type="text" value="192.168.1.10" name="ip" ></td>
						</tr>
						<tr>
							<td>Netmask: </td>
							<td><INPUT type="text" value="255.255.255.0" name="netmask" ></td>
						</tr>
						<tr>
							<td>Broadcast: </td>
							<td><INPUT type="text" value="192.168.1.255" name="broadcast" ></td>
						</tr>
						<tr>
							<td>Network: </td>
							<td><INPUT type="text" value="192.168.1.0" name="network" ></td>
						</tr>
						<tr>
							<td>Gateway: </td>
							<td><INPUT type="text" value="192.168.1.1" name="gateway" ></td>
						</tr>
					</table>
					<INPUT type="submit" value="Save New Configuration" class="btn last">
					</FORM>';
			
		}

		if ((!empty($_POST["ip"])) && (!empty($_POST["netmask"])) && (!empty($_POST["broadcast"]))){

			if (!empty($_POST["dhcp"])){
				$_SESSION["KCONFIG"]["CONFIG_ETH0_DHCP_ONCE"]="y";
				
				$_SESSION["KCONFIG"]["CONFIG_ETH0_IP"]=$_POST["ip"];
				$_SESSION["KCONFIG"]["CONFIG_ETH0_MASK"]=$_POST["netmask"];
				$_SESSION["KCONFIG"]["CONFIG_ETH0_NETWORK"]=$_POST["network"];
				$_SESSION["KCONFIG"]["CONFIG_ETH0_BROADCAST"]=$_POST["broadcast"];
				$_SESSION["KCONFIG"]["CONFIG_ETH0_GATEWAY"]=$_POST["gateway"];
				
				check_add_existing_kconfig("CONFIG_ETH0_DHCP_ONCE=");
				check_add_existing_kconfig("CONFIG_ETH0_IP=");
				check_add_existing_kconfig("CONFIG_ETH0_MASK=");
				check_add_existing_kconfig("CONFIG_ETH0_NETWORK=");
				check_add_existing_kconfig("CONFIG_ETH0_BROADCAST=");
				check_add_existing_kconfig("CONFIG_ETH0_GATEWAY=");
				
				delete_from_kconfig("CONFIG_ETH0_DHCP=");
				delete_from_kconfig("CONFIG_ETH0_STATIC=");
				
				echo '<br><div id="alert"><center>"DHCP Once" is now set for eth0<br>
					Changes will take place after reboot.</center></div>';
				echo '<form action="reboot.php">
						<INPUT style="float: right;" type="submit" value="Reboot Now" class="btn last">
						</form>';
				
			}else{
				$_SESSION["KCONFIG"]["CONFIG_ETH0_STATIC"]="y";
				
				$_SESSION["KCONFIG"]["CONFIG_ETH0_IP"]=$_POST["ip"];
				$_SESSION["KCONFIG"]["CONFIG_ETH0_MASK"]=$_POST["netmask"];
				$_SESSION["KCONFIG"]["CONFIG_ETH0_NETWORK"]=$_POST["network"];
				$_SESSION["KCONFIG"]["CONFIG_ETH0_BROADCAST"]=$_POST["broadcast"];
				$_SESSION["KCONFIG"]["CONFIG_ETH0_GATEWAY"]=$_POST["gateway"];
				
				check_add_existing_kconfig("CONFIG_ETH0_STATIC=");
				check_add_existing_kconfig("CONFIG_ETH0_IP=");
				check_add_existing_kconfig("CONFIG_ETH0_MASK=");
				check_add_existing_kconfig("CONFIG_ETH0_NETWORK=");
				check_add_existing_kconfig("CONFIG_ETH0_BROADCAST=");
				check_add_existing_kconfig("CONFIG_ETH0_GATEWAY=");
				
				delete_from_kconfig("CONFIG_ETH0_DHCP_ONCE=");
				delete_from_kconfig("CONFIG_ETH0_DHCP=");
				
				
				echo '<br><div id="alert"><center>"Static Only" is now set for eth0<br>
					Changes will take place after reboot.</center></div>';
				echo '<form action="reboot.php">
						<INPUT style="float: right;" type="submit" value="Reboot Now" class="btn last">
						</form>';
			}
			
			save_kconfig();
			apply_kconfig();

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
