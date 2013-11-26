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
			<br><br><br><br><br><br>
			<center>
				<form action="exe_program.php" method="post">
				<input type="hidden" name="cmd" value="halt">
				<input type="submit" value="Halt switch">
				</form>
				<form action="exe_program.php" method="post">
				<input type="hidden" name="cmd" value="reboot">
				<input type="submit" value="Reboot switch">
				</form>
			</center>
       
        </div>
        <div class="footer"><?php include 'footer.php'; ?></div>
        
    </div>
</div>
</body>
</html>
