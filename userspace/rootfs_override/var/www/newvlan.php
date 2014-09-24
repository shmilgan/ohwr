<?php include 'functions.php'; include 'head.php'; ?>
<body id="management">
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
<h1 class="title">VLAN Management <a href='help.php?help_id=vlan' onClick='showPopup(this.href);return(false);'><img align=right src="./img/question.png"></a></h1>

	<?php session_is_started() ?>
	<?php $_SESSION['advance']=""; ?>

	
	
	
	<?php 
		$vlan_cmd = "/wr/bin/wrs_vlans ";
		if(!empty($_POST['vid'])){ $vlan_cmd .= " --rvid ".$_POST['vid'];}
		if(!empty($_POST['fid'])){$vlan_cmd .= " --rfid ".$_POST['fid'];}
		if(!empty($_POST['mask'])){$vlan_cmd .= " --rmask ".$_POST['mask'];}
		if(!empty($_POST['drop'])){$vlan_cmd .= " --rdrop ".$_POST['drop'];}
		if(!empty($_POST['prio'])){$vlan_cmd .= " --rprio ".$_POST['prio'];}
		
		shell_exec($vlan_cmd);
		
		header('Location: vlan.php');
	
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
