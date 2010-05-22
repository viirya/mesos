import bottle
import commands
import datetime
import gviz_api

from bottle import route, send_file, template, response

start_time = datetime.datetime.now()


@route('/')
def index():
  bottle.TEMPLATES.clear() # For rapid development
  return template("index", start_time = start_time)


@route('/framework/:id#[0-9]*#')
def framework(id):
  bottle.TEMPLATES.clear() # For rapid development
  return template("framework", framework_id = int(id))


@route('/static/:filename#.*#')
def static(filename):
  send_file(filename, root = './webui/static')


@route('/log/:level#[A-Z]*#')
def log_full(level):
  send_file('nexus-master.' + level, root = '/tmp',
            guessmime = False, mimetype = 'text/plain')


@route('/log/:level#[A-Z]*#/:lines#[0-9]*#')
def log_tail(level, lines):
  bottle.response.content_type = 'text/plain'
  return commands.getoutput('tail -%s /tmp/nexus-master.%s' % (lines, level))

@route('/utilization_table')
def utilization_table():
  description = [("datetime", "datetime", "datetime"),
                 ("fw_id", "number", "fw_id"),
                 ("fw_name", "string", "fw_name"),
                 ("cpus", "number", "cpus")]
  dt = gviz_api.DataTable(description)

  #read in the history of master utilization,   FILE = open("/mnt/shares",'r')
  datarow = ""
  fws = {}
  for line in FILE:
    row = line.split("#")
    #here is what we get from /mnt/shares: tick, timestamp in microseconds, f->id, f->name,  f->cpus,  f->mem, cpu_share, mem_share, max_share
    if len(row)>1:
      timestamp = datetime.datetime.fromtimestamp(float(row[1])/1000000.0)
      #fws[int(row[2])] = {'datetime': timestamp, 'fw_id':int(row[2]), 'fw_name':row[3], 'cpus':int(row[4]), 'mem':int(row[5]), 'cpu_share':float(row[6]), 'mem_share'}
    
  #Now make a data table to hold the last 100 frameworks, and add data (date, util) points to the
  #data table which will be consumed by the visualization on the nexus webui
      #datarow = datarow + row[0] + ", " + row[1] + ", date_time: " + str(date_time) + "\n" 
      #data_row = [[date_time, int(row[2]), row[3], int(row[4])]]
      #dt.AppendData(data_row)
  
    #since the format that annotatedtimeline requires is 
    #(datetime or date, y-val, [annotation_title, annotation text]),
    #lets only send a filtered view of this data table
  #response.header['Content-Type'] = 'text/plain' 
  #return dt.ToJSonResponse()
  #return datarow
  return ""


bottle.TEMPLATE_PATH.append('./webui/master/%s.tpl')
bottle.run(host = '0.0.0.0', port = 8080)
