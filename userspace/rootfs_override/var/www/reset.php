<?php include 'functions.php'; include 'head.php'; ?>
<body id="management">
<div class="main">
<div class="page">
<div class="header" >
<!--<h1>White-Rabbit Switch Tool</h1>-->
<div class="header-ports" ><?php wrs_header_ports(); ?></div>
<div class="topmenu">
	<?php include 'topmenu.php' ?>
</div>
</div>
<div class="content">
<div class="leftpanel">
<h2>Main Menu</h2>
	<?php include 'menu.php' ?>
</div>
<div class="rightpanel">
<div class="rightbody">
<h1 class="title">Reset <a href='help.php?help_id=firmware' onClick='showPopup(this.href);return(false);'><img align=right src="./img/question.png"></a></h1>


<?php
	session_is_started();
?>

	<p>This will reset your switch to the default state</p>
	<br>
	<FORM method="POST" ENCTYPE="multipart/form-data" onsubmit="return confirm('Are you sure you want to reset the switch?');">
        <th><INPUT type=submit value="Reset" class="btn" name="reset" ></th>
        </FORM>              

</div>
</div>
</div>
<div class="footer">
<?php 
	if(isset($_POST['reset'])){
		resetswitch();
    	}

 ?>
</div>
</div>
</div>
</body>
</html>
