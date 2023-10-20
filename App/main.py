import webapp2, threading, time, socket
from jinja2 import Environment, FileSystemLoader

templateEnv = Environment(loader=FileSystemLoader('./html'))

server = socket.socket(socket.AF_INET, socket.SOCK_STREAM);
server.setblocking(True)
server.bind(('192.168.0.100', 8081))
server.listen(10)

devices = []

class NodeList(webapp2.RequestHandler):
  def get(self):
    template = templateEnv.get_template('index.html')
    self.response.write(template.render(devices=devices))


class NodeSend(webapp2.RequestHandler):
  def get(self, ip, text):
    d = getNode(ip)
    d.send(text)
    self.redirect('/')

def clearNodes(g):
  global devices
  for d in devices:
    d.connection.close()
  devices = []

app = webapp2.WSGIApplication([
  webapp2.Route(r'/', NodeList, name='node-list'),
  webapp2.Route(r'/<ip>/<text>', NodeSend, name='node-list'),
  webapp2.Route(r'/clear', clearNodes)
  ], debug=True)


class Node():
  def __init__(self, ipAddr, conn):
    self.ip = ipAddr[0]
    self.connection = conn

  def send(self, text):
    self.connection.send(text+"\n")

def main():
  from paste import httpserver
  thread = threading.Thread(target=httpserver.serve, args=(app, '192.168.0.100', '8080'))
  thread.start()
  while True:
    conn, addr = server.accept()
    node = Node(addr, conn)
    devices.append(node)

def getNode(ip):
  for d in devices:
      if(d.ip == ip):
        return d
  return None

if __name__ == '__main__':
  main()
