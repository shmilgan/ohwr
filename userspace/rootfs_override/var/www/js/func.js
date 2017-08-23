$( document ).ready(function() {
	addColorRows("#alternatecolor1");
        columnNonEditable("#alternatecolor1",2);
        changeMode();
});

function addColorRows(name){
        $("table" + name + " tbody tr:nth-child(odd)").addClass("oddrowcolor");
        $("table" + name + " tbody tr:nth-child(even)").addClass("evenrowcolor");
}

function columnNonEditable(id,column){
        $(id+" tr > :nth-child("+column+") input").prop("readonly", true);
}

function changeMode(){
        if($("#endpointconfig").length != 0) {
                $(".drop").on('change', function() {
                        $(this).attr('value',$(this).val());
                });
                $(".checkbox").on('change', function() {
                        if($(this).val()=="n")
                                $(this).attr('value',"y");
                        else
                               $(this).attr('value',"n"); 
                });        
        }
          
}


