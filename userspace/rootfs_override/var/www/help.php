<?php include 'functions.php' ?>
<html id="help">
<head>
<title>Help Menu</title>
<link href="css/help.css" rel="stylesheet" type="text/css">
</head>
<body  bgcolor="#FFFF99">
<h1><center>Help</center></h1>	
<hr>
	
<?php
	wrs_display_help($_GET['help_id'],"");
?>
</body>

</html>
