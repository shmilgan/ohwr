<?php 
include 'functions.php';
ob_start();
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

if(isset($_SESSION['myusername'])) 
{
	phpinfo();
}
else
{
	header('Location: index.php');
}

?>
