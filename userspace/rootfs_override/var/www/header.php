<?php
	
	$str = shell_exec("/wr/bin/wr_management ports");
	$ports = explode(" ", $str);

	echo "<a href='index.php'><IMG SRC='img/wr_logo.png' align=left , vspace=7, hspace=23 , width=150 , hight=100 , border=0 , alt='White Rabbit'></a>";
	echo "<table border='0' align='center' vspace=37 >";
	
	echo '<tr>';
	for($i=1; $i<18*4; $i=$i+4){
		
		if (strstr($ports[($i-1)],"up")){
			if (strcmp($ports[($i)],"Master")){
				echo '<th>'."<IMG SRC='img/master.png' align=left ,   width=50 , hight=50 , border=0 , alt='master'>".'</th>';
			}else{
				echo  '<th>'."<IMG SRC='img/slave.png' align=left ,  width=50 , hight=50 , border=0 , alt='slave'>".'</th>';
			}
		}else{
			echo '<th>'."<IMG SRC='img/linkdown.png' align=left ,  width=50 , hight=50 , border=0 , alt='down'>".'</th>';
		}
		
	}
	echo '</tr>';
	
	echo '<tr>';
	for($i=1; $i<18*4; $i=$i+4){
		
		if (!strstr($ports[($i+1)],"NoLock")){
			echo '<th>'."<IMG SRC='img/locked.png' align=center ,  width=20 , hight=20 , border=0 , alt='locked'>";
		}else{
			echo '<th>'."<IMG SRC='img/unlocked.png' align=center ,  width=20 , hight=20 , border=0 , alt='unlocked'>";
		}
		if (!strstr($ports[($i+2)],"Uncalibrated")){
			echo "<IMG SRC='img/check.png' align=center ,  width=20 , hight=20 , border=0 , alt='check'>".'</th>';
		}else{
			echo "<IMG SRC='img/uncheck.png' align=center ,  width=20 , hight=20 , border=0 , alt='uncheck'>".'</th>';
		}
	}
	echo '</tr>';
	
	echo '<tr>';
	echo "<a href='http://www.ohwr.org/projects/wr-switch-sw'><IMG SRC='img/ohr.png' align=right , vspace=7, hspace=23 , width=100 , hight=100 , border=0 , alt='OHR'></a>";
	echo '</tr>';
	
	echo '</table>';

?>
