

define([
    'require',
    'jquery',
    'base/js/namespace',
    'base/js/events',
], function(
    requirejs,
    $,
    Jupyter,
    events,
    createRsaKeys
) {

    "use strict";
    
    //********************//
    //* Global variables *//
    //********************//
    
    var prefix = "infrastructure-deployment";
    
    var queues = ["slurm"];
    var commonapps = ["openports","clues","clues2"];
    var applications = ["slurm","compilers","openmpi","nfs","sshkey","onedata"];
    var localApplications = ["slurm","compilers","openmpi","nfs","sshkey","onedata","openports","clues","clues2"]

    var templatesURL = "";
    var localTemplatePrefix = "__local_";
    
    var deployInfo = {};

    var deploying = false; //Stores if the notebook is deploying something
    
    var clearDeployInfo = function(){
	var apps = [];
	if(typeof deployInfo.apps != undefined){
	    apps = deployInfo.apps;
	}
        deployInfo = {
	    "topology": "",
            "user": "",
            "credential": "",
            "deploymentType": "OpenNebula",
	    "host": "",
	    "tenant": "",
	    "id": "",
	    "infName": "cluster-name",
            "frontend":{
                "CPUs":1, //Minimum number of CPUs
		"instance": "",
                "memory": 2048, //in MB
                "flavour": "ubuntu",
                "version": "16.04",
                "image": "",
                "arch": "x86_64",
		"user": "ubuntu",
                "credentials": "ubuntu"
            },
            "worker":{
                "maxNumber": 1, // Maximum number of workers
                "minNumber": 0, // Minimum number of workers
                "CPUs":1, //Minimum number of CPUs
		"instance": "",
                "memory": 1024, //in MB
                "flavour": "ubuntu",
                "version": "16.04",
                "image": "",
                "arch": "x86_64",
		"user": "ubuntu",
                "credentials": "ubuntu",
            },
	    "destroyInterval": 300,
            "apps": apps
        }
    }
    
    var load_css = function(){
        console.log("Loading css");
        var link = document.createElement("link");
        link.type = "text/css";
        link.rel = "stylesheet";
        link.href = requirejs.toUrl("./main.css");
        document.getElementsByTagName("head")[0].appendChild(link);
    }
    
    var createTable = function(obj){
        var keyNames = Object.keys(obj);
        var nkeys = keyNames.length;

        var nElements = 0;
            
        var table = $('<table width="100%" border="5%">');
        
        var row = $("<tr>");
        //Iterate for all object properties and create
        //first row with its names.
        for(let i = 0; i < nkeys; i++){
            var name = keyNames[i];

            //Create column
            var column = $("<th>")
            .append(name)

            //Append column to row
            row.append(column);
            //Check if this property has more elements than previous ones
            if(nElements < obj[name].length){
                nElements = obj[name].length;
            }
        }
        //Apend row to table
        table.append(row);
        

        //Iterate for properties elements to create all element rows
        for(let j = 0; j < nElements; j++){

            var row = $("<tr>");
            for(let i = 0; i < nkeys; i++){
                var name = keyNames[i];

                //Create column
                var column = $("<th>")
                .append(obj[name][j])

                //Append column to row
                row.append(column);
            }
            //Append row to table
            table.append(row)
        }        
        return table;
    }
    
    //****************//
    //*   Buttons    *//
    //****************//
        
    var listDeployments_button = function(){
        console.log("Creating deployments list button");
        if(!Jupyter.toolbar){
            events.on("app_initialized.NotebookApp", listDeployments_button);
            return;
        }
        if($("#listDeployments_button").length === 0){
            Jupyter.toolbar.add_buttons_group([
                Jupyter.actions.register({
                    "help"   : "Deployments list",
                    "icon"   : "fa-list",
                    "handler": toggle_DeploymentList,
                }, "toggle-deployment-list", prefix)
            ]);
        }
    };

    var deploy_button = function(){
        console.log("Creating deploy button");
        if(!Jupyter.toolbar){
            events.on("app_initialized.NotebookApp", deploy_button);
            return;
        }
        clearDeployInfo();
        if($("#deploy_button").length === 0){
            Jupyter.toolbar.add_buttons_group([
                Jupyter.actions.register({
                    "help"   : "Infrastructure deploy",
                    "icon"   : "fal fa-sitemap",
                    "handler": toggle_Deploy,
                }, "toggle-deploy", prefix)
            ]);
        }
    };    
    
    //****************//
    //*   Dialogs    *//
    //****************//    
    
    var create_ListDeployments_dialog = function(show){

	//Check if kernel is available
	if(typeof Jupyter.notebook.kernel == "undefined" || Jupyter.notebook.kernel == null){
	    events.on("kernel_ready.Kernel", function(evt, data) {
		create_ListDeployments_dialog(show);
            });
	    return;
	}
        console.log("Creating deployments list window");
	
	// Get cluster list 

	var callbacks = {
	    iopub : {
		output : function(data){
		    //Check message
		    var check = checkStream(data)
		    if(check < 0) return; //Not a stream
		    if(check > 0){ //Error message
			alert(data.content.text);
			return;
		    }
		    
		    //Successfully execution
		    //console.log("Reviced:")
		    //console.log(data.content.text)

		    //Parse data
		    var words = data.content.text.split(" ");
		    var lists = {};
		    lists["Name"] = [];
		    lists["State"] = [];
		    lists["IP"] = [];
		    lists["Nodes"] = [];

		    for(let i = 5; i < words.length; i+=4){
			lists.Name.push(words[i]);
			lists.State.push(words[i+1]);
			lists.IP.push(words[i+2]);
			lists.Nodes.push(words[i+3]);
		    }
		    
		    var table = createTable(lists);

		    //Check if dialog has been already created
		    if($("#dialog-deployments-list").length == 0){
			var listDeployment_dialog = $('<div id="dialog-deployments-list" title="Deployments list">')
			    .append(table)
			$("body").append(listDeployment_dialog);
			$("#dialog-deployments-list").dialog();
		    } else{
			//Clear dialog
			$("#dialog-deployments-list").empty();

			//Append dable
			$("#dialog-deployments-list").append(table)
			$("#dialog-deployments-list").dialog("open");
		    }
		    if(show == false){
			$("#dialog-deployments-list").dialog("close");
		    }
		}
	    }
	};

	//Create listing script
	var cmd = "%%bash \n";
	cmd += "ec3Out=\"`python2 /usr/local/bin/ec3 list`\"\n";
	//Print ec3 output on stderr or stdout
	cmd += "if [ $? -ne 0 ]; then \n";
	cmd += "    >&2 echo -e $ec3Out \n";	
	cmd += "    exit 1\n";
	cmd += "else\n";
	cmd += "    echo -e $ec3Out \n";	
	cmd += "fi\n";
	
	//console.log(cmd);
	//Deploy using ec3
	var Kernel = Jupyter.notebook.kernel;
	Kernel.execute(cmd, callbacks);
    }
    
    
    var create_Deploy_dialog = function(){
        console.log("Creating deploy window");
        
        var deploy_dialog = $('<div id="dialog-deploy" title="Deploy infrastructure">')
        
        $("body").append(deploy_dialog);
        $("#dialog-deploy").dialog()
        
        //Set initial state
        state_Deploy_Mechanism();
        
        //Close dialog
        $("#dialog-deploy").dialog("close");
    }
    
    // Deploy button states
    var state_Deploy_Mechanism = function(){
        
        //Get dialog
        var deployDialog = $("#dialog-deploy");
        
        //Enable shortcuts
        Jupyter.keyboard_manager.enable();        

        //Clear dialog
        deployDialog.empty();
        
        deployDialog.append($("<p>Select deployment topology</p>"));
        
        deployDialog.dialog("option", "buttons",{
            "Advanced": function() {
		deployInfo.topology = "Advanced";
		//Clear deploy apps selection
		deployInfo.apps = [];
		for(let i = 0; i < deployInfo.apps.length; i++){
			deployInfo.apps.append(commonapps[i])
		}
		state_deploy_provider();
	    },
            "MPI-Cluster": function() {
		deployInfo.topology = "MPI-Cluster";
		//Clear deploy apps selection
		deployInfo.apps = ["nfs","sshkey","compilers","openmpi","onedata","openports"];
		for(let i = 0; i < deployInfo.apps.length; i++){
			deployInfo.apps.append(commonapps[i])
		}		    
		state_deploy_provider();
	    },
            "Batch-Cluster": function() {
		deployInfo.topology = "Batch-Cluster";
		//Clear deploy apps selection
		deployInfo.apps = ["nfs","sshkey","compilers","onedata","openports"];
		for(let i = 0; i < deployInfo.apps.length; i++){
			deployInfo.apps.append(commonapps[i])
		}		    
		state_deploy_provider();
	    }
        });
    }
    
    // select provider function
    var state_deploy_provider = function(){
     
        //Get dialog
        var deployDialog = $("#dialog-deploy");

	//Clear instance type
	deployInfo.frontend.instance = "";
	deployInfo.worker.instance = "";
	
        //Clear dialog
        deployDialog.empty();
        
        //Informative text
        deployDialog.append($("<p>Select infrastructure provider</p>"));
	
        deployDialog.dialog("option", "buttons",{
            "Back": state_Deploy_Mechanism,
            "ONE": function() {

		//Check if the provider has been changed
		if(deployInfo.deploymentType != "OpenNebula"){
		    clearDeployInfo();
		}
		
		deployInfo.id = "one";
		deployInfo.deploymentType = "OpenNebula";
		
		state_deploy_credentials();
	    },
            "EC2": function() {

		//Check if the provider has been changed
		if(deployInfo.deploymentType != "EC2"){
		    clearDeployInfo();
		}
		
		deployInfo.id = "ec2";
		deployInfo.deploymentType = "EC2";
		
		state_deploy_credentials();
	    },
            "OST": function() {

		//Check if the provider has been changed
		if(deployInfo.deploymentType != "OpenStack"){
		    clearDeployInfo();
		}
		
		deployInfo.id = "ost";
		deployInfo.deploymentType = "OpenStack";
		
		state_deploy_credentials();
	    }
        });
    }

    // introduce credentials function
    var state_deploy_credentials = function(){
	
        //Get dialog
        var deployDialog = $("#dialog-deploy");
        
        //Clear dialog
        deployDialog.empty();
        
        //Disable shortcuts
        Jupyter.keyboard_manager.disable();        

        //Create form for input
        var form = $("<form>")
        
        //Informative text
	var text1 = "";
	var text2 = "";
	var text3 = "";
	if(deployInfo.deploymentType == "EC2"){
	    text1 = "<p>Introduce AWS IAM credentials</p>";
	    text2 = "Access Key ID:<br>";
	    text3 = "Secret Access Key:<br>";
	}
	else if(deployInfo.deploymentType == "OpenNebula"){
	    text1 = "<p>Introduce ONE credentials</p>";
	    text2 = "Username:<br>";
	    text3 = "Password:<br>";

            //Create host input field
            form.append("host:<br>");
            form.append($('<input id="hostIn" type="text" value="' + deployInfo.host + '" name="host"><br>'));
	    
	}
	else if(deployInfo.deploymentType == "OpenStack"){
	    text1 = "<p>Introduce OST credentials</p>";
	    text2 = "Username:<br>";
	    text3 = "Password:<br>";

            //Create host input field
            form.append("host:<br>");
            form.append($('<input id="hostIn" type="text" value="' + deployInfo.host + '" name="host"><br>'));
	    //Create tenant (project) input field
            form.append("tenant:<br>");
            form.append($('<input id="tenantIn" type="text" value="' + deployInfo.tenant + '" name="tenant"><br>'));	    	    
	}

        deployDialog.append($(text1));

        //Create username input field
        form.append(text2);
        form.append($('<input id="userIn" type="text" value="' + deployInfo.user + '" name="user"><br>'));

        //Create password input field
        form.append(text3);
        form.append($('<input id="userPassIn" type="password" value="' + deployInfo.credential + '" name="userPass"><br>'));

	deployDialog.append(form);
	
	deployDialog.dialog("option", "buttons",{
            "Back": state_deploy_provider,
	    "Next": function(){
		if(deployInfo.deploymentType == "OpenNebula"){
		    if(deployInfo.host != $("#hostIn").val()){
			deployInfo.frontend.image = ""
			deployInfo.worker.image = ""
			deployInfo.host = $("#hostIn").val();
		    }
		}
		deployInfo.user = $("#userIn").val();
		deployInfo.credential = $("#userPassIn").val();

		if(deployInfo.deploymentType == "EC2"){
		    state_deploy_EC2_instances();
		}
		else if(deployInfo.deploymentType == "OpenNebula"){
		    state_deploy_ONE_frontendSpec();
		}
		else if(deployInfo.deploymentType == "OpenStack"){
		    console.log("on construction...");
		    //deployInfo.tenant = $("#tenantIn").val();
		    //state_deploy_OST_frontendSpec();
		}
	    }
        });
    }

    // state deploy-EC2-instances
    var state_deploy_EC2_instances = function(){

        //Get dialog
        var deployDialog = $("#dialog-deploy");
        
        //Clear dialog
        deployDialog.empty();
        
        //Disable shortcuts
        Jupyter.keyboard_manager.disable();        
        
        //Informative text
        deployDialog.append($("<p>Introduce required EC2 instance types:</p>"));
        
        //Create form for input
        var form = $("<form>")

	var zone = "";
	var ami = "";
	if(deployInfo.frontend.image.length > 0){
	    var words = deployInfo.frontend.image.split('/');

	    if(words.length >= 4){
		zone = words[2];
		ami = words[3];
	    }
	}
	
        //Create availability zone input field
        form.append("Availability zone:<br>");
        form.append($('<input id="availabilityZoneIn" type="text" value="' + zone + '" name="availabilityZone"><br>'));	

	
	//Create AMI input field 
        form.append("AMI:<br>");
        form.append($('<input id="AMIIn" type="text" value="' + ami + '" name="AMI"><br>'));
	
        //Create instance type input field for fronted
        form.append("Frontend instance type:<br>");
        form.append($('<input id="frontendInstanceTypeIn" type="text" value="' + deployInfo.frontend.instance + '" name="frontendInstanceType"><br>'));

        //Create instance type input field for worker
        form.append("Worker instance type:<br>");
        form.append($('<input id="workerInstanceTypeIn" type="text" value="' + deployInfo.worker.instance + '" name="workerInstanceType"><br>'));

	//Append elements to dialog
	deployDialog.append(form);
	
	deployDialog.dialog("option", "buttons",{
            "Back": state_deploy_credentials,
	    "Next": function(){

		//Availability zone
		var AWSzone = $("#availabilityZoneIn").val();
		var AMI = $("#AMIIn").val();
		var imageURL = "aws://" + AWSzone + "/" + AMI;
		
		//Frontend
		deployInfo.frontend.instance = $("#frontendInstanceTypeIn").val();
		deployInfo.frontend.image = imageURL;
		deployInfo.frontend.user = "ubuntu";

		//Worker
		deployInfo.worker.instance = $("#workerInstanceTypeIn").val();
		deployInfo.worker.image = imageURL;
		deployInfo.worker.user = "ubuntu";
		
		state_deploy_app(state_deploy_EC2_instances);
	    }
        });
    }
    
    // state deploy ONE frontendSpec
    var state_deploy_ONE_frontendSpec = function(){
        
        //Get dialog
        var deployDialog = $("#dialog-deploy");
        
        //Clear dialog
        deployDialog.empty();
        
        //Disable shortcuts
        Jupyter.keyboard_manager.disable();        
        
        //Informative text
        deployDialog.append($("<p>Introduce frontend specifications</p>"));
        
        //Create form for input
        var form = $("<form>")
	
        //Create image architecture input field
        form.append("Architecture:<br>");
        form.append($('<input id="imageArchIn" type="text" value="' + deployInfo.frontend.arch + '" name="imageArch"><br>'));
	
        //Create image flavour input field
        form.append("Image flavour:<br>");
        form.append($('<input id="imageFlavourIn" type="text" value="' + deployInfo.frontend.flavour + '" name="imageFlavour"><br>'));

        //Create image version input field
        form.append("Version:<br>");
        form.append($('<input id="imageVersionIn" type="text" value="' + deployInfo.frontend.version + '" name="imageVersion"><br>'));

        //Create CPU input field
        form.append("Minimum CPUs:<br>");
        form.append($('<input id="CPUsIn" type="number" value="' + deployInfo.frontend.CPUs + '" min="1" name="CPUs"><br>'));
	
        //Create memory input field
        form.append("Minimum memory (MB):<br>");
        form.append($('<input id="imageMemIn" type="number" value="' + deployInfo.frontend.memory + '" min="1024" name="imageMem"><br>'));
	
        //Create image url input field
        form.append("Image url:<br>");
	var imageURL = deployInfo.frontend.image;
	if(imageURL.length == 0){
	    if(deployInfo.deploymentType = "OpenNebula"){
		imageURL = "one://" + deployInfo.host + "/";
	    }
	}
        form.append($('<input id="imageUrlIn" type="text" value="' + imageURL + '" name="imageUrl"><br>'));

        //Create image username input field
        form.append("Image username:<br>");
        form.append($('<input id="imageUserIn" type="text" value="' + deployInfo.frontend.user + '" name="imageUser"><br>'));

        //Create image password input field
        form.append("Image user password:<br>");
        form.append($('<input id="imageUserPassIn" type="password" value="' + deployInfo.frontend.credentials + '" name="imageUserPass"><br>'));
	
	deployDialog.append(form);
	
	deployDialog.dialog("option", "buttons",{
            "Back": state_deploy_credentials,
	    "Next": function(){
		deployInfo.frontend.arch = $("#imageArchIn").val();
		deployInfo.frontend.version = $("#imageVersionIn").val();
		deployInfo.frontend.CPUs = $("#CPUsIn").val();
		deployInfo.frontend.memory = $("#imageMemIn").val();
		deployInfo.frontend.flavour = $("#imageFlavourIn").val();
		deployInfo.frontend.image = $("#imageUrlIn").val();

		if($("#imageUserIn").val().length == 0){
		    deployInfo.frontend.user = "";
		}else{
		    deployInfo.frontend.user = $("#imageUserIn").val();
		}
		
		if($("#imageUserPassIn").val().length == 0){
		    deployInfo.frontend.credentials = ""
		}else{
		    deployInfo.frontend.credentials = $("#imageUserPassIn").val();
		}
		
		
		state_deploy_ONE_workerSpec();
	    }
        });
    }

    // state deploy OST frontendSpec
    var state_deploy_OST_frontendSpec = function(){

	//COMPLETAR!!!!
        
        //Get dialog
        var deployDialog = $("#dialog-deploy");
        
        //Clear dialog
        deployDialog.empty();
        
        //Disable shortcuts
        Jupyter.keyboard_manager.disable();        
        
        //Informative text
        deployDialog.append($("<p>Introduce frontend specifications</p>"));
        
        //Create form for input
        var form = $("<form>")
	
        //Create image architecture input field
        form.append("Architecture:<br>");
        form.append($('<input id="imageArchIn" type="text" value="' + deployInfo.frontend.arch + '" name="imageArch"><br>'));
	
        //Create image flavour input field
        form.append("Image flavour:<br>");
        form.append($('<input id="imageFlavourIn" type="text" value="' + deployInfo.frontend.flavour + '" name="imageFlavour"><br>'));

        //Create image version input field
        form.append("Version:<br>");
        form.append($('<input id="imageVersionIn" type="text" value="' + deployInfo.frontend.version + '" name="imageVersion"><br>'));

        //Create CPU input field
        form.append("Minimum CPUs:<br>");
        form.append($('<input id="CPUsIn" type="number" value="' + deployInfo.frontend.CPUs + '" min="1" name="CPUs"><br>'));
	
        //Create memory input field
        form.append("Minimum memory (MB):<br>");
        form.append($('<input id="imageMemIn" type="number" value="' + deployInfo.frontend.memory + '" min="1024" name="imageMem"><br>'));
	
        //Create image url input field
        form.append("Image url:<br>");
	var imageURL = deployInfo.frontend.image;
	if(imageURL.length == 0){
	    if(deployInfo.deploymentType = "OpenStack"){
		imageURL = "one://" + deployInfo.host + "/";
	    }
	}
        form.append($('<input id="imageUrlIn" type="text" value="' + imageURL + '" name="imageUrl"><br>'));

        //Create image username input field
        form.append("Image username:<br>");
        form.append($('<input id="imageUserIn" type="text" value="' + deployInfo.frontend.user + '" name="imageUser"><br>'));

        //Create image password input field
        form.append("Image user password:<br>");
        form.append($('<input id="imageUserPassIn" type="password" value="' + deployInfo.frontend.credentials + '" name="imageUserPass"><br>'));
	
	deployDialog.append(form);
	
	deployDialog.dialog("option", "buttons",{
            "Back": state_deploy_credentials,
	    "Next": function(){
		deployInfo.frontend.arch = $("#imageArchIn").val();
		deployInfo.frontend.version = $("#imageVersionIn").val();
		deployInfo.frontend.CPUs = $("#CPUsIn").val();
		deployInfo.frontend.memory = $("#imageMemIn").val();
		deployInfo.frontend.flavour = $("#imageFlavourIn").val();
		deployInfo.frontend.image = $("#imageUrlIn").val();

		if($("#imageUserIn").val().length == 0){
		    deployInfo.frontend.user = "";
		}else{
		    deployInfo.frontend.user = $("#imageUserIn").val();
		}
		
		if($("#imageUserPassIn").val().length == 0){
		    deployInfo.frontend.credentials = ""
		}else{
		    deployInfo.frontend.credentials = $("#imageUserPassIn").val();
		}
		
		
		state_deploy_ONE_workerSpec();
	    }
        });
    }
    
    // state deploy-one-worker
    var state_deploy_ONE_workerSpec = function(){
        
        //Get dialog
        var deployDialog = $("#dialog-deploy");
        
        //Clear dialog
        deployDialog.empty();
        
        //Disable shortcuts
        Jupyter.keyboard_manager.disable();        
        
        //Informative text
        deployDialog.append($("<p>Introduce worker specifications</p>"));
        
        //Create form for input
        var form = $("<form>")

        //Create CPU input field
        form.append("Minimum CPUs:<br>");
        form.append($('<input id="CPUsIn" type="number" value="' + deployInfo.worker.CPUs + '" min="1" name="CPUs"><br>'));
	
        //Create memory input field
        form.append("Minimum memory (MB):<br>");
        form.append($('<input id="imageMemIn" type="number" value="' + deployInfo.worker.memory + '" min="1024" name="imageMem"><br>'));	
	
	deployDialog.append(form);
	
	deployDialog.dialog("option", "buttons",{
            "Back": state_deploy_ONE_frontendSpec,
	    "Next": function(){

		deployInfo.worker.arch = deployInfo.frontend.arch;
		deployInfo.worker.version = deployInfo.frontend.version;
		deployInfo.worker.flavour = deployInfo.frontend.flavour;
		deployInfo.worker.image = deployInfo.frontend.image;
		deployInfo.worker.user = deployInfo.frontend.user;
		deployInfo.worker.credentials = deployInfo.frontend.credentials;		
		
		deployInfo.worker.memory = $("#imageMemIn").val();
		deployInfo.worker.CPUs = $("#CPUsIn").val();
		
		state_deploy_app(state_deploy_ONE_workerSpec);
	    }
        });
    }

    var state_deploy_app = function(back_function){

        //Get dialog
        var deployDialog = $("#dialog-deploy");
        
        //Clear dialog
        deployDialog.empty();
        
        //Disable shortcuts
        Jupyter.keyboard_manager.disable();        
	
        //Create form for input
        var form = $("<form>");

	//Create queue selector
	var selector = $('<select id="queueSelector" name="queueSelector">');
	for(let i = 0; i < queues.length; i++){
	    let option = $('<option value="' + queues[i] + '">');
	    option.text(queues[i]);
	    selector.append(option);
	}

	//Create cluster name input field
        form.append("Cluster name:<br>");
        form.append($('<input id="clusterNameIn" type="text" value="' + deployInfo.infName + '" name="clusterName"><br>'));

	//Maximum workers input field
        form.append("Max workers:<br>");
        form.append($('<input id="clusterNWorkersIn" type="number" value="1" min="1" name="clusterNWorkers"><br>'));
	//Create workers destroy time input field
        form.append("Workers idle time (s) before shutdown:<br>");
        form.append($('<input id="destroyTimeIn" type="number" value="' + deployInfo.destroyInterval + '" min="0" name="destroyTime"><br>'));
	
        form.append("Queue system:<br>");	
	form.append(selector);

	if(deployInfo.topology == "Advanced"){
	    //Create check boxes with optional app
	    var ul = $('<ul class="checkbox-grid">');
	    for(let i = 0; i < applications.length; i++){

            if(applications[i] == "sshkey"){continue;} //sshkey will be used with nfs
            //Create line
            let line = $('<li style="white-space:nowrap">'); //Force checkbox and label to stay at same line
            //Create checkbox
            let checkbox = $('<input type="checkbox" id="' + applications[i] + '-appCheckID" name="' + applications[i] + '" value="' + applications[i] + '">');
            //Create label
            let label = $('<label for=" ' + applications[i] + '">');
            label.text(applications[i])	    
    
            //Append all to line
            line.append(checkbox);
            line.append(label);

            //Append line to grid
            ul.append(line);
	    }
	}
	
	//Append all to dialog
	deployDialog.append(form);

	if(deployInfo.topology == "Advanced"){
            //Informative text
	    deployDialog.append($("<br>"));
            deployDialog.append($("<p>Select cluster applications</p>"));
	    deployDialog.append($("<br>"));
	    
	    deployDialog.append(ul);
	}
	
	deployDialog.dialog("option", "buttons",{
            "Back": function(){ back_function();},
	    "Deploy": function() {
		if(deploying){
		    alert("Previous deploy has been not finished.")
		    return; //Deploy only one infrastructure at once
		}
		deploying = true;

		//Remove queue systems from apps
		for(let i = 0; i < queues.length; i++){
		    var index = deployInfo.apps.indexOf(queues[i]);
		    if(index > -1){
			deployInfo.apps.splice(index,1);
		    }
		}
		
		//Get specified information
		deployInfo.infName = $("#clusterNameIn").val();
		deployInfo.apps.push($("#queueSelector").val());
		deployInfo.worker.maxNumber = $("#clusterNWorkersIn").val();
		deployInfo.destroyInterval = $("#destroyTimeIn").val();

		//Set minimum number of workers
		deployInfo.worker.minNumber = Math.round(deployInfo.worker.maxNumber/2)
		
		if(deployInfo.topology == "MPI-Cluster"){
		    deployInfo.worker.minNumber = deployInfo.worker.maxNumber;
		}

		if(deployInfo.worker.minNumber > deployInfo.worker.maxNumber){
		    deployInfo.worker.minNumber = deployInfo.worker.maxNumber;
		}

		if(deployInfo.worker.minNumber < 1){
		    deployInfo.worker.minNumber = 1
		}

		//Set applications
		for(let i = 0; i < applications.length; i++){
		    if($("#" + applications[i] + "-appCheckID").length > 0){
			if($("#" + applications[i] + "-appCheckID").is(":checked")){
			    deployInfo.apps.push(applications[i]);
                if(applications[i] == "nfs"){
                    //add "sshkey" too
                    deployInfo.apps.push("sshkey");
                }
			}
		    }
		}

		//Print selected applications
		console.log("Cluster applications: " + deployInfo.apps);

		//Create kernel callback
		var callbacks = {
		    iopub : {
			output : function(data){
			    //Check message
			    var check = checkStream(data)

			    if(check < 0){
				return; //Not a stream
			    }

			    var pubtext = data.content.text.replace("\r","\n");
			    if(check > 0){ //Error message
				deploying = false;
				alert(pubtext);
				console.log(pubtext)
				//Call self function to reconstruct dialog
				state_deploy_app(back_function);
				return;
			    }

			    //Successfully execution
			    deploying = false
			    console.log(pubtext)

			    //Call self function to reconstruct dialog
			    state_deploy_app(back_function);
			}
		    }
		};		

		//Create deploy script
		var cmd = deployEC3Command(deployInfo,templatesURL);
		//console.log(cmd)
		
		//Clear dialog
		deployDialog.empty();
		
		//Show loading spiner
		deployDialog.append($('<div class="loader"></div>'));
		
		//Remove buttons
		deployDialog.dialog("option", "buttons",{})
		
		//Deploy using ec3
	  	var Kernel = Jupyter.notebook.kernel;
		Kernel.execute(cmd, callbacks);
	    }
	});
    }

    var deployEC3Command = function(obj, templateURL){

	var userReplace;
	if(obj.frontend.user.length > 0){
	    userReplace = obj.frontend.user;
	}
	else{
	    userReplace = "root";
	}
	
	var pipeAuth = obj.infName + "-auth-pipe";
	var imageRADL = obj.infName + "-image-spec";
	var cmd = "%%bash \n";
	cmd += "PWD=`pwd` \n";
	//Remove pipes if exist
	cmd += "rm $PWD/" + pipeAuth + " &> /dev/null \n";
	//Create directory for templates
	cmd += "mkdir $PWD/templates &> /dev/null \n";
	//Get necessary local templates
	for(let i = 0; i < localApplications.length; i++)
	{
	    //Check if the deploy needs this template
	    if(obj.apps.indexOf(localApplications[i]) > -1){
		var completeName = localTemplatePrefix + localApplications[i] + ".radl";
		cmd += "curl -s " + templateURL + "/"
		    + completeName
		    +  " > $PWD/templates/" + completeName + " \n";
	    }
	}

	//Change "__MIN_NODES__" in local templates
	cmd += "sed -i -e 's/__MIN_NODES__/" + obj.worker.minNumber + "/g' $PWD/templates/* \n";
	//Change "__USER_NAME__" in local templates
	cmd += "sed -i -e 's/__USER_NAME__/" + userReplace + "/g' $PWD/templates/* \n";
	
	//Create pipes
	cmd += "mkfifo $PWD/" + pipeAuth + "\n";
	//Write data to pipes/files
	cmd += "echo -e \"";

	if(obj.deploymentType == "OpenNebula"){
	    cmd += "description ubuntu_one (\n ";
	} else if ( obj.deploymentType == "EC2"){
	    cmd += "description ubuntu_EC2 (\n ";
	}
	cmd += "kind = 'images' and\n ";

	if(obj.deploymentType == "OpenNebula"){
	    cmd += "short = 'deploy on OpenNebula' and\n \
    content = 'deploy on OpenNebula'\n "
	} else if(obj.deploymentType == "EC2"){
	    cmd += "short = 'deploy on AWS EC2' and\n \
    content = 'deploy on AWS EC2'\n"	    
	}
	cmd += ")\n";
	cmd += "\n";

	//Frontend
	cmd += "system front (\n ";

	//cmd += "ec3_node_type = 'front' and\n ";
	cmd += "disk.0.os.name = 'linux' and\n ";
	
	if(obj.deploymentType == "EC2"){
	    cmd += "instance_type = '" + obj.frontend.instance + "' and\n";

	    //Image url
	    if(obj.frontend.image.length > 0){
		cmd += "disk.0.image.url ='" + obj.frontend.image + "' and\n ";
	    }

	    //Username
	    if(obj.frontend.user.length > 0){
		cmd += "disk.0.os.credentials.username = '" + obj.frontend.user + "'\n ";
	    }
	}
	else if(obj.deploymentType == "OpenNebula"){
	    
	    if(obj.frontend.arch.length > 0){
		cmd += "cpu.arch = '" + obj.frontend.arch + "' and\n ";
	    } else{
		cmd += "cpu.arch = 'x86_64' and\n ";
	    }
	    cmd += "cpu.count >= " + obj.frontend.CPUs + " and\n ";
	    cmd += "memory.size >= " + obj.frontend.memory + "m and\n ";
	    if(obj.frontend.flavour.length > 0){
		cmd += "disk.0.os.flavour = '" + obj.frontend.flavour + "' and\n ";
	    } else {
		cmd += "disk.0.os.flavour = 'ubuntu' and\n ";
	    }
	    if(obj.frontend.version.length > 0){
		cmd += "disk.0.os.version >= '" + obj.frontend.version + "' and\n ";
	    } else {
		cmd += "disk.0.os.version >= '16.04' and\n ";	    
	    }
	    if(obj.frontend.image.length > 0){
		cmd += "disk.0.image.url ='" + obj.frontend.image + "'";
	    }
	    
	    if(obj.frontend.user.length > 0){
		cmd += " and\n disk.0.os.credentials.username = '" + obj.frontend.user + "'";
	    }
	    if(obj.frontend.credentials.length > 0){
		cmd += " and\n disk.0.os.credentials.password = '" + obj.frontend.credentials + "'";
	    }
	    cmd += "\n"
	    
	}
	cmd += ")\n";

	//Workers
	cmd += "system wn (\n ";
	cmd += "ec3_node_type = 'wn' and\n ";
	//cmd += "net_interface.0.connection = 'net'\n ";
	cmd += "ec3_max_instances = " + obj.worker.maxNumber + " and\n ";

	if(obj.deploymentType == "EC2"){
	    cmd += "instance_type = '" + obj.worker.instance + "' and\n";

	    //Image url
	    if(obj.worker.image.length > 0){
		cmd += "disk.0.image.url ='" + obj.worker.image + "' and\n ";
	    }

	    //Username
	    if(obj.worker.user.length > 0){
		cmd += "disk.0.os.credentials.username = '" + obj.worker.user + "'\n ";
	    }
	}
	else if(obj.deploymentType == "OpenNebula"){
	    
	    if(obj.worker.arch.length > 0){
		cmd += "cpu.arch = '" + obj.worker.arch + "' and\n ";
	    } else{
		cmd += "cpu.arch = 'x86_64' and\n ";
	    }

	    cmd += "ec3_destroy_interval = " + obj.destroyInterval + " and\n ";
	    cmd += "cpu.count >= " + obj.worker.CPUs + " and\n ";
	    cmd += "memory.size >=" + obj.worker.memory + "m and\n ";
	    cmd += "disk.0.os.name ='linux' and\n ";

	    if(obj.worker.flavour.length > 0){
		cmd += "disk.0.os.flavour = '" + obj.worker.flavour + "' and\n ";
	    } else {
		cmd += "disk.0.os.flavour = 'ubuntu' and\n ";
	    }
	    if(obj.worker.version.length > 0){
		cmd += "disk.0.os.version >= '" + obj.worker.version + "' and\n ";
	    } else {
		cmd += "disk.0.os.version >= '16.04' and\n ";	    
	    }
	    if(obj.worker.image.length > 0){
		cmd += "disk.0.image.url ='" + obj.worker.image + "'";
	    }
	    
	    if(obj.worker.user.length > 0){
		cmd += " and\n disk.0.os.credentials.username = '" + obj.worker.user + "'";
	    }
	    if(obj.worker.credentials.length > 0){
		cmd += " and\n disk.0.os.credentials.password = '" + obj.worker.credentials + "'";
	    }
	    cmd += "\n"
	    
	}
	
	cmd += ")\n ";
	
	cmd += "\" > ~/.ec3/templates/" + imageRADL + ".radl\n";

	cmd += "echo -e \"id = " + obj.id + "; type = " + obj.deploymentType + "; host = " + obj.host + "; username = " + obj.user + "; password = " + obj.credential + ";\" > $PWD/" + pipeAuth + " & \n"
	//Create final command where the output is stored in "ec3Out"
	cmd += "ec3Out=\"`python2 /usr/local/bin/ec3 launch " + obj.infName + " -a $PWD/" + pipeAuth;
	//Add applications
	for(let i = 0; i < obj.apps.length; i++){
	    //Check if is a local or a ec3 application
	    if(localApplications.indexOf(obj.apps[i]) > -1){		
		cmd += " __local_" + obj.apps[i];
	    } else{
		cmd += " " + obj.apps[i];
	    }
	}
	cmd += " " + imageRADL + " --yes`\" \n";
	//cmd += " --dry-run";

	//Remove pipe and radl
	cmd += "rm $PWD/" + pipeAuth + " &> /dev/null \n";
	cmd += "rm -r $PWD/templates &> /dev/null \n";

	//Print ec3 output on stderr or stdout
	cmd += "if [ $? -ne 0 ]; then \n";
	cmd += "    >&2 echo -e $ec3Out \n";	
	cmd += "    exit 1\n";
	cmd += "else\n";
	cmd += "    echo -e $ec3Out \n";	
	cmd += "fi\n";
	
	return cmd;
    }

    var checkStream = function(data){
	if(data.msg_type == "stream"){
	    if(data.content.name == "stdout"){
		//Is a stdout message
		return 0;
	    }else{
		//Is a error message
		return 1;
	    }
	}
	//Is not a stream message
	return -1;
    }
    
    //****************//
    //*Dialogs handle*//
    //****************//    
    
    
    var toggle_DeploymentList = function(){
        if($("#dialog-deployments-list").dialog("isOpen")){
            $("#dialog-deployments-list").dialog("close");
        } else{
	    create_ListDeployments_dialog(true);
            $("#dialog-deployments-list").dialog("moveToTop");                        
        }
        Jupyter.notebook.set_dirty();
    }
    

    var toggle_Deploy = function(){
        if($("#dialog-deploy").dialog("isOpen")){
            $("#dialog-deploy").dialog("close");            
        } else{
            $("#dialog-deploy").dialog("open");                        
            $("#dialog-deploy").dialog("moveToTop");                        
        }
        Jupyter.notebook.set_dirty();
    }
    
    //*******************//
    //* Jupyter handler *//
    //*******************//        
    
    
    var load_jupyter_extension = function() {
        console.log("Initialize deployment plugin");
        load_css();
	
	//Get local radl directory
	var url = requirejs.toUrl("./templates");
	templatesURL = location.protocol + '//' + location.host
	    + url.substring(0, url.lastIndexOf('/'))
	    + "/templates";
	console.log("Templates url: " + templatesURL);
	
	listDeployments_button();
        deploy_button();
        create_ListDeployments_dialog(false);
        create_Deploy_dialog();
    }

    return {
        load_ipython_extension: load_jupyter_extension
    };
});
