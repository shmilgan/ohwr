<?php include 'title.php'; ?>
<body>

<div class="page">
    <div class="left-bar">
        <div class="menu">
			<?php include 'menu.php'; ?>
        </div>
    </div>
    <div class="right-bar">
        <div class="header"><?php include 'header.php'; ?></div>
        <div class="content">
			<?php 
				$cmd =  htmlspecialchars($_POST["cmd"]); 
				
				
				if(!strcmp($cmd, "halt")){
					echo '<br><br><br></be>System is halting';
				}else if (!strcmp($cmd, "reboot")){
					echo '<br><br><br>System is rebooting. Please wait 30 seconds.';
				}
				
				$output = shell_exec($cmd);
				
			?>

        </div>
        <div class="footer"><?php include 'footer.php'; ?></div>
        
    </div>
</div>
</body>
</html>
