<?php ob_start();
if(strcmp($_SESSION['LASTIME'],filectime($GLOBALS['kconfigfile'])) &&
		strcmp(basename($_SERVER['PHP_SELF']), "reboot.php")){
	echo "<script>
	alert('WARNING: Dotconfig has been modified. Realoading configuration...');
	window.location.href='index.php';
	</script>";
	load_kconfig();
}
if (isset($_SESSION['LAST_ACTIVITY'])
					&& (!empty($_SESSION["myusername"]))
					&& (time() - $_SESSION['LAST_ACTIVITY'] > 600)) {
    // last request was more than 10 minutes ago
    session_unset();
    session_destroy();
    header('Location: index.php');
}
$_SESSION['LAST_ACTIVITY'] = time();
?>
<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">
<html xmlns="http://www.w3.org/1999/xhtml">
<head>

  <title>White-Rabbit Local Management Tool</title>
  <meta http-equiv="Content-Type"
 content="text/html; charset=utf-8">
  <link href="css/style.css" rel="stylesheet" type="text/css">

  <!-- Javascript goes in the document HEAD -->
  <!-- Javascript are located in /js/scripts.js -->
  <script type="text/javascript" src="js/jquery-3.1.1.min.js"></script>
  <script type="text/javascript" src="js/jquery-3.1.1.js"></script>
  <script type="text/javascript" src="js/scripts.js"></script>
</head>
