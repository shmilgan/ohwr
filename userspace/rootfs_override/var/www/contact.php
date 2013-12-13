<?php include 'functions.php'; include 'head.php'; ?>
<body>
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
<h1 class="title">Contact</h1>

	
	<p><strong>White-Rabbit switch Firmware v<?php $str = shell_exec("uname -v"); echo $str;  ?> </strong></p><p>&nbsp;</p>
	<p><strong>Open Hardware Repository  <a href="http://www.ohwr.org/projects/white-rabbit/wiki">http://www.ohwr.org/projects/white-rabbit/wiki</a> </strong></p></strong></p><p>&nbsp;</p>
	<p><strong>Built in <?php $str = shell_exec("/wr/bin/shw_ver -c"); echo $str; ?></strong></p><p>&nbsp;</p>
	
	<a href='http://www.ugr.es'><IMG SRC='img/ugr.gif' align=right  width=250 , hight=100 , border=0 , alt='UGR'></a><p>&nbsp;</p><p>&nbsp;</p><p>&nbsp;</p><p>&nbsp;</p><p>&nbsp;</p><p>&nbsp;</p>
	<p align=right><strong>Developed by Jos&eacute; Luis Guti&eacute;rrez <a href="mailto:jlgutierrez@ugr.es?subject=[White-Rabbit Switch Local Management Tool]">(jlgutierrez@ugr.es)</a> </strong></p>

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
