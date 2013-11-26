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
			</br>

		</br></br>
		<FORM action="command.php" method="POST" accept-charset="UTF-8">
		Unix Command: <input type="text" name="cmd">
		<input type="submit" value="Submit">
		</FORM>
		</div>
        
        
        
        <div class="footer"><?php include 'footer.php'; ?></div>
        
    </div>
</div>
</body>
</html>


