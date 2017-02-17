function addColorRows(name){
        $("table" + name + " tbody tr:nth-child(odd)").addClass("oddrowcolor");
        $("table" + name + " tbody tr:nth-child(even)").addClass("evenrowcolor");
}

function columnNonEditable(id,column){
        $(id+" tr > :nth-child("+column+") input").prop("readonly", true);
}

$( document ).ready(function() {
	addColorRows("#alternatecolor1");
	columnNonEditable("#alternatecolor1",2);
});
