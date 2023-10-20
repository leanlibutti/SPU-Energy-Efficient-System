import random
import string
import socket
from threading import Thread, get_ident, Lock
import database as db
import time
import datetime
import thinger

mutex = Lock() # Para evitar que varios threads traten de comunicarse con un nodo a la vez

devices = []

server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
server.setblocking(True)
server.bind(('0.0.0.0' , 8081))

def id_generator(size=10, chars=string.ascii_uppercase + string.digits):
  return ''.join(random.choice(chars) for _ in range(size))

def begin():
  server.listen(128)
  Thread(target=listen, args=[]).start()
  print ("Listening")

def listen():
  global server
  while True:
    try:
      conn, addr = server.accept()
      conn.settimeout(5.0)
      node = Node(addr, conn)
      devices.append(node)
      print ("Node {0} connected".format(node.name))
      if node.name == 'unnamed':
        newName = id_generator()
        while db.get_node_by_name(newName): newName = id_generator()
        db.create_node(newName)
        thinger.createBucket(newName+"Groups", newName+" groups", "Group info for node {0}".format(newName))
        Thread(target=node.setup, args=[newName]).start()
      else: 
        node.id = db.get_node_by_name(node.name)
        Thread(target=node.checkTime, args=[]).start()
    except socket.timeout:
      print("Timeout en accept")
      pass

def getNodeByName(nodeName):
    for n in devices:
        if n.name == nodeName:
          return n
    return None

def getNodeById(nodeId):
    for n in devices:
        if n.id == int(nodeId): return n
    return None

def shutdown():
  global close
  close = True

def clear():
  global devices
  for d in devices:
    d.connection.close()
  devices = []

class Node():
  def __init__(self, ipAddr, conn):
    self.ip = ipAddr[0]
    self.connection = conn
    self.name = self.getName();
    self.id = 0;

  def setup(self, newName):
    if not self.setName(newName): return False
    self.id = db.get_node_by_name(self.name)
    info = self.getRelayInfo()
    if info: master_relays, slave_relays = info
    else: return False
    for n in range(master_relays): db.add_relay_to_node(self.id, n)
    slave_info = self.getSlaveInfo()
    if slave_relays and slave_info:
      if slave_info[0] != "no_slaves":
        for slave in range(len(slave_info)): db.create_slave(slave, slave_info[slave], slave_relays)
    else: return False
    if self.changeInternalWifi(self.name, '0123456789'):
      db.set_internal_wifi(self.id, self.name, '0123456789')
    else: return False
    return self.checkTime()

  def disconnect(self):
    self.connection.close()
    try: devices.remove(self)
    except: pass
    return False

  def checkTime(self):
    time = self.getTime()
    if time:
      print("Time ")
      print(time)
      now = datetime.datetime.now()
      if now.hour != time[0] or now.minute != time[1] or now.day != time[3] or now.month != time[4] or now.year != time[5]:
        return self.changeTime(now.hour, now.minute, now.day, now.month, now.year)
      else: return True
    else: return False

  def startPairing(self, seconds=60):
    return self.send("pairing_mode&{0}".format(seconds))

  def sendAndReceive(self, text):
    print("Enviando {}".format(text))
    mutex.acquire()
    try:
      send = self.connection.send((text+"\n").encode())
      length = int(self.connection.recv(4).decode())
      response_string = self.connection.recv(1024).decode()
      while len(response_string) < length:
        response_string += self.connection.recv(1024).decode()
      response = response_string.split('\n',1)[0].split('&');
      response.pop()
      return response
    except (socket.error, socket.timeout, ValueError) as exc:
      print("Error en envio o recepcion: {}, {}".format(type(exc), exc))
      return self.disconnect()
    finally:
      mutex.release()

  def send(self, text):
    response = self.sendAndReceive(text)
    return response and (response[0] == "ok")

  def receive(self):
    try:
      response = self.connection.recv(256).decode().split('\n',1)[0].split('&');
      response.pop()
      return response
    except socket.error:
      print("Socket error en receive")
      devices.remove(self)
      return False
    except socket.timeout:
      print("Timeout en receive")
      self.connection.close()
      devices.remove(self)
      return False

  def getName(self):
    res = self.sendAndReceive("name")
    if res: return res[0]
    else: return False

  def setName(self, newName):
    if self.send("name&{0}".format(newName)):
      self.name = newName
      return True
    else: return False

  def createGroup(self, name, ignorePir, motionDetecionTime=20):
    if(ignorePir):
      message = "create_group&{0}&ignore_pir".format(name)
    else:
      message = "create_group&{0}&{1}".format(name, motionDetecionTime)
    return self.send(message)

  def deleteGroup(self, name):
    return self.send("delete_group&{0}".format(name))

  def modifyGroup(self, name, newName, ignorePir, motionDetecionTime=20):
    if(ignorePir):
      message = "modify_group&{0}&{1}&ignore_pir".format(name, newName)
    else:
      message = "modify_group&{0}&{1}&{2}".format(name, newName, motionDetecionTime)
    return self.send(message)

  def turnOnGroup(self, group):
    message = "turnon_group&{0}".format(group)
    return self.send(message)
  
  def turnOffGroup(self, group):
    message = "turnoff_group&{0}".format(group)
    return self.send(message)

  def getGroupsInfo(self):
    info = self.sendAndReceive("groups")
    if info and info[0] != "no_groups":
      groups = []
      for r in info:
        groups.append({ 'name':r.split(' ')[0], 'state':r.split(' ')[1] })
      return groups
    elif info: return "no_groups"
    else: return False

  def getRelayInfo(self):
    info = self.sendAndReceive("relay_info")
    if info:
      return [int(info[0]), int(info[1])]
    else: return False

  def changeExternalWifi(self, ssid, password, ip):
    message = 'extwifi&{0}&{1}&{2}'.format(ssid, password, ip)
    return self.send(message)

  def addWeeklyAlarm(self, day, startHour, startMinute, finishHour, finishMinute, t, group=0):
    message = 'weekly_alarm&{0}&{1}&{2}&{3}&{4}&{5}&{6}'.format(day, startHour, startMinute, finishHour, finishMinute, t, group)
    data = self.sendAndReceive(message)
    if data is not False:
      return data[0]
    return False

  def addAlarmOnce(self, day, month, year, startHour, startMinute, finishHour, finishMinute, t, group=0):
    message = 'alarm_once&{0}&{1}&{2}&{3}&{4}&{5}&{6}&{7}&{8}'.format(day, month, year, startHour, startMinute, finishHour, finishMinute, t, group)
    data = self.sendAndReceive(message)
    if data is not False:
      return data[0]
    return False

  def deleteAlarm(self, alarmId):
    message = "delete_alarm&{0}".format(alarmId)
    return self.send(message)

  def connectMasterPir(self):
    return self.send("connect&master&pir")

  def connectMasterTemp(self):
    return self.send("connect&master&temp")

  def connectMasterRelay(self, number, group):
    return self.send("connect&master&relay&{0}&{1}".format(number, group))

  def disconnectMasterPir(self):
    return self.send("disconnect&master&pir")

  def disconnectMasterTemp(self):
    return self.send("disconnect&master&temp")

  def disconnectMasterRelay(self, number):
    return self.send("disconnect&master&relay&{0}".format(number))

  def connectSlavePir(self, slaveNumber):
    return self.send("connect&{0}&pir".format(slaveNumber))

  def connectSlaveTemp(self, slaveNumber):
    return self.send("connect&{0}&temp".format(slaveNumber))

  def connectSlaveIrControl(self, slaveNumber, group):
    return self.send("connect&{0}&ircontrol&{1}".format(slaveNumber, group));

  def connectSlaveRelay(self, slaveNumber, relayNumber, group):
    return self.send("connect&{0}&relay&{1}&{2}".format(slaveNumber, relayNumber, group))

  def disconnectSlavePir(self, slaveNumber):
    return self.send("disconnect&{0}&pir".format(slaveNumber))

  def disconnectSlaveTemp(self, slaveNumber):
    return self.send("disconnect&{0}&temp".format(slaveNumber))

  def disconnectSlaveRelay(self, slaveNumber, relayNumber):
    return self.send("disconnect&{0}&relay&{1}".format(slaveNumber, relayNumber))

  def disconnectSlaveIrControl(self, slaveNumber):
    return self.send("disconnect&{0}&ircontrol".format(slaveNumber));

  def changePirSettings(self, inactivityTime):
    return self.send("pir_settings&{}".format(inactivityTime))

  def getPirSettings(self):
    return self.sendAndReceive("pir_settings")

  def getAmbientData(self):
    data = self.sendAndReceive("temperature"); print(data)
    if data:
      if data[0] != "no_sensor":
        return {"temperature": data[0], "humidity": data[1]}
      else:
        return {"temperature": data[0], "humidity": data[0]}
    else:
      return False

  def getSlaveInfo(self):
    return self.sendAndReceive("slaves")

  def getTime(self):
    return self.sendAndReceive("time")

  def changeTime(self, hour, minute, day, month, year):
    return self.send("time&{0}&{1}&{2}&{3}&{4}".format(hour, minute, day, month, year))

  def getInternalWifi(self):
    return self.sendAndReceive("intwifi")

  def changeInternalWifi(self, ssid, password):
    return self.send("intwifi&{0}&{1}".format(ssid, password))

  def resetMaster(self):
    r = self.send("reset")
    devices.remove(self)
    return r

  def recordIrCode(self):
    return self.send("record_ir")

  def sendIrCode(self, code):
    message = "ir_transmit&{0}&{1}".format("&".join(map(str,code)), -1)
    return self.send(message)

  def getIrCode(self):
    code = self.sendAndReceive("ir_transmit_server")
    if code:
      if code[0] == "no_code":
        return []
      else:
        return list(map(int, filter(lambda a: a != '', code))) # Retorna el codigo como una lista de integers
    else:
      return False

  def clearEEPROM(self):
    return self.send("clear_eeprom")
