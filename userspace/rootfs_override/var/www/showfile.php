<?php include 'functions.php' ?>
<html>
<head>
<title></title>
</head>
<body >
<h1><center><?php echo $_GET['name'];?></center></h1>	
<hr>
	
<?php
	wrs_display_help($_GET['help_id'], $_GET['name']);
?>
</body>

</html>
