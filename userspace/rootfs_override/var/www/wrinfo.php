<?php include 'functions.php'; include 'head.php'; ?>
<body id="management">
<div class="main">
<div class="page">
<div class="header" >
<script type="text/javascript" src="js/loadwrinfo.js"></script>
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
<h1 class="title">WR Live Information <a href='help.php?help_id=wrinfo' onClick='showPopup(this.href);return(false);'><img align=right src="./img/question.png"></a></h1>


<?php
	session_is_started();
?>

<div id="wrstatus"></div>

</div>
</div>
</div>
<div class="footer">
	<?php include 'footer.php' ?>
</div>
</div>
</div>
</body>
</html>
