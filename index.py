from flask import Flask, request, session, abort, url_for
from flask import render_template
from flask import request, redirect
from threading import Thread
from node import *
from math import floor
from time import sleep
import database as db
import functools
import os
import json
import hashlib
import binascii

app = Flask(__name__)
app.debug = True
app.secret_key = '2e4f882620e4306990699203'


# ----------------- ENCRIPTA CONTRASEÑA ------------------ #

def hash_password(password):
    salt = hashlib.sha256(os.urandom(60)).hexdigest().encode('ascii')
    pwdhash = hashlib.pbkdf2_hmac('sha512', password.encode('utf-8'),
                                  salt, 100000)
    pwdhash = binascii.hexlify(pwdhash)
    return (salt + pwdhash).decode('ascii')

def verify_password(stored_password, provided_password):
    """Verify a stored password against one provided by user"""
    salt = stored_password[:64]
    stored_password = stored_password[64:]
    pwdhash = hashlib.pbkdf2_hmac('sha512',
                                  provided_password.encode('utf-8'),
                                  salt.encode('ascii'), 
                                  100000)
    pwdhash = binascii.hexlify(pwdhash).decode('ascii')
    return pwdhash == stored_password

# ----------------- ENCRIPTA CONTRASEÑA ------------------ #
#pas=hash_password('SPU2019..')
#print(pas)
#a5006e0ec89c3a280ec0dfed565e7a6ec87353c28efb460716b45ec39e9dcb997eaaa9ada4293dd65c2f5f7cf4dfe8aad0ee126bae41d254ab3dfb44734e550298c1c6b0dfdbbaf28b115236b7c2014e60fc4f42597a7a2c52106b43617fe7ec

def session_required(fn):
    @functools.wraps(fn)
    def inner(*args, **kwargs):
        #user = db.get_user()
        if session.get('logged_in') and session['username']=='admin':
            return fn(*args, **kwargs)
        return redirect(url_for('index'))
    return inner

def session_required_visit(fn):
    @functools.wraps(fn)
    def inner(*args, **kwargs):
        if session.get('logged_in'):
            return fn(*args, **kwargs)
        return redirect(url_for('index'))
    return inner

@app.route('/', methods = ['GET']) 
def index():
    if not session.get('logged_in'):
        return render_template('login.html')
    else:
        rooms = db.get_all_rooms()
        return render_template('index.html', r = rooms)

@app.route('/login', methods=['POST'])
def login():
    #session['POST_USERNAME'] = request.form['email']
    #session['POST_PASSWORD'] = request.form['password']

    POST_USERNAME = str(request.form['email'])
    POST_PASSWORD = str(request.form['password'])
    usuario = db.get_user(POST_USERNAME)
    if usuario is not None:
        #Verificar base de datos si usuario y contraseña coinciden
        if (POST_USERNAME == str(usuario[0]) and verify_password(str(usuario[1]),POST_PASSWORD)):
            result = True
            session['username'] = POST_USERNAME
        else:
            result = False
        if result:
            session['logged_in'] = True
            session['id'] = os.urandom(12).hex()
        else:
            pass
    else:
        pass
    '''if usuario is not None:
        result = True
        session['username'] = POST_USERNAME
        session['logged_in'] = True
        session['id'] = os.urandom(12).hex() '''
    return redirect('/')
        
    

@app.route("/logout")
def logout():
    #session.pop('username', None)
    session.clear() #Clears the entire session
    return redirect('/')

@app.route('/register') 
def register():
    return render_template('register.html')

@app.route('/forgot-password') 
def forgot_password():
    return render_template('forgot-password.html')

@app.route('/new-once-alarm/<int:room_id>/<int:group_id>', methods = ['POST', 'GET'])
@session_required
def new_once_alarm(room_id, group_id):
    if request.method == 'POST':
        date = request.form['date']
        name = request.form['name']
        time_i = request.form['time_i']
        time_f = request.form['time_f']
        year, month, day = date.split("-")
        hour_i, minute_i = time_i.split(":")
        hour_f, minute_f = time_f.split(":")
        node = db.get_master_by_room(room_id)
        master = getNodeById(node['ID_NODE'])
        if master:
            if group_id != 0:
                group = db.get_group(group_id)
                alarm_id = master.addAlarmOnce( day, month, year, hour_i, minute_i,\
                    hour_f, minute_f, 3, group['NAME'])
                if alarm_id is not False:
                    db.create_once_alarm(date, hour_i, minute_i, hour_f, minute_f, name, group_id, alarm_id)
                    return redirect('/alarm-list/{0}'.format(group_id))
            else:
                alarm_id = master.addAlarmOnce( day, month, year, hour_i, minute_i, hour_f, minute_f, 1)
                if alarm_id is not False:
                    db.create_global_once_alarm(date, hour_i, minute_i, hour_f, minute_f, name, room_id, alarm_id)
                    return redirect('/global-alarm-list/{0}'.format(room_id))
        return redirect('/room/{0}?node_disconnected=1'.format(room_id))
    else:
        room = db.get_room_by_id(room_id)
        return render_template('new-once-alarm.html', group = group_id, room = room)



@app.route('/new-weekly-alarm/<int:room_id>/<int:group_id>', methods = ['POST', 'GET'])
@session_required
def new_weekly_alarm(room_id, group_id):
    if request.method == 'POST':
        name = request.form['name']
        time_i = request.form['time_i']
        time_f = request.form['time_f']
        hour_i, minutes_i = time_i.split(":")
        hour_f, minutes_f = time_f.split(":")
        day = request.form['day']
        node = db.get_master_by_room(room_id)
        master = getNodeById(node['ID_NODE'])
        if master:
            if group_id != 0:
                group = db.get_group(group_id)
                alarm_id = master.addWeeklyAlarm( day, hour_i, minutes_i, hour_f, minutes_f, 3, group['NAME'])
                if alarm_id is not False:
                    db.create_weekly_alarm(name, hour_i, hour_f, day, minutes_i, minutes_f, group_id, alarm_id)
                    return redirect('/alarm-list/{0}'.format(group_id))
            else:
                alarm_id = master.addWeeklyAlarm( day, hour_i, minutes_i, hour_f, minutes_f, 1)
                if alarm_id is not False:
                    db.create_global_weekly_alarm(name, hour_i, hour_f, day, minutes_i, minutes_f, room_id, alarm_id)
                    return redirect('/global-alarm-list/{0}'.format(room_id))
        return redirect('/room/{0}?node_disconnected=1'.format(room_id))
    else:
        room = db.get_room_by_id(room_id)
        return render_template('new-weekly-alarm.html',group = group_id, room = room)


@app.route('/alarm-list/<int:group_id>',  methods = ['POST', 'GET'])
@session_required_visit
def alarm_list(group_id):
    room = db.get_room_by_group(group_id)
    if request.method == 'GET' and request.args.get('id'):
        identificador = request.args.get('id')
        borrar = request.args.get('borrar')
        master = db.get_master_by_room(room['ID_ROOM'])
        node = getNodeByName(master['NAME'])
        if borrar == "1":
            alarm_id = db.get_once_alarm(identificador)['ALARM_ID']
            if node and node.deleteAlarm(alarm_id):
                db.delete_once_alarm(identificador)
            else:
                return redirect('/room/{0}?node_disconnected=1'.format(room['ID_ROOM']))
        elif borrar == "2":
            alarm_id = db.get_weekly_alarm(identificador)['ALARM_ID']
            if node and node.deleteAlarm(alarm_id):
                db.delete_weekly_alarm(identificador)
            else:
                return redirect('/room/{0}?node_disconnected=1'.format(room['ID_ROOM']))
    
    once_alarms = db.get_once_alarms(group_id)
    weekly_alarms = db.get_weekly_alarms(group_id)
    return render_template('alarm-list.html', once_a = once_alarms, weekly_a = weekly_alarms, group = group_id,room=room)



@app.route('/global-alarm-list/<int:room_id>',  methods = ['POST', 'GET'])
@session_required_visit
def global_alarm_list(room_id):
    room = db.get_room_by_id(room_id)
    if request.method == 'GET' and request.args.get('id'):
        identificador = request.args.get('id')
        borrar = request.args.get('borrar')
        master = db.get_master_by_room(room['ID_ROOM'])
        node = getNodeByName(master['NAME'])
        if borrar == "1":
            alarm_id = db.get_global_once_alarm(identificador)['ALARM_ID']
            if node and node.deleteAlarm(alarm_id):
                db.delete_global_once_alarm(identificador)
            else:
                return redirect('/room/{0}?node_disconnected=1'.format(room['ID_ROOM']))
        elif borrar == "2":
            alarm_id = db.get_global_weekly_alarm(identificador)['ALARM_ID']
            if node and node.deleteAlarm(alarm_id):
                db.delete_global_weekly_alarm(identificador)
            else:
                return redirect('/room/{0}?node_disconnected=1'.format(room['ID_ROOM']))
    once_alarms = db.get_global_once_alarms(room_id)
    weekly_alarms = db.get_global_weekly_alarms(room_id)
    return render_template('alarm-list.html', once_a = once_alarms, weekly_a = weekly_alarms,room=room, group= 0, global_alarms=True)


@app.route('/new-room', methods = ['POST','GET']) 
@session_required
def new_room():
    if request.method == 'POST':
        name = request.form['room_n']
        node_id = request.form['node']
        print("id: {0}".format(node_id))
        node = getNodeById(node_id)
        room_available = True
        #para verificar si ya existe el aula con ese nombre
        if(db.create_room(name, node_id)):
            return redirect('/')
        else:
            free_nodes = db.get_free_nodes()
            room_available = False
            return render_template('new-room.html', nodes=free_nodes, room_available=room_available)
    else:
        free_nodes = db.get_free_nodes()
        room_available = False
        return render_template('new-room.html', nodes=free_nodes,room_available=room_available)

@app.route('/room/<int:room_id>', methods= ['GET'])
@session_required_visit
def show_room(room_id):
    room_name = db.get_room_name(room_id)
    groups = db.get_groups_by_room(room_id)
    master = db.get_master_by_room(room_id)
    ambient_data = False; ambient_data_read = False
    nodeDisconnected = False

    if request.method == 'GET' and request.args.get('id'):
        identificador = request.args.get('id')
        db.delete_room(identificador)
        rooms = db.get_all_rooms()
        return render_template('index.html', r = rooms)

    if master:
        if groups:
            node = getNodeByName(master['NAME'])
            if node:
                info = node.getGroupsInfo()
                if info:
                    for group in info:
                        group_id = list(filter(lambda g: g['NAME'] == group['name'], groups))[0]['ID_GROUP'] # Busca el id del grupo
                        if group['state'] == 'on': db.turn_on_group(group_id)
                        else: db.turn_off_group(group_id)
                        print(group_id)
                        print(group)
                    groups = db.get_groups_by_room(room_id)
                    if master["SENSOR"] == "true":
                        ambient_data = node.getAmbientData()
                        if ambient_data:
                            ambient_data_read = (ambient_data["temperature"] != "no_sensor")
                else: nodeDisconnected = True
            else: nodeDisconnected = True
        master_relays = db.get_relays_in_node(master['ID_NODE'])
        slaves = db.get_slaves_by_room(room_id)
        for s in slaves:
            s['relays'] = db.get_relays_in_node(s['ID_NODE'])
            s['ircontrol'] = db.get_ircontrol_in_node(s['ID_NODE'])
    else: 
        master_relays = False
        slaves = False
    return render_template('room.html', room_name=room_name, room_id=room_id, groups=groups, ambient_data= ambient_data, ambient_data_read= ambient_data_read,\
                            master=master, master_relays=master_relays, slaves=slaves, nodeDisconnected = nodeDisconnected)

@app.route('/groupsettings/<int:group_id>', methods= ['GET', 'POST'])
@session_required
def group_settings(group_id):
    if request.method == 'GET':
        group = db.get_group(group_id)
        relays = db.get_relays_in_group(group_id)
        ir = db.get_ircontrol_in_group(group_id)
        room = db.get_room_by_group(group_id)
        pir_hours = floor(int(group['PIR_TIME'])/60/60)
        pir_minutes = floor(int(group['PIR_TIME'])/60) - pir_hours*60
        pir_seconds = int(group['PIR_TIME']) - pir_minutes*60 - pir_hours*60*60
        inactivity_time = db.get_pir_inactivity_by_room(room['ID_ROOM'])
        
        return render_template('group_settings.html', group=group, relays=relays, ircontrols=ir, room=room,\
                               pir_hours=pir_hours, pir_minutes=pir_minutes, pir_seconds=pir_seconds,\
                               inactivity_time=inactivity_time)
    else:
        group = db.get_group(request.form['group_id'])
        hours = request.form['time_hours']
        minutes = request.form['time_minutes']
        seconds = request.form['time_seconds']
        pir_effect = request.form.get('pir_enable', 'off')
        room = db.get_room_by_group(group['ID_GROUP'])
        node_id = db.get_master_by_room(room['ID_ROOM'])['ID_NODE']
        relays = db.get_relays_in_group(group['ID_GROUP'])
        ir = db.get_ircontrol_in_group(group['ID_GROUP'])
        pir_time = int(hours)*60*60 + int(minutes)*60 + int(seconds)
        node = getNodeById(node_id)
        if node and node.modifyGroup(group['NAME'], group['NAME'], pir_effect != 'on', pir_time):
            db.modify_group(group['ID_GROUP'], group['NAME'], pir_effect, pir_time)
        else: return redirect('/room/{0}?node_disconnected=1'.format(room['ID_ROOM']))
        group = db.get_group(request.form['group_id'])
        return render_template('group_settings.html', group=group, relays=relays, ircontrols=ir, room=room,\
                               pir_hours=hours, pir_minutes=minutes, pir_seconds=seconds)

@app.route('/change_inactivity_time/<int:node_id>', methods = ['POST'])
@session_required
def change_inactivity_time(node_id):
    time = request.form['inac_time']
    group_id = request.form['group_id']
    node = db.get_node(node_id)
    master = getNodeByName(node['NAME'])
    if master and master.changePirSettings(time):
        db.change_pir_inactivity(time, node['ID_NODE'])
        return redirect('/groupsettings/{}'.format(group_id))
    else:
        return redirect('/room/{0}?node_disconnected=1'.format(node['ID_ROOM']))

@app.route('/new-group/<int:room_id>', methods = ['POST','GET']) 
@session_required
def new_group(room_id):
    node = db.get_master_by_room(room_id)
    relays = db.get_free_relays_in_room(room_id)
    room = db.get_room_by_id(room_id)
    if request.method == 'POST':
        name = request.form['group_n']
        pir_effect = request.form.get('pir_effect', 'off')
        pir_time = request.form['pir_time']
        room_name = db.get_room_name(room_id)
        groups = db.get_groups_by_room(room_id)
        n = getNodeByName(node['NAME'])
        #if True:
        if n and n.createGroup(name, pir_effect != 'on', pir_time):
            db.create_group(name,pir_effect,pir_time,node[0])
            return redirect('/room/{0}'.format(room_id))
        else:
            return redirect('/room/{0}?node_disconnected=1'.format(room_id))
    #elif True:
    elif getNodeByName(node['NAME']):
        return render_template('new-group.html', room=room)
    else: return redirect('/room/{0}?node_disconnected=1'.format(room_id))

@app.route('/delete-group', methods = ['GET'])
@session_required
def delete_group():
    group_id = request.args.get('group')
    if group_id:
        group = db.get_group(group_id)
        node = db.get_node(group['ID_NODE'])
        connectedNode = getNodeByName(node['NAME'])
        if connectedNode and connectedNode.deleteGroup(group['NAME']):
            db.delete_group(group_id)
        else:
            return redirect('/room/{0}?node_disconnected=1'.format(node["ID_ROOM"]))
        return redirect('/room/{0}'.format(node['ID_ROOM']))
    else:
        return redirect('/')

@app.route('/connect-relay', methods = ['GET', 'POST'])
@session_required
def connect_relay():
    if request.method == 'GET':
        relay_id = request.args.get('id')
        node_id = request.args.get('node')
        room = db.get_room_by_id(request.args.get('room'))
        groups = db.get_groups_by_room(room['ID_ROOM'])
        return render_template('connect-relay.html', relay_id=relay_id, node_id=node_id, groups=groups, room=room)
    else:
        relay_id = request.form['relay_id']
        room_id = request.form['room_id']
        group_id = request.form['group_id']
        node_id = request.form['node_id']
        node = db.get_node(node_id)
        relay = db.get_relay(relay_id)
        group = db.get_group(group_id)
        if node['M_E'] == 'M':
            connected_node = getNodeByName(node['NAME'])
            if connected_node and connected_node.connectMasterRelay(relay['NUMBER'], group['NAME']):
                db.connect_relay_to_group(relay_id, group_id)
            else:
                return redirect('/room/{0}?node_disconnected=1'.format(room_id))
        else:
            connected_node = getNodeByName(db.get_master_by_room(room_id)['NAME'])
            if connected_node and connected_node.connectSlaveRelay(node['IP'], relay['NUMBER'], group['NAME']):
                db.connect_relay_to_group(relay_id, group_id)
            else:
                return redirect('/room/{0}?node_disconnected=1'.format(room_id))
        return redirect('/room/{0}'.format(room_id))

@app.route('/disconnect-relay', methods=['GET'])
@session_required
def disconnect_relay():
    relay_id = request.args.get('id')
    room_id = request.args.get('room')
    relay = db.get_relay(relay_id)
    node = db.get_node(relay['ID_NODE'])
    if node['M_E'] == 'M':
        connected_node = getNodeByName(node['NAME'])
        if connected_node and connected_node.disconnectMasterRelay(relay['NUMBER']):
            db.disconnect_relay(relay_id)
        else: return redirect('/room/{0}?node_disconnected=1'.format(room_id))
    else:
        connected_node = getNodeByName(db.get_master_by_room(room_id)['NAME'])
        if connected_node and connected_node.disconnectSlaveRelay(node['IP'], relay['NUMBER']):
            db.disconnect_relay(relay_id)
        else: return redirect('/room/{0}?node_disconnected=1'.format(room_id))
    return redirect('/room/{0}'.format(room_id))

@app.route('/connect-ir', methods=['GET', 'POST'])
@session_required
def connect_ir():
    if request.method == 'GET':
        node = request.args.get('node')
        room = db.get_room_by_id(request.args.get('room'))
        groups = db.get_groups_by_room(room['ID_ROOM'])
        return render_template('connect-ir.html', node_id= node, room=room, groups=groups)
    else:
        room_id = request.form['room_id']
        node = db.get_node(request.form['node_id'])
        group = db.get_group(request.form['group_id'])
        master = db.get_master_by_room(room_id)
        connected_node = getNodeByName(master['NAME'])
        if connected_node and connected_node.connectSlaveIrControl(node['IP'], group['NAME']):
            db.connect_ir(node['ID_NODE'], group['ID_GROUP'])
        else: return redirect('/room/{0}?node_disconnected=1'.format(room_id))
        return redirect('/')

@app.route('/disconnect-ir', methods=['GET', 'POST'])
@session_required
def disconnect_ir():
    node = db.get_node(request.args.get('node'))
    room = db.get_room_by_id(request.args.get('room'))
    master = db.get_master_by_room(room['ID_ROOM'])
    connected_node = getNodeByName(master['NAME'])
    if connected_node and connected_node.disconnectSlaveIrControl(node['IP']):
        db.disconnect_ir(node['ID_NODE'])
    else: return redirect('/room/{0}?node_disconnected=1'.format(room['ID_ROOM']))
    return redirect('/room/{0}'.format(room['ID_ROOM']))

@app.route('/connect-temp-sensor', methods=['GET'])
@session_required
def connect_temp_sensor():
    room_id = request.args.get('room')
    node = db.get_node(request.args.get('node'))
    if node['M_E'] == 'M':
        connected_node = getNodeByName(node['NAME'])
        if connected_node and connected_node.connectMasterTemp():
            db.connect_temp(node['ID_NODE'])
            db.disconnect_pir(node['ID_NODE']);
        else: return redirect('/room/{0}?node_disconnected=1'.format(room_id))
    else:
        connected_node = getNodeByName(db.get_master_by_room(room_id)['NAME'])
        if connected_node and connected_node.connectSlaveTemp(node['IP']):
            db.connect_temp(node['ID_NODE'])
        else: return redirect('/room/{0}?node_disconnected=1'.format(room_id))
    return redirect('/room/{0}'.format(room_id))

@app.route('/disconnect-temp-sensor', methods=['GET'])
@session_required
def disconnect_temp_sensor():
    room_id = request.args.get('room')
    node = db.get_node(request.args.get('node'))
    if node['M_E'] == 'M':
        connected_node = getNodeByName(node['NAME'])
        if connected_node and connected_node.disconnectMasterTemp():
            db.disconnect_temp(node['ID_NODE'])
        else: return redirect('/room/{0}?node_disconnected=1'.format(room_id))
    else:
        connected_node = getNodeByName(db.get_master_by_room(room_id)['NAME'])
        if connected_node and connected_node.disconnectSlaveTemp(node['IP']):
            db.disconnect_temp(node['ID_NODE'])
        else: return redirect('/room/{0}?node_disconnected=1'.format(room_id))
    return redirect('/room/{0}'.format(room_id))

@app.route('/connect-movement-sensor', methods=['GET'])
@session_required
def connect_movement_sensor():
    room_id = request.args.get('room')
    node = db.get_node(request.args.get('node'))
    if node['M_E'] == 'M':
        connected_node = getNodeByName(node['NAME'])
        if connected_node and connected_node.connectMasterPir():
            db.connect_pir(node['ID_NODE'])
            db.disconnect_temp(node['ID_NODE'])
        else: return redirect('/room/{0}?node_disconnected=1'.format(room_id))
    else:
        connected_node = getNodeByName(db.get_master_by_room(room_id)['NAME'])
        if connected_node and connected_node.connectSlavePir(node['IP']):
            db.connect_pir(node['ID_NODE'])
        else: return redirect('/room/{0}?node_disconnected=1'.format(room_id))
    return redirect('/room/{0}'.format(room_id))

@app.route('/disconnect-movement-sensor', methods=['GET'])
@session_required
def disconnect_movement_sensor():
    room_id = request.args.get('room')
    node = db.get_node(request.args.get('node'))
    if node['M_E'] == 'M':
        connected_node = getNodeByName(node['NAME'])
        if connected_node and connected_node.disconnectMasterPir():
            db.disconnect_pir(node['ID_NODE'])
        else: return redirect('/room/{0}?node_disconnected=1'.format(room_id))
    else:
        connected_node = getNodeByName(db.get_master_by_room(room_id)['NAME'])
        if connected_node and connected_node.disconnectSlavePir(node['IP']):
            db.disconnect_pir(node['ID_NODE'])
        else: return redirect('/room/{0}?node_disconnected=1'.format(room_id))
    return redirect('/room/{0}'.format(room_id))

@app.route('/group-connect/<int:group_id>', methods=['GET', 'POST'])
@session_required
def group_connect(group_id):
    if request.method == 'GET':
        group = db.get_group(group_id)
        room = db.get_room_by_group(group_id)
        node = getNodeById(group['ID_NODE'])
        #if not node:
        #    return redirect('/room/{0}?node_disconnected=1'.format(room['ID_ROOM']))
        relays = db.get_free_relays_in_room(room['ID_ROOM'])
        ircontrols = db.get_free_ir_controls_in_room(room['ID_ROOM'])
        #ircontrols = ""
        return render_template('group-connect.html', group=group, relays=relays, ircontrols=ircontrols, room= room)
    else:
        actuator_type, actuator_id = request.form['actuator'].split('-')
        group = db.get_group(group_id)
        if actuator_type == 'relay':
            relay = db.get_relay(actuator_id)
            node = db.get_node(relay['ID_NODE'])
            room_id = node['ID_ROOM']
            if node['M_E'] == 'M':
                connected_node = getNodeByName(node['NAME'])
                if connected_node and connected_node.connectMasterRelay(relay['NUMBER'], group['NAME']):
                    db.connect_relay_to_group(actuator_id, group_id)
                else:
                    return redirect('/room/{0}?node_disconnected=1'.format(room_id))
            else:
                connected_node = getNodeByName(db.get_master_by_room(room_id)['NAME'])
                if connected_node and connected_node.connectSlaveRelay(node['IP'], relay['NUMBER'], group['NAME']):
                    db.connect_relay_to_group(relay['ID_RELAY'], group['ID_GROUP'])
                else:
                    return redirect('/room/{0}?node_disconnected=1'.format(room_id))
        elif actuator_type == 'ircontrol':
            node = db.get_node(actuator_id)
            master = db.get_master_by_room(node['ID_ROOM'])
            connected_node = getNodeByName(master['NAME'])
            if connected_node and connected_node.connectSlaveIrControl(node['IP'], group['NAME']):
                db.connect_ir(actuator_id, group_id)
            else:
                return redirect('/room/{0}?node_disconnected=1'.format(node['ID_ROOM']))
        return redirect('/groupsettings/{0}'.format(group_id))

@app.route('/turnon', methods = ['GET'])
@session_required_visit
def turn_on_group():
    if not request.args.get('room'):
        return redirect('/')
    room_id = request.args.get('room')
    if not request.args.get('group'):
        return redirect('/room/{0}'.format(room_id))
    group_id = request.args.get('group')
    node = getNodeByName(db.get_master_by_room(room_id)['NAME'])
    if not(node and node.turnOnGroup(db.get_group(group_id)['NAME'])):
        return redirect('/room/{0}?node_disconnected=1'.format(room_id))
    return redirect('/room/{0}'.format(room_id))

@app.route('/turnoff', methods = ['GET'])
@session_required_visit
def turn_off_group():
    if not request.args.get('room'):
        return redirect('/')
    room_id = request.args.get('room')
    if not request.args.get('group'):
        return redirect('/room/{0}'.format(room_id))
    group_id = request.args.get('group')
    node = getNodeByName(db.get_master_by_room(room_id)['NAME'])
    if not(node and node.turnOffGroup(db.get_group(group_id)['NAME'])):
        return redirect('/room/{0}?node_disconnected=1'.format(room_id))
    return redirect('/room/{0}'.format(room_id))

@app.route('/connect-slaves/<int:room_id>', methods=['GET'])
@session_required
def connect_slaves(room_id):
    master = db.get_master_by_room(room_id)
    node = getNodeByName(master['NAME'])
    if node:
        slave_info = node.getSlaveInfo()   # Pide la informacion de esclavos y sus relays
        relay_amount = node.getRelayInfo() # Solo almacena los que no hayan sido almacenados antes, fijandose en la IP
        if relay_amount and slave_info:
            if slave_info[0] != "no_slaves":
                for slave in range(len(slave_info)):
                    if not db.get_slave_by_room_and_ip(room_id, slave_info[slave]):
                        db.create_slave(slave, slave_info[slave], relay_amount[1], room_id)
        else: return redirect('/room/{0}?node_disconnected=1'.format(room_id))
    else: return redirect('/room/{0}?node_disconnected=1'.format(room_id))
    return redirect('/room/{0}'.format(room_id))

@app.route('/pair_nodes/<int:room_id>', methods=['GET'])
@session_required
def pair_nodes(room_id):
    master = db.get_master_by_room(room_id)
    room = db.get_room_by_id(room_id)
    node = getNodeByName(master['NAME'])
    if node and node.startPairing():
        return render_template('pairing.html', room=room)
    else:
        return redirect('/room/{0}?node_disconnected=1'.format(room_id))

@app.route('/record-ir', methods=['GET', 'POST'])
@session_required
def record_ir():
    if request.method == 'GET':
        room = db.get_room_by_id(request.args.get('room'))
        return render_template('record_ir.html', room=room)
    else:
        room = db.get_room_by_id(request.form['room'])
        code_name = request.form['ir_code_name']
        node_id = db.get_master_by_room(room['ID_ROOM'])['ID_NODE']
        node = getNodeById(node_id)
        code = node.recordIrCode() if node else False
        if code:
            return redirect('/get-ir/{0}?name={1}'.format(room['ID_ROOM'], code_name))
        else:
            return redirect('/room/{0}?node_disconnected=1'.format(room['ID_ROOM']))

@app.route('/get-ir/<int:room_id>', methods=['GET'])
@session_required
def get_ir(room_id):
    room = db.get_room_by_id(room_id)
    if request.args.get('ready', default=None) is not None:
        master = db.get_master_by_room(room_id)
        code_name = request.args.get('name')
        node = getNodeByName(master['NAME'])
        code = node.getIrCode() if node else None
        if code is not None:
            if len(code) == 0:
                return redirect('/transmit-ir?code=-1&room={}'.format(room_id))
            else:
                db.create_ir_code(code_name, code)
                print("CODIGO {}: {}".format(code_name, code))
                code = db.get_ir_code_by_name(code_name)['ID_IR_CODE']
                return redirect('/transmit-ir?code={0}&room={1}'.format(code, room_id))
    else:
        return render_template('recording-ir.html', room = room,
            code_name= request.args.get('name', default=None))

@app.route('/transmit-ir', methods = ['GET', 'POST'])
@session_required
def transmit_ir():
    if request.method == 'GET':
        master = db.get_master_by_room(request.args.get('room'))
        code = request.args.get('code', default=None)
        codes = db.get_ir_codes()
        return render_template('transmit-ir.html', codes= codes, room_id=master['ID_ROOM'])
    else:
        master = db.get_master_by_room(request.form['room'])
        code = db.get_ir_code_by_id(request.form['code'])
        node = getNodeByName(master['NAME'])
        if node and node.sendIrCode(code['CODE']):
            return redirect('/room/{0}'.format(master['ID_ROOM']))
        else: 
            return redirect('/room/{0}?node_disconnected=1'.format(master['ID_ROOM']))

@app.route('/edit_slave', methods= ['GET', 'POST'])
@session_required
def edit_slave():
    if request.method == 'GET':
        slave = db.get_node(request.args.get('node'))
        room = db.get_room_by_id(slave['ID_ROOM'])
        return render_template('edit_slave.html', node = slave, room = room)
    else:
        node_id = request.form['node']
        node = db.get_node(node_id)
        new_name = request.form['new_name']
        db.edit_node(node_id, new_name)
        return redirect('/room/{0}'.format(node['ID_ROOM']))

@app.route('/group-state/<int:room_id>', methods = ['GET'])
def group_state(room_id):
    groups = db.get_groups_by_room(room_id)
    master = db.get_master_by_room(room_id)
    node = getNodeByName(master['NAME'])
    if node and groups:
        info = node.getGroupsInfo()
        if info:
            for group in info:
                group_id = list(filter(lambda g: g['NAME'] == group['name'], groups))[0]['ID_GROUP'] # Busca el id del grupo
                if group['state'] == 'on': db.turn_on_group(group_id)
                else: db.turn_off_group(group_id)
    groups = db.get_groups_by_room(room_id)
    if groups:
        return json.dumps(groups)
    else:
        return "no_groups"

@app.route('/clear/<node_name>', methods=['GET'])
@session_required
def clear_eeprom(node_name):
    node = getNodeByName(node_name)
    if node and node.clearEEPROM():
        return redirect('/debug-clear?sent=1')
    else: return redirect('/debug-clear?node_disconnected=1')

@app.route('/reset/<node_name>', methods=['GET'])
@session_required
def reset(node_name):
    node = getNodeByName(node_name)
    if node and node.resetMaster():
        return redirect('/debug-clear?sent=1')
    else: return redirect('/debug-clear?node_disconnected=1')

@app.route('/debug-clear', methods=['GET'])
@session_required
def debug_clear():
    return render_template('debug_clear.html', nodes = devices,
        disconnected=request.args.get('node_disconnected', default=None),
        sent=request.args.get('sent', default=None))


if __name__ == "__main__":
    try:
        bd.create_BD()
    except:
        pass
    begin()
    app.secret_key = os.urandom(12)
    app.run(host = '0.0.0.0', threaded=True, use_reloader=False)