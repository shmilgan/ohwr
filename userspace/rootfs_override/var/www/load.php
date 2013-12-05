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

			<br><br><br>
			
			<div>
			<FORM method="POST" ENCTYPE="multipart/form-data">
			Load FPGA binary		  <INPUT type=file name="file">
									  <INPUT type=submit value="Load">
									  <INPUT type=hidden name=MAX_FILE_SIZE  VALUE=8192>
			</FORM>
			</div>

			
			
			
			
			<?  

				$uploaddir = '/tmp/';
				$uploadfile = $uploaddir . basename($_FILES['file']['name']);
				echo '<pre>';
				if (move_uploaded_file($_FILES['file']['tmp_name'], $uploadfile)) {
					echo "File is valid, and was successfully uploaded.\n";
				} //else {
					//echo "Possible file upload attack!\n";
				//}

				//echo 'Here is some more debugging info:';
				//print_r($_FILES);

				print "</pre>";

				
			?>
			
			<?  include 'functions.php'; wrs_check_writeable(); ?>
			
        </div>
        <div class="footer"><?php include 'footer.php'; ?></div>
        
    </div>
</div>
</body>
</html>

