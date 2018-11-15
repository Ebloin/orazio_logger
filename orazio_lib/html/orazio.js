// orazio JS simple javascript thing
var ws = new WebSocket('ws://'+self.location.host+'/','orazio-robot-protocol');

ws.onmessage = function(event) {
    document.getElementById('msgBox').innerHTML = event.data;
    document.getElementById('outMsg').value='';
}

function setVariable(variable)
{
    var id_name=variable.replace(/\./g,'_');
    id_name=id_name.replace(/\]/g,'_');
    id_name=id_name.replace(/\[/g,'_');
    
    console.log("setvariable " + id_name );
    ws.send("set " + variable + " "+ document.getElementById(id_name).value);
}

function sendPacket(packet)
{
    ws.send('send ' + packet);
}

function stop(packet){
    ws.send('set joint_control[0].control.mode 0 ');
    ws.send('set joint_control[0].control.speed 0 ');
    ws.send('set joint_control[1].control.mode 0 ');
    ws.send('set joint_control[1].control.speed 0 ');
    sendPacket('joint_control[0]');
    sendPacket('joint_control[1]');
}

function storeParams(packet){
    ws.send("store " + packet);
}

function fetchParams(packet){
    ws.send("fetch " + packet);
}

function requestParams(packet){
    ws.send("request " + packet);
}
