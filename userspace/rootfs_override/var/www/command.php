<html>
<head>
<title>White Rabbit Switch Management Tool</title>
<meta http-equiv="Content-Type" content="text/html; charset=utf-8"/>
<link rel="stylesheet" href="wrs.css">
</head>
<body>

<div class="page">
    <div class="left-bar">
        <div class="menu">
			<p><a href="index.php"> Dashboard </a></p>
			<p>Monitor</p>
			<p>Configuration</p>
			<p><a href="tools.php"> Tools </a></p>
        </div>
    </div>
    <div class="right-bar">
        <div class="header"><?php include 'header.php'; ?></div>
        <div class="content">
	
	<?php
		echo '</br>Here is a list of white rabbit commands:';
		echo '</br>------------------------------------------</br>';
		$pre= shell_exec("ls /wr/bin/");
		echo '</br>'.$pre.'</br>';
		echo '</br>------------------------------------------</br>';
		echo '</br></br></br>';
		$cmd = htmlspecialchars($_POST["cmd"]);
		echo '$' . $cmd . ':';
		$output = shell_exec($cmd);
		echo "<pre>$output</pre>";
	?>
	</div>
        
        
        
        
        <div class="footer"><?php include 'footer.php'; ?></div>
        
    </div>
</div>
</body>
</html>


