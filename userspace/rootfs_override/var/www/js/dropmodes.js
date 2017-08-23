/*
 * Javascripts for endpointmode.php
 *
 * @author Anne M. Muñoz <amunoz@sevensols.com>
 *
 * Depending on the dropdown list option selected updates the port status
 * without the need of procesing a POST call in PHP
 * The process is controlled by javascript, avoiding to reload the page
 * improving user experience
 *
 * Result: A Dropdown List with the choices for the port status
 *
 */

$(document).ready(function() {
    $('.drop').on('change', set_endpoint_mode);
});

function set_endpoint_mode(){
    var selected = $("#"+this.id).val();
    var wri = "wri" + this.id.replace( /^\D+/g, '');
    
    console.log(selected, wri);
    $.ajax({
        url: 'modifymode.php',
        type: 'GET',
        data: {
            wri : wri,
            mode : selected
        }
    });
}

