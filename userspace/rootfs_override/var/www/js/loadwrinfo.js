var datainfowr;

get_wrinfo();

var intervalwr = setInterval(get_wrinfo, 5000);

function get_wrinfo(){
	$.ajax({
                url: "getwrstatus.php",
                type: "POST",
                data: "data",
                success: function (data) {
		    datainfowr = JSON.parse(data);
		    updateWrValues()
                },
                error: function () {
		    clearInterval(intervalwr);
                }
            }); // AJAX Get
}

function updateWrValues(){
	var tables ="";

        //table temperatures + time
        tables += drawrowtable(removewhitespaces(datainfowr[1]));
	tables += '<br>';

	//table ports status
	tables += drawportstable(datainfowr[0]);
	tables += '<br>';
	
	//table timing parameters & sync status
        var data = datainfowr[2]+ ','+removewhitespaces(datainfowr[3]);
	tables += drawtimingtable(data);
	tables += '<br>';
	
	$("#wrstatus").html(tables);
	 addcolorrows();
}

function removewhitespaces(darray){
	darray = darray.filter(function(str) {
    		return /\S/.test(str);
	});
	return darray;
}

function drawrowtable(data){
	var names=[["FGPA","fgpa"],["PLL","pll"],["Left power supply", "psl"],["Right power supply", "psr"]];

	var tablewr = '<div><table class="altrowstable firstcol" width="100%" id="alternatecolor">';
	tablewr += '<tr><th>Temperatures</th></tr>';
	for (var i = 1; i < data.length; i++) {
		data[i] = data[i].split(":");
		tablewr += '<tr><td>' + names[i-1][0] + '</td><td>' + data[i][1] +" &degC" +'</td></tr>';
        }
        tablewr += '</table></div>';
        return tablewr;
}

function drawtimingtable(data){
	var names=[["WR time", "TAI"],["Switch time","UTC"],["Servo state", "ss"],["Round-trip time (mu)", "mu"],
		["Master-slave delay","dms"],["Master PHY delays TX", "dtxm"], ["Master PHY delays RX", "drxm"],
		["Slave PHY delays TX", "dtxs"],["Slave PHY delays RX", "drxs"], ["Total link asymmetry","asym"],
		["Estimated link length","ll"],
		["Clock offset", "cko"],["Phase setpoint","setp"],["Servo update counter","ucnt"]];
	
	var tablewr = '<div><table class="altrowstable firstcol" width="100%" id="alternatecolor">';
	tablewr += '<tr><th>Timing parameters</th></tr>';
	
	var j=0;
	data = data.split(",");
	for (var i = 0; i < data.length; i++) {
		if((data[i].match(/:/g) || []).length>1){
			tablewr += '<tr><td>' + names[j][0] + '</td><td>' + data[i] + '</td></tr>';
			j++;
		}	
		else if((data[i].match(/:/g) || []).length==1){	
			data[i] = data[i].split(":");
			if(data[i][0].indexOf("ss")>=0){
				data[i][1] = data[i][1].replace("\'",'');
				data[i][1] = data[i][1].replace("\'",'');
				tablewr += '<tr><td>' + names[j][0] + '</td><td>' + data[i][1] + '</td></tr>';
                                j++;
			}
			else if(data[i][0].indexOf("ucnt")>=0){
				tablewr += '<tr><td>' + names[j][0] + '</td><td>' + data[i][1] + ' times' + '</td></tr>';
				j++;
			}
			else if(data[i][0].indexOf("ll")>=0){
				tablewr += '<tr><td>' + names[j][0] + '</td><td>' + data[i][1]/100 + ' m ' + '</td></tr>';
				j++;
			}
			else if(data[i][0].indexOf("sv")<0 && data[i][0].indexOf("crtt")<0 && data[i][0].indexOf("lock")<0){
				tablewr += '<tr><td>' + names[j][0] + '</td><td>' + data[i][1]/1000 + ' nsec' + '</td></tr>';
				j++;
			}	
		}
	}
	
	tablewr += '</table></div>';
	return tablewr;
}

function drawportstable(data){
	var tablewr = '<div><table class="altrowstable firstcol" width="100%" id="alternatecolor">';
        tablewr += '<tr><th>Port</th><th>Link</th><th>WRconf</th><th>Freq</th><th>Calibration</th><th>MAC of peer port</th>';
	for (var i = 0; i < data.length-1; i++) {
        	data[i] = data[i].split(" ");
		data[i] = removewhitespaces(data[i]);
		tablewr += '<tr>';	
	        for (var j = 0; j < 6; j++) {
			tablewr += '<td>' + data[i][j]+ '</td>';
        	}
		tablewr += '</tr>';
	}
        tablewr += '</table></div>';
        return tablewr;

}

function addcolorrows(){
	$("table#alternatecolor tbody tr:nth-child(odd)").addClass("oddrowcolor");
	$("table#alternatecolor tbody tr:nth-child(even)").addClass("evenrowcolor");
}

function columnNonEditable(id,column){
	$(id+" tr > :nth-child("+column+") input").prop("readonly", true);
}
