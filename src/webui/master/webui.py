import sys
import bottle
import commands
import datetime
import sqlite3
import json
import os

from bottle import route, send_file, template, response, request

start_time = datetime.datetime.now()
database_location = os.getcwd() + '/logs/event_history_db.sqlite3'


@route('/')
def index():
  bottle.TEMPLATES.clear() # For rapid development
  return template("index", start_time = start_time)


@route('/framework/:id')
def framework(id):
  taskid = request.GET.get('taskid', '').strip()
  bottle.TEMPLATES.clear() # For rapid development
  return template("framework", framework_id = id, task_id = taskid)


@route('/static/:filename#.*#')
def static(filename):
  send_file(filename, root = './webui/static')


@route('/yui_2.8.1/build/:filename#.*#')
def static(filename):
  send_file(filename, root = './webui/third_party/yui_2.8.1/build')


@route('/log/:level#[A-Z]*#')
def log_full(level):
  send_file('mesos-master.' + level, root = '/tmp',
            guessmime = False, mimetype = 'text/plain')


@route('/log/:level#[A-Z]*#/:lines#[0-9]*#')
def log_tail(level, lines):
  bottle.response.content_type = 'text/plain'
  return commands.getoutput('tail -%s /tmp/mesos-master.%s' % (lines, level))


#generate list of task history using JSON
@route('/tasks_json')
def tasks_json():
  fwid = request.GET.get('fwid', '').strip()
  try:
    conn = sqlite3.connect(database_location)
  except:
    return "Error opening database at " + database_location
  conn.row_factory = sqlite3.Row
  c = conn.cursor()
  #TODO(andyk): prevent SQL injection! This is probably dangerous!
  c.execute("SELECT * FROM task WHERE fwid = \"" + fwid + "\" ORDER BY \"datetime_created\" DESC;")
  result = c.fetchall()
  json_string = "{\"ResultSet\": {\"TotalItems\":" + str(len(result)) + ", \"Items\":[\n"
  k = 0
  for row in result:
    k += 1
    json_string += "\t\t{\n"
    i = 0
    for col in row.keys():
      i += 1
      if col == "resource_list":
        json_string += "\t\t\"" + col + "\":" + str(row[col])
      else:
        json_string += "\t\t\"" + col + "\":\"" + str(row[col]) + "\""
      if i != len(row.keys()):
        json_string += ","
      json_string += "\n"
    json_string += "\t\t}"
    if k != len(result):
      json_string += ","
    json_string += "\n"
  json_string += "\t]}\n}"
  response.header['Content-Type'] = 'text/plain' 
  return str(json_string)


#generate list of state updates for this task
@route('/task_details_json')
def tasks_json():
  fwid = request.GET.get('fwid', '').strip()
  taskid = request.GET.get('taskid', '').strip()
  try:
    conn = sqlite3.connect(database_location)
  except:
    return "Error opening database at " + database_location
  conn.row_factory = sqlite3.Row
  c = conn.cursor()
  #TODO(andyk): prevent SQL injection! This is probably dangerous!
  c.execute("SELECT * FROM taskstate WHERE fwid = \"" + fwid + "\" and taskid = \"" + taskid + "\";")
  result = c.fetchall()
  json_string = "{\"ResultSet\": {\"TotalItems\":" + str(len(result)) + ", \"Items\":[\n"
  k = 0
  for row in result:
    k += 1
    json_string += "\t\t{\n"
    i = 0
    for col in row.keys():
      i += 1
      json_string += "\t\t\"" + col + "\":\"" + str(row[col]) + "\""
      if i != len(row.keys()):
        json_string += ","
      json_string += "\n"
    json_string += "\t\t}"
    if k != len(result):
      json_string += ","
    json_string += "\n"
  json_string += "\t]}\n}"
  response.header['Content-Type'] = 'text/plain' 
  return str(json_string)


#generate list of framework history using JSON
@route('/frameworks_json')
def frameworks_json():
  try:
    conn = sqlite3.connect(database_location) 
  except:
    return "Error opening database at " + database_location
  conn.row_factory = sqlite3.Row
  c = conn.cursor()
  c.execute("SELECT * FROM framework ORDER BY \"datetime_registered\" DESC;")
  result = c.fetchall()
  json_string = "{\"ResultSet\": {\"TotalItems\":" + str(len(result)) + ", \"Items\":[\n"
  k = 0
  for row in result:
    k += 1
    json_string += "\t\t{\n"
    i = 0
    for col in row.keys():
      i += 1
      json_string += "\t\t\"" + col + "\":\"" + str(row[col]) + "\""
      if i != len(row.keys()):
        json_string += ","
      json_string += "\n"
    json_string += "\t\t}"
    if k != len(result):
      json_string += ","
    json_string += "\n"
  json_string += "\t]}\n}"
  response.header['Content-Type'] = 'text/plain' 
  return str(json_string)

bottle.TEMPLATE_PATH.append('./webui/master/%s.tpl')
if sys.argv[1]:
  webui_port = sys.argv[1] 
else:
  webui_port = 8080
bottle.run(host = '0.0.0.0', port = webui_port)
