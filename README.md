# APRICOT: Advanced Platform for Reproducible Infrastructures in the Cloud via Open Tools

## Intrudoction 

APRICOT is an open-source extension to support infrastructure deployment, management and usage in Jupyter environment. It allows multi-cloud infrastructure deployment using a HTML graphic user interface that guides the user step by step through deployment process. Also, implements Ipython magic functionality to use and manage the deployed infrastructures within Jupyter notebooks.

## Experiment replication methodology

A generic computational experimentation involves the elements showed at next figure. So, to be able to reproduce any experiment, any researcher needs,

 - Required input data or a method to produce it.

 - Specific hardware needs, like memory requirements, GPGPUs, FPGAs, multi-node cluster, etc.

 - A list of all required software and their configuration, such as MPI on a cluster, specific language compilers, specific software source codes, libraries, etc.

 - Clear instructions on how to reproduce the entire experiment.

![Alt text](docs/images/experiment.png?raw=true "Experimentation")

APRICOT, using other components, satisfy all the requirements for develop reproducible experiments within a unique environment, Jupyter notebooks. The key points for develop reproducible experiments using APRICOT extensions are,


 - Required data must be provided using external storage systems or a notebook with instructions to create it. APRICOT has been configured to use OneData as external storage provider.

 - APRICOT provides a set of predefined configurable infrastructures to fit the experiments. Any researcher can deploy easily the same infrastructure than the used in a experimentation with APRICOT deployed infrastructure.

 - APRICOT allow executing instructions at deployed infrastructures. So, extra needed software can be documented and installed at infrastructure within the same Jupyter notebook where the experimentation has been documented and executed step by step. 

 - Since APRICOT extension uses Jupyter notebooks as base environment, all experiment can be documented using text, life code and images.

## Components

![Alt text](docs/images/APRICOT_components.png?raw=true "Components")

APRICOT has been constructed using the following components:

- [**Jupyter**](https://jupyter.org/), an open-source web application that allows you to create and share documents that contain live code, equations, visualizations and narrative text. 
- [**CLUES**](https://github.com/grycap/clues), an elasticity manager that horizontally scales in and out the number of nodes of the Kubernetes cluster according to the workload.
- [**EC3**](https://servproject.i3m.upv.es/ec3/), an open-source tool to deploy compute clusters that can horizontally scale in terms of number of nodes with multiple plugins.
- [**IM**](https://www.grycap.upv.es/im/index.php), an open-source virtual infrastructure provisioning tool for multi-Clouds.
- [**ONEDATA**](https://github.com/grycap/clues), global data storage backed by computing centers and storage providers worldwide.

## Installation

### Requisites

APRICOT needs EC3 client to deploy and infrastructure and get credentials. The installation details can be found at [EC3 documentation](https://ec3.readthedocs.io/en/devel/intro.html#installation).

Also, APRICOT requires a [Jupyter installation](https://jupyter.org/install), since uses its environment to run. Should be compatible with Jupyter 4.x and 5.x versions.

### Installing

To install APRICOT package, run the provided install script

`` bash install.sh``

## Deployment

Infrastructure deployment with APRICOT is done using a notebook extension. This one, creates the button showed at next image, which opens a GUI to guide the user step by step trough deployment process.


![Alt text](docs/images/pluginDeploy.png?raw=true "Deploy plugin")


Deployment process include,

- Select infrastructure topology between a set of predefined infrastructures. At the moment, predefined topologies are Batch-Cluster, MPI-Cluster and Advanced which allows a custom configuration.
- Select a cloud provider where deploy the infrastructure. Actual supported providers are OpenNebula and AWS.
- Depending on selected provider, introduce necessary access credentials.
- Specify frontend and workers specifications such as memory, CPUs, OS image etc.
- Set infrastructure features like maximum number of workers or cluster identifier name.
- Deploy the infrastructure.

If any error occurs during deployment, this will shown at web console. Also, is possible to get cluster configuration logs using implemented magics functions.

## Infrastructure management

To manage and use previous deployed infrastructures within Jupyter notebook environment, a set of Ipython magic functions has been implemented. This functions are listed below,


