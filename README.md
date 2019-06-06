# APRICOT: Advanced Platform for Reproducible Infrastructures in the Cloud via Open Tools

## Introduction 

APRICOT is an open-source extension to support customised virtual infrastructure deployment and usage from Jupyter notebooks. It allows multi-cloud infrastructure provisioning using a wizard-like GUI that guides the user step by step through the deployment process. It implements Ipython magic functionality to use and manage the deployed infrastructures within Jupyter notebooks for increased usability.

## Experiment replication methodology

A generic computational experimentation involves the elements shown in the following figure. So, to be able to reproduce any experiment, any researcher needs:

- Required input data or a method to produce it.

- Specific hardware needs, like memory requirements, GPGPUs, FPGAs, multi-node cluster, etc.

- A list of all the required software and their configuration, such as MPI on a cluster, specific language compilers, specific software or libraries, source codes, etc.

- Clear instructions on how to reproduce the entire experiment.

![Alt text](docs/images/experiment.png?raw=true "Experimentation")

APRICOT can be used to achieve reproducible experiments for experiments that require complex customised computing infrastructures using Jupyter notebooks. The key points to develop reproducible experiments using APRICOT extensions are:


 - Required data must be provided using external storage systems or a notebook with instructions to create it. APRICOT has been configured to use OneData as external storage provider.

 - APRICOT provides a set of predefined configurable infrastructures to fit the experiments. Any researcher can easily deploy  the same computing infrastructure than the one used in a previous experimentation carried out with the deployed infrastructure in APRICOT.

 - APRICOT allows remote execution of commands at the deployed infrastructures to ease interaction. So, extra needed software can be documented and installed at the infrastructure within the same Jupyter notebook where the experimentation has been documented in order to be executed step by step. 

 - Since APRICOT extension uses Jupyter notebooks as base environment, all the experiment can be documented using text, life code and images.

## Components

![Alt text](docs/images/APRICOT_components.png?raw=true "Components")

APRICOT has been constructed using the following components:

- [**Jupyter**](https://jupyter.org/), an open-source web application that allows you to create and share documents that contain live code, equations, visualizations and narrative text. 
- [**CLUES**](https://github.com/grycap/clues), an energy management system for High Performance Computing (HPC) Clusters and Cloud infrastructures.
- [**EC3**](https://servproject.i3m.upv.es/ec3/), an open-source tool to deploy compute clusters that can horizontally scale in terms of number of nodes with multiple plugins.
- [**IM**](https://www.grycap.upv.es/im/index.php), an open-source virtual infrastructure provisioning tool for multi-Clouds.
- [**ONEDATA**](https://github.com/grycap/clues), global data storage backed by computing centers and storage providers worldwide.

## Installation

### Requisites

APRICOT requires the EC3 client to deploy the infrastructure and get the access credentials. The installation details can be found at [EC3 documentation](https://ec3.readthedocs.io/en/devel/intro.html#installation).

Also, APRICOT requires a [Jupyter installation](https://jupyter.org/install), since uses its environment to run. It is compatible with Jupyter 4.x and 5.x versions.

### Installing

To install APRICOT package, run the provided install script

`` bash install.sh``

## Deployment

Infrastructure deployment with APRICOT is done using a notebook extension. This one, creates the button showed at next image, which opens a GUI to guide the user step by step trough deployment process.


![Alt text](docs/images/pluginDeploy.png?raw=true "Deploy plugin")


Deployment process include,

- Select infrastructure topology between a set of predefined infrastructures. At the moment, predefined topologies are Batch-Cluster, MPI-Cluster and Advanced which allows a custom configuration.
- Select a cloud provider where deploy the infrastructure. Actual supported providers are OpenNebula and AWS.
- Depending on the selected provider, introduce the required access credentials.
- Specify frontend and workers specifications such as memory, CPUs, OS image etc.
- Set infrastructure features like maximum number of workers or cluster identifier name.
- Deploy the infrastructure.

![Alt text](docs/images/deploySteps.png?raw=true "Deploy steps")

If any error occurs during deployment, this will be shown in the web console. Also, it is possible to get cluster configuration logs using implemented magics functions or get them directly using EC3 client via a terminal or bash magics into the notebook.

## Infrastructure management

To manage and use previous deployed infrastructures within Jupyter notebook environment, a set of Ipython magic functions have been implemented. These functions are listed below:

* Magic lines:
    * **apricot_genMPid**:
        * Arguments: A list of ranges where each one has the format *lowest highest step*
        * Returns: A identifier created with input ranges
    * **apricot_log**:
        * Arguments: Cluster name identifier
        * Returns: The configuration logs of specified cluster
    * **apricot_onedata**:
        * Arguments: Cluster name identifier, selected command and command parameters. Valid commands are,
            * mount: Takes a mount point, Oneprovider host and Onedata token to create and mount the specified space at mount point.
            * umount: Takes a mounted point as argument and umount it.
            * download: Copy files from Onedata space path (first argument) to local path (second argument). 
            * upload: Copy files from local path (first argument) to Onedata space path (second argument). 
    * apricot_runMP: Executes the specified [sbatch script](https://slurm.schedmd.com/sbatch.html) replacing text seeds *__N__*, where N is the range number, by all values specified in corresponding input ranges. Each range follows the same format as in *apricot_genMPid*. So, for three ranges with *N1*, *N2* and *N3* number of possible values respectively, *apricot_runMP* will execute sbatch script *Nt = N1 N2 N3* times.
        * Arguments: Cluster name identifier, script path, range1, range2...
    * **apricot_ls**: Takes no arguments and returns a list with all the deployed clusters using EC3. Internally, executes a *ec3 list*.
    * **apricot_nodels**:
        * Arguments: Cluster name identifier.
        * Return: A list of working nodes and their status at the specified cluster.
    * **apricot_upload**: Upload specified local files into the specified cluster destination path.
        * Arguments: Cluster name identifier, upload files paths, destination path.
    * **apricot_download**: Download files located at specified cluster to local storage.
        * Arguments: Cluster name identifier, download files paths, local destination path.
 * Magic line and cell:
    * **apricot**: Perform multiple tasks depending on input command.
        * exec: Takes as arguments a cluster name identifier and a command to be executed in the specified cluster. This call is syncronous.
        * execAsync: Same as *exec* but the call is done asyncronous.
        * list: Same as *apricot_ls*
        * destroy: Take a cluster name identifier as argument an destroys the infrastructure.

Like any Jupyter magics, these must be lodaded at the notebook using *%reload_ext apricot_magic* or configure jupyter to load these magics in all notebooks.

## Docker

A docker file has been provided to construct a docker image with jupyter and apricot configured. Use

`` docker pull grycap/apricot ``

to pull the image. Then, use

`` docker run --publish 8888:8888 -it grycap/apricot ``

to create and execute a container. The container will start automatically a jupyter server with apricot preconfigured. Then, use the url provided by jupyter to access to the server.

## Licensing

APRICOT is licensed under the Apache License, Version 2.0. See [LICENSE](https://github.com/grycap/apricot/blob/master/LICENSE) for the full license text.
