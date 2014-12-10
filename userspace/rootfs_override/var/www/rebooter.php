<?php
	include 'functions.php';
	session_start();
	ob_start();
	session_is_started();
	
	// This php file must be only called from reboot.php
	if(!empty($_SERVER['HTTP_REFERER'])){ 
		shell_exec("reboot");
	}else{
		header('Location: index.php');
	}
?>
