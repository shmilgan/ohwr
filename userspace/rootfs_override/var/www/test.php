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
			 <div class="wrs-info" >


				<form action="cgi-bin/test.php" method="post">
				Command: <input type="text" name="cmd"><br>
				<input type="submit">
				</form>

			 </div>
        

        </div>
        <div class="footer"><?php include 'footer.php'; ?></div>
        
    </div>
</div>
</body>
</html>

