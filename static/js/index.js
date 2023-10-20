function getUrlParameter(name) {
    name = name.replace(/[\[]/, '\\[').replace(/[\]]/, '\\]');
    var regex = new RegExp('[\\?&]' + name + '=([^&#]*)');
    var results = regex.exec(location.search);
    return results === null ? '' : decodeURIComponent(results[1].replace(/\+/g, ' '));
};

// Cantidad de intentos restantes de almacenar esclavos en la db
var trys = parseInt(getUrlParameter('trys'));

function updateNodes(){
  window.location.href = '/connect-slaves/'+roomId+'?trys='+trys;
};

function updateGroupState(){
  $.ajax({
    url: "/group-state/"+roomId,
    success: function(response){
      handleGroupData(response);
      setTimeout(updateGroupState, 15000);
    },
    error: function(){
      console.log("ERROR EN AJAX");
    },
    timeout: 5000
  });
}

function handleGroupData(json){
  if(json != "no_groups"){
    $.each(JSON.parse(json), function (index, group) {
      elem = `<div id="group-${group.ID_GROUP}">
                <b>Grupo ${group.NAME}</b>`;
      if(group.ON_OFF == "on"){
        elem += ` Prendido <a href='/turnoff?room=${roomId}&group=${group.ID_GROUP}'>Apagar </a>`;
      } else {
        elem += ` Apagado <a href='/turnon?room=${roomId}&group=${group.ID_GROUP}'>Prender </a>`;
      }
      elem += `<i class="fa fa-gear"></i><a href="/groupsettings/${group.ID_GROUP}">  Configuración</a> &nbsp
                      <i class="fe fe-clock">&nbsp</i><a href="/alarm-list/${group.ID_GROUP}">Alarmas</a>
                      <br>
                    </div>`;
      $("#group-"+group["ID_GROUP"]).remove();
      $("#Grupos").append(elem);
    });
  }
}

// if(!isNaN(trys) && (trys > 0))
//   setTimeout(updateNodes, 15000); // Tratar de actualizar en 15 segundos
$().ready(function (){
  if(getUrlParameter('node_disconnected').localeCompare('') != 0){
    $('.alert-danger').css('display', 'block');
  }

  $('#connectTemp').click(function(){
  	if(movementSensorConnected)
  		return window.confirm("Esto desconectará el sensor de movimiento, ¿Está seguro?");
  });

  $('#connectPir').click(function(){
  	if(tempSensorConnected)
  		return window.confirm("Esto desconectará el sensor de temperatura y humedad, ¿Está seguro?");
  });

  setTimeout(updateGroupState, 15000);
});