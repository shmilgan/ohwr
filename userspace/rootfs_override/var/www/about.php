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
			<div>
			<br><br><br><br><br>
			<a href='www.ugr.es'><IMG SRC='img/ugr.gif' align=right , vspace=7, hspace=23 , width=350 , hight=100 , border=0 , alt='UGR'></a>
			<p>White-Rabbit switch Firmware v<?php $str = shell_exec("uname -v"); echo $str;  ?> </p>
			<p>Developed by José Luis Gutiérrez (jlgutierrez@ugr.es)</p>
			<p>Open Hardware Repository http://www.ohwr.org/projects/white-rabbit</p>
			<p>Built in <?php $str = shell_exec("/wr/bin/shw_ver -c"); echo $str; ?></p>
			
			</div>
		
		</div>
        <div class="footer"><?php include 'footer.php'; ?></div>
        
    </div>
</div>
</body>
</html>
