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
<h1 class="title">White-Rabbit Switch Console <a href='help.php?help_id=console' onClick='showPopup(this.href);return(false);'><img align=right src="./img/question.png"></a></h1>

	<?php session_is_started() ?>

	<FORM action="terminal.php" method="POST" accept-charset="UTF-8">
	Unix Command: <input type="text" name="cmd">
	<input type="submit" value="Enter" class="btn">
	</FORM>
	

	<?php
	wrs_change_wrfs("rw");
	/*$path = shell_exec("echo $PATH");
	if(substr_count($path , "/wr/bin")==0){
		//$new_path = shell_exec("export PATH=/wr/bin:$PATH");
		$path = htmlspecialchars(shell_exec("echo $PATH"));
		echo "PATH=".$path;
	}*/
	
	// If pwd is empty, we go to the root directory
	if(empty($_SESSION["pwd"])){$_SESSION["pwd"]="/";}
	shell_exec("cd ".$_SESSION["pwd"]);
	
	$cmd = htmlspecialchars($_POST["cmd"]);
	
	
	// user is moving to another directory
	$cmd_aux = $cmd;
	if(substr_count($cmd_aux , "cd")>0){
		
		$cmd = " ls ";
		// moving backwards
		if(substr_count($cmd_aux , "cd ..")>0){
			
			$back = explode("/", $_SESSION["pwd"]);
			$size = count($back);
			$_SESSION["pwd"] = str_replace("/".$back[$size-1],"",$_SESSION["pwd"]);
			
		// moving forwards
		}else{
			$cmd_aux = explode(" ", $cmd_aux);
			
			if(!strstr($cmd_aux[1][0],"/")){
								
				if(file_exists($_SESSION["pwd"]."/".$cmd_aux[1])){

					if (!strcmp($_SESSION["pwd"], "/")){
						$_SESSION["pwd"].= $cmd_aux[1];
					}else{
						$_SESSION["pwd"].= "/".$cmd_aux[1];
					}
					
				}else{
					
					$output = "folder not found";
					
				}

				
			}else{
				
				if(file_exists($cmd_aux[1])){
					$_SESSION["pwd"] = $cmd_aux[1];
					
				}else{
					$output = "folder not found";
				}
				
			}
			
		}
	}else{
		
	}

	$path = "PATH=/sbin:/usr/sbin:/bin:/usr/bin:/wr/bin";
	
	if(strstr($output,"folder not found")){
		$output = 'folder "'.$cmd_aux[1].'" not found';
		$cmd = "";
	}else{
		$output = shell_exec( $path." ; cd ".$_SESSION["pwd"]." ; ".$cmd);
	}
	
	echo '<div align="center"> <div id="preview" style= "BORDER-RIGHT: #000 1px solid; PADDING-RIGHT: 0px; 
		BORDER-TOP: #000 1px solid; PADDING-LEFT: 2px; PADDING-BOTTOM: 2px; WORD-SPACING: 1px; OVERFLOW: scroll; 
		BORDER-LEFT: #000 1px solid; WIDTH: 100%; PADDING-TOP: 1px; 
		BORDER-BOTTOM: #000 2px solid; HEIGHT: 350px; TEXT-ALIGN: left"> 
		<p>'.$_SESSION["pwd"].'$:'.$cmd.'<br>'.$output.'</p> </div></div>';
		
		
	
		wrs_change_wrfs("ro");
	 ?>


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
