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

