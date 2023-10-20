$().ready(function (){
  if($("#pir_enabled_check").is(":checked")){
    $("#pir_config").css("display", "block");
  }
  $("#pir_enabled").click(function (){
    if($("#pir_enabled_check").is(":checked")){
      $("#pir_config").css("display", "block");
    } else {
      $("#pir_config").css("display", "none");
    }
  });
});