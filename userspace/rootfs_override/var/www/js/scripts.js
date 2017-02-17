/*
 * Displays HTML tables in two colors
 * 
 * @author José Luis Gutiérrez <jlgutierrez@ugr.es>
 *
 * Displays two different colors (gray and blue) for tables and forms.
 * It allows two tables per page.
 * 
 */
function altRows(id){
	if(document.getElementsByTagName){  
		var table = document.getElementById(id);  
		try{
			var rows = table.getElementsByTagName("tr"); 
			for(i = 0; i < rows.length; i++){
                        	if(i % 2 == 0){
                                	rows[i].className += " evenrowcolor";
                        	}else{
                                	rows[i].className += " oddrowcolor";
                        	}
                	}

                	var table = document.getElementById(id+1);
                	var rows = table.getElementsByTagName("tr");

                	for(i = 0; i < rows.length; i++){
                        	if(i % 2 == 0){
                                	rows[i].className += " evenrowcolor";
                        	}else{
                                	rows[i].className += " oddrowcolor";
                        	}
                	}

		}
		catch(err){
			console.log(err);
		}
	}
}
window.onload=function(){
	altRows('alternatecolor');
}

/*
 * Opens the help message
 * 
 * @author José Luis Gutiérrez <jlgutierrez@ugr.es>
 *
 * Opens the help message for each page.
 * 
 */
function showPopup(url) {
	newwindow=window.open(url,'name','height=250,width=520,top=200,left=300,resizable');
	if (window.focus) {
		newwindow.focus()
	}
}

/*
 * Reboots the switch
 * 
 * @author José Luis Gutiérrez <jlgutierrez@ugr.es>
 * @author Benoit Rat <benoit@sevensols.com>
 *
 * Reboots the switch using the rebooter.php file.
 * This has been done to display the "rebooting" message after any
 * configuration.
 * 
 */
$(document).ready(
		function() {
			setTimeout(function() {
				var pageid=$('body').attr('id');
				if (pageid=="reboot")
				{
					//Obtain timeout from the hidden input (#reboot_to) or set it by default to 40s
					tout=$('#reboot_to').val()
					if($.isNumeric(tout)==false || tout < 30) tout=40
					
					//Improve how to print the timeout when more than 1 minute
					if(tout>60) tout_str=Math.floor(tout/60)+"m "+(tout%60)+"s."
					else tout_str=tout+"s."
					
					//Update the HTML and call the rebooter.
					$('#rebootingtext').text("Rebooting WRS. The web interface will refresh automatically after "+tout_str);
					$('#rebooting').load('rebooter.php');
				}
			}, 1500);
		});

/*
 * Redirects users to index.php
 * 
 * @author José Luis Gutiérrez <jlgutierrez@ugr.es>
 * @author Benoit Rat <benoit@sevensols.com>
 *
 * tout seconds after the execution of the reboot cmd, the web browser 
 * reloads automatically the web interface.
 * 
 */
$(document).ready(
		function() {
			var pageid=$('body').attr('id');
			if (pageid=="reboot")
			{
				//Obtain timeout from the hidden input (#reboot_to) or set it by default to 40s
				tout=$('#reboot_to').val()
				if($.isNumeric(tout)==false || tout < 30) tout=40
			
			//Main timeout to refresh the webpage
			setTimeout(function() {
				window.location.href = "index.php";
			}, tout*1000);
			}
		});
