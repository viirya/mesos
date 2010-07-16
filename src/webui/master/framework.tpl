%from datetime import datetime
%from master import get_master
%from webui_lib import *
%
%master = get_master()
<html>
<head>
<title>Framework {{framework_id}} on {{HOSTNAME}}</title>
<!-- Combo-handled YUI CSS files: -->
<link rel="stylesheet" type="text/css" href="/yui_2.8.1/build/paginator/assets/skins/sam/paginator.css">
<link rel="stylesheet" type="text/css" href="/yui_2.8.1/build/datatable/assets/skins/sam/datatable.css">

<!-- Combo-handled YUI JS files: -->
<script type="text/javascript" src="/yui_2.8.1/build/yahoo-dom-event/yahoo-dom-event.js"></script>
<script type="text/javascript" src="/yui_2.8.1/build/connection/connection-min.js"></script>
<script type="text/javascript" src="/yui_2.8.1/build/datasource/datasource-min.js"></script>
<script type="text/javascript" src="/yui_2.8.1/build/element/element-min.js"></script>
<script type="text/javascript" src="/yui_2.8.1/build/paginator/paginator-min.js"></script>
<script type="text/javascript" src="/yui_2.8.1/build/datatable/datatable-min.js"></script>
<script type="text/javascript" src="/yui_2.8.1/build/history/history-min.js"></script>
<script type="text/javascript" src="/yui_2.8.1/build/json/json-min.js"></script>

<script type='text/javascript'>
//Copied from http://developer.yahoo.com/yui/examples/datatable/dt_xhrjson.html

// Custom parser 
var timestampToDate = function(sTimestamp) { 
    //convert from microseconds to milliseconds
    return new Date(sTimestamp/1000); 
}; 

var frameworkToURL = function(fwName) {
   return "http://www.andykonwinski.com"; 
};

//SETUP TASKS TABLE
YAHOO.util.Event.addListener(window, "load", function() {
  YAHOO.example.XHR_JSON = new function() {

    // Define columns
    var myColumnDefs = [
        {key:"taskid", label:"Task ID", sortable:true},
        {key:"fwid", label:"FW ID", sortable:true},
        {key:"datetime_created", label:"Date-time created", sortable:true, formatter:YAHOO.widget.DataTable.formatDate}, 
        {key:"resource_list.cpus", label:"Num Cores", sortable:true, formatter:YAHOO.widget.DataTable.formatNumber},
        {key:"resource_list.mem", label:"Memory(MB)", sortable:true, formatter:YAHOO.widget.DataTable.formatNumber}
    ];

    // DataSource instance
    myTasksDataSource = new YAHOO.util.DataSource("../tasks_json?");
    myTasksDataSource.responseType = YAHOO.util.DataSource.TYPE_JSON;
    myTasksDataSource.connXhrMode = "queueRequests";
    myTasksDataSource.responseSchema = {
        resultsList: "ResultSet.Items",
        fields: ["taskid",
                 "fwid",
                 {key:"datetime_created",parser:timestampToDate},
                 {key:"resource_list.cpus",parser:"number"},
                 {key:"resource_list.mem",parser:"number"}]
    };

    // A custom function to translate the js paging request into a query
    // string sent to the XHR DataSource
    var buildQueryString = function (state,dt) {
        return "startIndex=" + state.pagination.recordOffset +
               "&results=" + state.pagination.rowsPerPage +
               "&fwid=" + {{framework_id}};
    };

    // DataTable configurations
    var myConfig = {
      // Create the Paginator
       paginator: new YAHOO.widget.Paginator({
          template : "{PreviousPageLink} {CurrentPageReport} {NextPageLink} {RowsPerPageDropdown}",
          pageReportTemplate : "Showing items {startIndex} - {endIndex} of {totalRecords}",
          rowsPerPage: 20,
          rowsPerPageOptions: [20,50,100,200,500]
      }),
      initialRequest: 'startIndex=0&results=20&fwid={{framework_id}}',
      generateRequest: buildQueryString
    }; 

    // Instantiate DataTable
    var myDataTable = new YAHOO.widget.DataTable("tasks_table", myColumnDefs,
            myTasksDataSource, myConfig);

  };
});

</script>

<link rel="stylesheet" type="text/css" href="/static/stylesheet.css" />
</head>
<body>

<h1>Framework {{framework_id}} on {{HOSTNAME}}</h1>

%# Find the framework with the requested ID
%framework = None
%for i in range(master.frameworks.size()):
%  if master.frameworks[i].id == framework_id:
%    framework = master.frameworks[i]
%  end
%end

%# Build a dict from slave ID to slave for quick lookups of slaves
%slaves = {}
%for i in range(master.slaves.size()):
%  slave = master.slaves[i]
%  slaves[slave.id] = slave
%end

%if framework != None:
  <p>
  Name: {{framework.name}}<br />
  Connected: {{format_time(framework.connect_time)}}<br />
  Executor: {{framework.executor}}<br />
  Running Tasks: {{framework.tasks.size()}}<br />
  CPUs: {{framework.cpus}}<br />
  MEM: {{format_mem(framework.mem)}}<br />
  </p>

  <h2>Running Tasks</h2>

  %# TODO: sort these by task id
  %if framework.tasks.size() > 0:
    <table>
    <tr>
    <th>ID</th>
    <th>Name</th>
    <th>State</th>
    <th>Running On</th>
    </tr>
    %for i in range(framework.tasks.size()):
      %task = framework.tasks[i]
      <tr>
      <td>{{task.id}}</td>
      <td>{{task.name}}</td>
      <td>{{TASK_STATES[task.state]}}</td>
      %if task.slave_id in slaves:
        %s = slaves[task.slave_id]
        <td><a href="http://{{s.public_dns}}:8081/">{{s.public_dns}}</a></td>
      %else:
        <td>Slave {{task.slave_id}} (disconnected)</td>
      %end
      </tr>
    %end
    </table>

  %else:
    <p>No tasks are running.</p>
  %end
%else:
  <p>No framework with ID {{framework_id}} is connected.</p>
%end


  <h2>Task History</h2>
  <!--div id='chart_div' style='width: 700px; height: 240px;'></div>-->
  <div id="tasks_table"></div>

<p><a href="/">Back to Master</a></p>

</body>
</html>
