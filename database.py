import sqlite3

dbPath = "/home/ubuntu/proyectoSPU/databaseSPU.db"

def create_BD():
    con = sqlite3.connect(dbPath)
    cursor = con.cursor()

    cursor.execute('''
    CREATE TABLE USER (
    ID    INTEGER PRIMARY KEY AUTOINCREMENT
                  UNIQUE
                  NOT NULL,
    NAME  CHAR    NOT NULL,
    EMAIL CHAR    NOT NULL,
    PASSWORD CHAR    NOT NULL
    );
    ''')

    cursor.execute('''
    CREATE TABLE ROOM (
    ID_ROOM   INTEGER PRIMARY KEY AUTOINCREMENT
                 NOT NULL
                 UNIQUE,
    NAME CHAR    NOT NULL
    );
    ''')

    cursor.execute('''
    CREATE TABLE WEEKLY_ALARM (
    ID_WEEKLY INTEGER PRIMARY KEY AUTOINCREMENT
                      UNIQUE
                      NOT NULL,
    NAME      CHAR    NOT NULL,
    HOUR_I    INTEGER NOT NULL,
    HOUR_F    INTEGER NOT NULL,
    DATE      INTEGER NOT NULL,
    MINUTES_I INTEGER NOT NULL,
    MINUTES_F INTEGER NOT NULL,
    ALARM_ID  INTEGER NOT NULL,
    ID_GROUP  INTEGER REFERENCES [GROUPS] (ID_GROUP) ON DELETE CASCADE
    );
    ''')

    cursor.execute('''
    CREATE TABLE ONCE_ALARM (
    ID_ONCE  INTEGER PRIMARY KEY
                     NOT NULL,
    NAME     CHAR    NOT NULL,
    DATE     CHAR    NOT NULL,
    HOUR_I   INTEGER NOT NULL,
    MINUTE_I INTEGER NOT NULL,
    ID_GROUP INTEGER REFERENCES [GROUPS] (ID_GROUP) ON DELETE CASCADE,
    HOUR_F   INTEGER NOT NULL,
    MINUTE_F INTEGER NOT NULL,
    ALARM_ID INTEGER NOT NULL
    );
    ''')

    cursor.execute('''
    CREATE TABLE GLOBAL_WEEKLY_ALARM (
    ID_WEEKLY INTEGER PRIMARY KEY AUTOINCREMENT
                      UNIQUE
                      NOT NULL,
    NAME      CHAR    NOT NULL,
    HOUR_I    INTEGER NOT NULL,
    HOUR_F    INTEGER NOT NULL,
    DATE      INTEGER NOT NULL,
    MINUTES_I INTEGER NOT NULL,
    MINUTES_F INTEGER NOT NULL,
    ALARM_ID  INTEGER NOT NULL,
    ID_ROOM   INTEGER REFERENCES ROOM (ID_ROOM) ON DELETE CASCADE
    );
    ''')

    cursor.execute('''
    CREATE TABLE GLOBAL_ONCE_ALARM (
    ID_ONCE  INTEGER PRIMARY KEY
                     NOT NULL,
    NAME     CHAR    NOT NULL,
    DATE     CHAR    NOT NULL,
    HOUR_I   INTEGER NOT NULL,
    MINUTE_I INTEGER NOT NULL,
    ID_ROOM  INTEGER REFERENCES ROOM (ID_ROOM) ON DELETE CASCADE,
    ALARM_ID INTEGER NOT NULL,
    HOUR_F   INTEGER NOT NULL,
    MINUTE_F INTEGER NOT NULL
    );
    ''')

    cursor.execute('''
    CREATE TABLE [GROUPS] (
    ID_GROUP   INTEGER PRIMARY KEY AUTOINCREMENT
                       NOT NULL
                       UNIQUE,
    ON_OFF     BOOLEAN NOT NULL,
    NAME       CHAR    NOT NULL,
    PIR_EFFECT BOOLEAN NOT NULL,
    PIR_TIME   INTEGER NOT NULL,
    ID_NODE    INTEGER REFERENCES NODE (ID_NODE) ON DELETE CASCADE
    );
    ''')

    cursor.execute('''
    CREATE TABLE RELAY (
    ID_RELAY INTEGER PRIMARY KEY AUTOINCREMENT
                     UNIQUE
                     NOT NULL,
    ID_NODE  INTEGER REFERENCES NODE (ID_NODE) ON DELETE CASCADE,
    NUMBER   INTEGER NOT NULL
    );
    ''')

    cursor.execute('''
    CREATE TABLE RELAY_GROUP (
    ID_RELAY INTEGER REFERENCES RELAY (ID_RELAY) ON DELETE CASCADE,
    ID_GROUP INTEGER REFERENCES [GROUPS] (ID_GROUP) ON DELETE CASCADE
    );
    ''')
    
    cursor.execute('''
    CREATE TABLE NODE (
    ID_NODE  INTEGER PRIMARY KEY AUTOINCREMENT
                     UNIQUE
                     NOT NULL,
    NAME     CHAR    NOT NULL,
    SENSOR   BOOLEAN NOT NULL,
    PIR      BOOLEAN NOT NULL,
    PIR_INAC INTEGER NOT NULL,
    M_E      CHAR    NOT NULL,
    SSID     CHAR    NOT NULL,
    PASSWORD CHAR    NOT NULL,
    IP       INTEGER ,
    ID_ROOM  INTEGER REFERENCES ROOM (ID_ROOM) ON DELETE CASCADE
    );
    ''')

    cursor.execute('''
    CREATE TABLE IRCONTROL (
    ID_IRCONTROL INTEGER PRIMARY KEY AUTOINCREMENT
                     UNIQUE
                     NOT NULL,
    ID_NODE INTEGER REFERENCES NODE (ID_NODE) ON DELETE CASCADE,
    ID_GROUP INTEGER REFERENCES [GROUPS] (ID_GROUP) ON DELETE CASCADE
    );
    ''')

    cursor.execute('''
    CREATE TABLE IR_CODE (
    ID_IR_CODE INTEGER PRIMARY KEY AUTOINCREMENT
                     UNIQUE
                     NOT NULL,
    NAME CHAR NOT NULL,
    CODE BLOB NOT NULL
    );
    ''')

    cursor.execute('''
    CREATE TABLE ROOM_IR_CODE (
    ID_ROOM_IR_CODE INTEGER PRIMARY KEY AUTOINCREMENT
                     UNIQUE
                     NOT NULL,
    ID_NODE INTEGER REFERENCES NODE (ID_NODE) ON DELETE CASCADE,
    ID_IR_CODE INTEGER REFERENCES IR_CODE (ID_IR_CODE) ON DELETE CASCADE
    );
    ''')

    cursor.execute('''
    INSERT INTO USER (NAME, EMAIL, PASSWORD) VALUES ("admin", "admin", "80e269cf289985b880760e155bdf0ff1f2d334ee8621850b8b7cbdecb59e9c53c8a37f6af5d0c09e2782ffcfc4dc570857fcbdd0a8c713273b90dbf26611134892005f49d549c510b224a7ceda40514bae9e89a250a6185db0ebc44f0270f2e1")''')

    con.commit()
    print('Tabla creada con exito')


# -------- USER ----------
def get_user(us):
    con = sqlite3.connect(dbPath)
    cursor = con.cursor()
    cursor.execute('SELECT email,password FROM USER WHERE EMAIL="{0}"'.format(us))
    for i in cursor:
        return i

# ----- ONCE ALARM ------

def get_once_alarms(group_id):
    con = sqlite3.connect(dbPath)
    cursor = con.cursor()
    cursor.execute("SELECT ONCE_ALARM.* from groups inner join once_alarm on groups.ID_GROUP = ONCE_ALARM.ID_GROUP where groups.ID_GROUP={0}".format(group_id))
    alarms = []
    for i in cursor:
        alarms.append(i)
    return alarms

def get_once_alarm(id):
    con = sqlite3.connect(dbPath)
    con.row_factory = sqlite3.Row
    cursor = con.cursor()
    cursor.execute("""SELECT * FROM ONCE_ALARM
                      WHERE ID_ONCE =?""", [id])
    rows = cursor.fetchone()
    if rows: return dict(rows)
    else: return False

def get_global_once_alarms(room_id):
    con = sqlite3.connect(dbPath)
    cursor = con.cursor()
    cursor.execute("""SELECT a.* from GLOBAL_ONCE_ALARM as a 
                      inner join ROOM as r USING (ID_ROOM)
                      where r.ID_ROOM={0}""".format(room_id))
    alarms = []
    for i in cursor:
        alarms.append(i)
    return alarms

def get_global_once_alarm(id):
    con = sqlite3.connect(dbPath)
    con.row_factory = sqlite3.Row
    cursor = con.cursor()
    cursor.execute("""SELECT * FROM GLOBAL_ONCE_ALARM
                      WHERE ID_ONCE =?""", [id])
    rows = cursor.fetchone()
    if rows: return dict(rows)
    else: return False

def get_all_once_alarms(): #devuelve todas las alarmas ocasionales
    con = sqlite3.connect(dbPath)
    cursor = con.cursor()
    cursor.execute("SELECT * from ONCE_ALARM")
    alarms = []
    for i in cursor:
        alarms.append(i)
    return alarms

def create_once_alarm(date, hour_i, minute_i,hour_f, minute_f, name, group, alarm_id):
    con = sqlite3.connect(dbPath)
    cursor = con.cursor()
    cursor.execute('''INSERT INTO ONCE_ALARM (NAME,DATE,HOUR_I,MINUTE_I,ID_GROUP,HOUR_F,MINUTE_F, ALARM_ID)
                      VALUES (?,?,?,?,?,?,?,?)''', [name, date, hour_i, minute_i, group, hour_f, minute_f, alarm_id])
    con.commit()

def create_global_once_alarm(date, hour_i, minute_i,hour_f, minute_f, name, room, alarm_id):
    con = sqlite3.connect(dbPath)
    cursor = con.cursor()
    cursor.execute('''INSERT INTO GLOBAL_ONCE_ALARM (NAME,DATE,HOUR_I,MINUTE_I,ID_ROOM,HOUR_F,MINUTE_F, ALARM_ID)
                      VALUES (?,?,?,?,?,?,?,?)''', [name, date, hour_i, minute_i, room, hour_f, minute_f, alarm_id])
    con.commit()

def delete_once_alarm(ident):
    con = sqlite3.connect(dbPath)
    cursor = con.cursor()
    cursor.execute("DELETE FROM ONCE_ALARM WHERE ID_ONCE={0}".format(ident))
    con.commit()

def delete_global_once_alarm(ident):
    con = sqlite3.connect(dbPath)
    cursor = con.cursor()
    cursor.execute("DELETE FROM GLOBAL_ONCE_ALARM WHERE ID_ONCE={0}".format(ident))
    con.commit()

def delete_global_weekly_alarm(ident):
    con = sqlite3.connect(dbPath)
    cursor = con.cursor()
    cursor.execute("DELETE FROM GLOBAL_WEEKLY_ALARM WHERE ID_WEEKLY={0}".format(ident))
    con.commit()

# ------ WEEKLY ALARM ------
def get_weekly_alarms(group_id):
    con = sqlite3.connect(dbPath)
    cursor = con.cursor()
    cursor.execute("SELECT WEEKLY_ALARM.* from groups inner join weekly_alarm on groups.ID_GROUP = WEEKLY_ALARM.ID_GROUP where groups.ID_GROUP={0}".format(group_id))
    alarms = []
    for i in cursor:
        alarms.append(i)
    return alarms

def get_weekly_alarm(id):
    con = sqlite3.connect(dbPath)
    con.row_factory = sqlite3.Row
    cursor = con.cursor()
    cursor.execute("""SELECT * FROM WEEKLY_ALARM
                      WHERE ID_WEEKLY = ?""", [id])
    rows = cursor.fetchone()
    if rows: return dict(rows)
    else: return False

def get_global_weekly_alarms(room_id):
    con = sqlite3.connect(dbPath)
    cursor = con.cursor()
    cursor.execute("""SELECT a.* from GLOBAL_WEEKLY_ALARM as a 
                      inner join ROOM as r USING (ID_ROOM)
                      where r.ID_ROOM={0}""".format(room_id))
    alarms = []
    for i in cursor:
        alarms.append(i)
    return alarms

def get_global_weekly_alarm(id):
    con = sqlite3.connect(dbPath)
    con.row_factory = sqlite3.Row
    cursor = con.cursor()
    cursor.execute("""SELECT * FROM GLOBAL_WEEKLY_ALARM
                      WHERE ID_WEEKLY = ?""", [id])
    rows = cursor.fetchone()
    if rows: return dict(rows)
    else: return False

def get_all_weekly_alarms(): #devuelve todas las alarmas semanales
    con = sqlite3.connect(dbPath)
    cursor = con.cursor()
    cursor.execute("SELECT * from WEEKLY_ALARM")
    alarms = []
    for i in cursor:
        alarms.append(i)
    return alarms

def create_weekly_alarm(name,hour_i,hour_f,day,minutes_i,minutes_f,group, alarm_id):
    con = sqlite3.connect(dbPath)
    cursor = con.cursor()
    cursor.execute('''INSERT INTO WEEKLY_ALARM (NAME,HOUR_I,HOUR_F,DATE,MINUTES_I,MINUTES_F,ID_GROUP, ALARM_ID)
        VALUES (?,?,?,?,?,?,?,?)''', [name, hour_i, hour_f, day, minutes_i, minutes_f, group, alarm_id])
    con.commit()

def create_global_weekly_alarm(name,hour_i,hour_f,day,minutes_i,minutes_f,room,alarm_id):
    con = sqlite3.connect(dbPath)
    cursor = con.cursor()
    cursor.execute('''INSERT INTO GLOBAL_WEEKLY_ALARM (NAME,HOUR_I,HOUR_F,DATE,MINUTES_I,MINUTES_F,ID_ROOM,ALARM_ID)
        VALUES (?,?,?,?,?,?,?,?)''', [name, hour_i, hour_f, day, minutes_i, minutes_f, room,alarm_id])
    con.commit()

def delete_weekly_alarm(id):
    con = sqlite3.connect(dbPath)
    cursor = con.cursor()
    cursor.execute("DELETE FROM WEEKLY_ALARM WHERE ID_WEEKLY={0}".format(id))
    con.commit()

# -------- ROOM ------------
def create_room(name, node_id):
    con = sqlite3.connect(dbPath)
    cursor = con.cursor()
    # verifico que el nombre del aula no exista
    cursor.execute('SELECT ROOM.NAME FROM ROOM WHERE ROOM.NAME="{0}"'.format(name))
    room = []
    for i in cursor:
        room.append(i)
    if (len(room)==0):
        cursor.execute('INSERT INTO ROOM (NAME) VALUES ("{0}")'.format(name))
        cursor.execute('UPDATE NODE SET ID_ROOM = ? WHERE ID_NODE=?', [cursor.lastrowid, node_id])
        con.commit()
        return True
    else: 
        return False

def get_all_rooms():
    con = sqlite3.connect(dbPath)
    cursor = con.cursor()
    cursor.execute("SELECT * from ROOM")
    rooms = []
    for i in cursor:
        rooms.append(i)
    return rooms

def get_room_name(id):
    con = sqlite3.connect(dbPath)
    cursor = con.cursor()
    cursor.execute('SELECT NAME FROM ROOM WHERE ID_ROOM = {0}'.format(id))
    return cursor.fetchone()[0]

def get_groups_by_room(room_id):
    con = sqlite3.connect(dbPath)
    con.row_factory = sqlite3.Row
    cursor = con.cursor()
    cursor.execute('SELECT g.id_group, g.name, g.on_off FROM NODE'+\
                   ' JOIN ROOM USING(ID_ROOM)'+\
                   ' JOIN GROUPS as g USING(ID_NODE)'+\
                   ' WHERE ROOM.ID_ROOM =?', (room_id,))
    rows = cursor.fetchall()
    if rows: return rows_to_dict(rows)
    else: return False

def get_room_by_group(group_id):
    con = sqlite3.connect(dbPath)
    con.row_factory = sqlite3.Row
    cursor = con.cursor()
    cursor.execute('SELECT r.ID_ROOM, r.NAME FROM ROOM as r'+\
                   ' JOIN NODE USING(ID_ROOM)'+\
                   ' JOIN GROUPS as g USING(ID_NODE)'+\
                   ' WHERE g.ID_GROUP =?', (group_id,))
    rows = cursor.fetchone()
    if rows: return dict(rows)
    else: return False

def get_node_by_room(room_id):
    con = sqlite3.connect(dbPath)
    cursor = con.cursor()
    cursor.execute('SELECT * from NODE WHERE NODE.ID_ROOM = {0}'.format(room_id))
    for i in cursor:
        return i

def get_room_by_id(room_id):
    con = sqlite3.connect(dbPath)
    con.row_factory = sqlite3.Row
    cursor = con.cursor()
    cursor.execute('SELECT * FROM ROOM WHERE ID_ROOM=?', [room_id])
    rows = cursor.fetchone()
    if rows: return dict(rows)
    else: return False

# ------ RELAYS -------
def get_relay(relay_id):
    con = sqlite3.connect(dbPath)
    con.row_factory = sqlite3.Row
    cursor = con.cursor()
    cursor.execute('SELECT * FROM RELAY WHERE ID_RELAY=?', [relay_id])
    rows = cursor.fetchone()
    if rows: return dict(rows)
    else: return False

def get_relay_without_group(id_node): #devuelve todos los relays sin grupo asignado
    con = sqlite3.connect(dbPath)
    cursor = con.cursor()
    cursor.execute('SELECT * FROM RELAY WHERE RELAY.ID_NODE = {0} AND relay.ID_RELAY not in (select relay_group.ID_RELAY from relay_group)'.format(id_node))
    relays = []
    for i in cursor:
        relays.append(i)
    return relays

def connect_relay_to_group(relay_id, group_id):
    con = sqlite3.connect(dbPath)
    cursor = con.cursor()
    cursor.execute('INSERT INTO RELAY_GROUP (ID_RELAY, ID_GROUP) VALUES (?,?)',[relay_id, group_id])
    con.commit()

def disconnect_relay(relay_id):
    con = sqlite3.connect(dbPath)
    cursor = con.cursor()
    cursor.execute('DELETE FROM RELAY_GROUP WHERE ID_RELAY=?',[relay_id])
    con.commit()

def get_relays_in_room(room_id):
    con = sqlite3.connect(dbPath)
    con.row_factory = sqlite3.Row
    cursor = con.cursor()
    cursor.execute('''SELECT r.ID_RELAY, r.NUMBER, n.ID_NODE, n.NAME as NODE_NAME, n.M_E, g.ID_GROUP, g.NAME as GROUP_NAME 
                      FROM RELAY as r
                      JOIN NODE as n USING(ID_NODE)
                      LEFT JOIN RELAY_GROUP USING(ID_RELAY)
                      LEFT JOIN GROUPS as g USING(ID_GROUP)
                      WHERE n.ID_ROOM = ?''', [room_id])
    rows = cursor.fetchall()
    if rows is None: return False
    else: return rows_to_dict(rows)

def get_free_relays_in_room(room_id):
    con = sqlite3.connect(dbPath)
    con.row_factory = sqlite3.Row
    cursor = con.cursor()
    cursor.execute('''SELECT r.ID_RELAY, r.NUMBER, n.ID_NODE, n.NAME as NODE_NAME, n.M_E, g.ID_GROUP, g.NAME as GROUP_NAME 
                      FROM RELAY as r
                      JOIN NODE as n USING(ID_NODE)
                      LEFT JOIN RELAY_GROUP USING(ID_RELAY)
                      LEFT JOIN GROUPS as g USING(ID_GROUP)
                      WHERE n.ID_ROOM = ? AND g.ID_GROUP IS NULL''', [room_id])
    rows = cursor.fetchall()
    if rows is None: return False
    else: return rows_to_dict(rows)
# ------ IR -----------

def create_ir_code(name, code):
    code_bytes = bytearray()
    for c in code:
        code_bytes += bytearray(c.to_bytes(2, 'big'))
    con = sqlite3.connect(dbPath)
    cursor = con.cursor()
    cursor.execute('''INSERT INTO IR_CODE (NAME, CODE)
                      VALUES (?, ?)''', [name, bytes(code_bytes)])
    con.commit()

def get_ir_code_by_name(name):
    con = sqlite3.connect(dbPath)
    con.row_factory = sqlite3.Row
    cursor = con.cursor()
    cursor.execute('''SELECT * FROM IR_CODE
                      WHERE NAME = ?''', [name])
    row = cursor.fetchone()
    if row:
        row = dict(row)
        code = row['CODE']
        row['CODE'] = []
        for b in range(0, len(code), 2):
            row['CODE'].append(int.from_bytes(bytes(bytearray(code[b:b+2])), 'big'))
        return row
    else: return False

def get_ir_code_by_id(id):
    con = sqlite3.connect(dbPath)
    con.row_factory = sqlite3.Row
    cursor = con.cursor()
    cursor.execute('''SELECT * FROM IR_CODE
                      WHERE ID_IR_CODE = ?''', [id])
    row = cursor.fetchone()
    if row:
        row = dict(row)
        code = row['CODE']
        row['CODE'] = []
        for b in range(0, len(code), 2):
            row['CODE'].append(int.from_bytes(bytes(bytearray(code[b:b+2])), 'big'))
        return row
    else: return False

def get_ir_codes():
    con = sqlite3.connect(dbPath)
    con.row_factory = sqlite3.Row
    cursor = con.cursor()
    cursor.execute('''SELECT * FROM IR_CODE''')
    rows = cursor.fetchall()
    if rows:
        result = []
        for r in rows:
            row = dict(r)
            code = row['CODE']
            row['CODE'] = []
            for b in range(0, len(code), 2):
                row['CODE'].append(int.from_bytes(bytes(bytearray(code[b:b+2])), 'big'))
            result.append(row)
        return result
    else: return False

def connect_ir(node_id, group_id):
    con = sqlite3.connect(dbPath)
    cursor = con.cursor()
    cursor.execute('''INSERT INTO IRCONTROL (ID_NODE, ID_GROUP)
                      VALUES (?, ?)''', [node_id, group_id])
    con.commit()

def disconnect_ir(node_id):
    con = sqlite3.connect(dbPath)
    cursor = con.cursor()
    cursor.execute('''DELETE FROM IRCONTROL
                      WHERE ID_NODE = ?''', [node_id])
    con.commit()

def get_ircontrol_in_node(node_id):
    con = sqlite3.connect(dbPath)
    con.row_factory = sqlite3.Row
    cursor = con.cursor()
    cursor.execute('''SELECT ir.ID_GROUP as group_id, g.NAME as group_name, ir.ID_NODE
                      FROM IRCONTROL as ir
                      JOIN GROUPS as g USING(ID_GROUP)
                      WHERE ir.ID_NODE = ?''', [node_id])
    row = cursor.fetchone()
    if row is None: return False
    else: return dict(row)

def get_ircontrol_in_group(group_id):
    con = sqlite3.connect(dbPath)
    con.row_factory = sqlite3.Row
    cursor = con.cursor()
    cursor.execute('''SELECT ir.ID_IRCONTROL, ir.ID_NODE, n.NAME as NODE_NAME, n.M_E
                      FROM IRCONTROL as ir
                      JOIN NODE as n USING(ID_NODE)
                      JOIN GROUPS as g USING(ID_GROUP)
                      WHERE ir.ID_GROUP = ?''', [group_id])
    rows = cursor.fetchall()
    if rows is None: return False
    else: return rows_to_dict(rows)

def get_free_ir_controls_in_room(room_id):
    con = sqlite3.connect(dbPath)
    con.row_factory = sqlite3.Row
    cursor = con.cursor()
    cursor.execute('''SELECT n.ID_NODE, n.NAME as NODE_NAME
                      FROM NODE as n
                      LEFT JOIN IRCONTROL as ir USING(ID_NODE)
                      WHERE n.ID_ROOM = ? AND n.M_E = 'E' AND ir.ID_IRCONTROL IS NULL''', [room_id])
    rows = cursor.fetchall()
    if rows is None: return False
    else: return rows_to_dict(rows)

# ------ GROUPS -------

def get_group(id):
    con = sqlite3.connect(dbPath)
    con.row_factory = sqlite3.Row
    cursor = con.cursor()
    cursor.execute('SELECT * FROM GROUPS WHERE ID_GROUP =? ', (id,))
    rows = cursor.fetchone()
    if rows is None: return False
    else: return dict(rows)

def get_relays_in_group(group_id):
    con = sqlite3.connect(dbPath)
    con.row_factory = sqlite3.Row
    cursor = con.cursor()
    cursor.execute('SELECT r.id_relay, r.number, n.NAME as NODE_NAME, n.M_E FROM RELAY as r'+\
                   ' JOIN RELAY_GROUP USING(ID_RELAY)'+\
                   ' JOIN GROUPS as g USING(ID_GROUP)'+\
                   ' JOIN NODE as n USING(ID_NODE)'+\
                   ' WHERE g.ID_GROUP =?', (group_id,))
    rows = cursor.fetchall()
    if rows is None: return False
    else: return rows_to_dict(rows)

def create_group(name,pir_effect,pir_time,node_id):
    con = sqlite3.connect(dbPath)
    cursor = con.cursor()
    cursor.execute('INSERT INTO GROUPS (ON_OFF, NAME, PIR_EFFECT,PIR_TIME,ID_NODE) VALUES ("off", "{}", "{}", "{}", "{}")'.format(name,pir_effect,pir_time,node_id))
    con.commit()

def delete_group(group_id):
    con = sqlite3.connect(dbPath)
    con.execute("PRAGMA foreign_keys = ON") # Para que funcione la eliminacion en cascada
    cursor = con.cursor()
    cursor.execute('''DELETE FROM GROUPS WHERE ID_GROUP=?''', [group_id])
    con.commit()

def modify_group(group_id, new_name, pir_effect, pir_time):
    con = sqlite3.connect(dbPath)
    cursor = con.cursor()
    cursor.execute('''UPDATE GROUPS
                      SET NAME = ?, PIR_EFFECT = ?, PIR_TIME = ?
                      WHERE ID_GROUP = ?''', [new_name, pir_effect, pir_time, group_id])
    con.commit()

def turn_on_group(group_id):
    con = sqlite3.connect(dbPath)
    cursor = con.cursor()
    cursor.execute('''UPDATE GROUPS
                      SET ON_OFF='on'
                      WHERE ID_GROUP=?''',[group_id])
    con.commit()

def turn_off_group(group_id):
    con = sqlite3.connect(dbPath)
    cursor = con.cursor()
    cursor.execute('''UPDATE GROUPS
                      SET ON_OFF='off'
                      WHERE ID_GROUP=?''',[group_id])
    con.commit()

# ----- SENSORS ------

def connect_temp(node_id):
    con = sqlite3.connect(dbPath)
    cursor = con.cursor()
    cursor.execute('''UPDATE NODE
                      SET SENSOR='true'
                      WHERE ID_NODE=?''',[node_id])
    con.commit()

def disconnect_temp(node_id):
    con = sqlite3.connect(dbPath)
    cursor = con.cursor()
    cursor.execute('''UPDATE NODE
                      SET SENSOR='false'
                      WHERE ID_NODE=?''',[node_id])
    con.commit()

def connect_pir(node_id):
    con = sqlite3.connect(dbPath)
    cursor = con.cursor()
    cursor.execute('''UPDATE NODE
                      SET PIR='true'
                      WHERE ID_NODE=?''',[node_id])
    con.commit()

def disconnect_pir(node_id):
    con = sqlite3.connect(dbPath)
    cursor = con.cursor()
    cursor.execute('''UPDATE NODE
                      SET PIR='false'
                      WHERE ID_NODE=?''',[node_id])
    con.commit()

def change_pir_inactivity(time, node_id):
    con = sqlite3.connect(dbPath)
    cursor = con.cursor()
    cursor.execute('''UPDATE NODE
                      SET PIR_INAC = ?
                      WHERE ID_NODE = ?''',[time, node_id])
    con.commit()

def get_pir_inactivity_by_room(room_id):
    con = sqlite3.connect(dbPath)
    cursor = con.cursor()
    cursor.execute('''SELECT n.PIR_INAC
                      FROM NODE as n
                      JOIN ROOM as r USING(ID_ROOM)
                      WHERE r.ID_ROOM = ?''',[room_id])
    con.commit()
    row = cursor.fetchone()
    if row is None: return False
    else: return int(row[0])

# ----- NODE ------

def get_node_by_name(name):
    con = sqlite3.connect(dbPath)
    cursor = con.cursor()
    cursor.execute('SELECT ID_NODE FROM NODE WHERE NAME =?', (name,))
    row = cursor.fetchone()
    if row is None: return False
    else: return int(row[0])

def get_node(node_id):
    con = sqlite3.connect(dbPath)
    con.row_factory = sqlite3.Row
    cursor = con.cursor()
    cursor.execute('SELECT * FROM NODE WHERE ID_NODE =?', [node_id])
    row = cursor.fetchone()
    if row is None: return False
    else: return dict(row)

def create_node(name):
    con = sqlite3.connect(dbPath)
    cursor = con.cursor()
    cursor.execute('INSERT INTO NODE (NAME, SENSOR, PIR, PIR_INAC, M_E, SSID, PASSWORD, ID_ROOM)'+\
                  ' VALUES (?, "false", "false", 0, "M", "", "", 0)', (name,))
    con.commit()

def create_slave(name, ip, relay_amount, room_id):
    con = sqlite3.connect(dbPath)
    cursor = con.cursor()
    cursor.execute('INSERT INTO NODE (NAME, SENSOR, PIR, PIR_INAC, M_E, SSID, PASSWORD, ID_ROOM, IP)'+\
                  ' VALUES (?, "false", "false", 0, "E", "", "", ?, ?)', [ip, room_id, ip])
    slave_id = cursor.lastrowid
    for n in range(relay_amount):
        cursor.execute('''INSERT INTO RELAY (ID_NODE, NUMBER)
                          VALUES (?,?)''', [slave_id, n])
    con.commit()

def get_master_by_room(room_id):
    con = sqlite3.connect(dbPath)
    con.row_factory = sqlite3.Row
    cursor = con.cursor()
    cursor.execute('SELECT * FROM NODE WHERE ID_ROOM =? AND M_E = "M"', [room_id])
    row = cursor.fetchone()
    if row is None: return False
    else: return row

def get_relays_in_node(node_id):
    con = sqlite3.connect(dbPath)
    con.row_factory = sqlite3.Row
    cursor = con.cursor()
    cursor.execute('SELECT r.NUMBER, g.NAME, g.ID_GROUP, r.ID_RELAY FROM RELAY as r'+\
                    ' JOIN NODE as n USING(ID_NODE)'+\
                    ' LEFT JOIN RELAY_GROUP USING(ID_RELAY)'+\
                    ' LEFT JOIN GROUPS as g ON (g.ID_GROUP = RELAY_GROUP.ID_GROUP)'+\
                    ' WHERE r.ID_NODE = ?', [node_id])
    row = cursor.fetchall()
    if row is None: return False
    else: return rows_to_dict(row)

def get_slaves_by_room(room_id):
    con = sqlite3.connect(dbPath)
    con.row_factory = sqlite3.Row
    cursor = con.cursor()
    cursor.execute('SELECT * FROM NODE WHERE ID_ROOM = ? AND M_E = "E"', [room_id])
    row = cursor.fetchall()
    return rows_to_dict(row)

def get_slave_by_room_and_ip(room_id, ip):
    con = sqlite3.connect(dbPath)
    con.row_factory = sqlite3.Row
    cursor = con.cursor()
    cursor.execute('SELECT * FROM NODE WHERE ID_ROOM =? AND M_E = "E" AND IP=?', [room_id, ip])
    row = cursor.fetchone()
    if row is None: return False
    else: return dict(row)

def get_free_nodes():
    con = sqlite3.connect(dbPath)
    con.row_factory = sqlite3.Row
    cursor = con.cursor()
    cursor.execute('SELECT * FROM NODE WHERE ID_ROOM = 0 AND M_E = "M"')
    row = cursor.fetchall()
    if not row: return False
    else: return rows_to_dict(row)

def add_relay_to_node(node_id, relay_number):
    con = sqlite3.connect(dbPath)
    cursor = con.cursor()
    cursor.execute('''INSERT INTO RELAY (ID_NODE, NUMBER)
                      VALUES (?,?)''', [node_id, relay_number])
    con.commit()

def set_internal_wifi(node_id, ssid, passwd):
    con = sqlite3.connect(dbPath)
    cursor = con.cursor()
    cursor.execute('''UPDATE NODE SET SSID=?, PASSWORD=? WHERE ID_NODE=?''', [ssid, passwd, node_id])
    con.commit()

def edit_node(node_id, new_name):
    con = sqlite3.connect(dbPath)
    cursor = con.cursor()
    cursor.execute('''UPDATE NODE SET NAME=? WHERE ID_NODE=?''', [new_name, node_id])
    con.commit()

def rows_to_dict(rows):
    d = []
    for r in rows: d.append(dict(r))
    return d


def delete_room(room):
    con = sqlite3.connect(dbPath)
    cursor = con.cursor()
    con.execute("PRAGMA foreign_keys = ON")
    cursor.execute('''DELETE FROM ROOM WHERE ROOM.ID_ROOM=? ''',[room])
    con.commit()