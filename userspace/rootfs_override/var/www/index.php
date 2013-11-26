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
				 <?php
					include 'functions.php';
					wrs_main_info();		 
				 ?>
				
			 </div>
        

        </div>
        <div class="footer"><?php include 'footer.php'; ?></div>
        
    </div>
</div>
</body>
</html>

