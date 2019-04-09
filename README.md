# APRICOT: Advanced Platform for Reproducible Infrastructures in the Cloud via Open Tools

## Intrudoction 

APRICOT is an open-source extension to support infrastructure deployment, management and usage in Jupyter environment. It allows multi-cloud infrastructure deployment using a HTML graphic user interface that guides the user step by step through deployment process. Also, implements Ipython magic functionality to use and manage the deployed infrastructures within Jupyter notebooks.

## Experiment replication methodology

A generic computational experimentation involves the elements showed at figure \ref{fig:computexp}. So, to be able to reproduce any experiment, any researcher needs,

 - Required input data or a method to produce it.

 - Specific hardware needs, like memory requirements, GPGPUs, FPGAs, multi-node cluster, etc.

 - A list of all required software and their configuration, such as MPI on a cluster, specific language compilers, specific software source codes, libraries, etc.

 - Clear instructions on how to reproduce the entire experiment.

![Alt text](docs/images/experiment.png?raw=true "Title")

Although in some experiments a simple computer fits the experiment requirements, lately more and more experiments needs a significant computer effort that requires a complex infrastructure. This infrastructure is usually obtained via private and public cloud providers, however, deploy and configuration steps can involve a significant effort by the researcher and advanced technical knowledge. To simplify this step in reproducible research field enter APRICOT. As we will see at next sections, APRICOT, using other components, satisfy all the requirements for develop reproducible experiments within a unique environment, Jupyter notebooks. The key points for develop reproducible experiments using APRICOT extensions are,


 - Required data must be provided using external storage systems or a notebook with instructions to create it. APRICOT has been configured to use OneData as external storage provider.

 - APRICOT provides a set of predefined configurable infrastructures to fit the experiments. Any researcher can deploy easily the same infrastructure than the used in a experimentation with APRICOT deployed infrastructure.

 - APRICOT allow executing instructions at deployed infrastructures. So, extra needed software can be documented and installed at infrastructure within the same Jupyter notebook where the experimentation has been documented and executed step by step. 

 - Since APRICOT extension uses Jupyter notebooks as base environment, all experiment can be documented using text, life code and images.

## Components

![Alt text](docs/images/APRICOT_components.png?raw=true "Title")

APRICOT has been constructed using the following components:

- [**Jupyter**](https://jupyter.org/), an open-source web application that allows you to create and share documents that contain live code, equations, visualizations and narrative text. 
- [**CLUES**](https://github.com/grycap/clues), an elasticity manager that horizontally scales in and out the number of nodes of the Kubernetes cluster according to the workload.
- [**EC3**](https://servproject.i3m.upv.es/ec3/), an open-source tool to deploy compute clusters that can horizontally scale in terms of number of nodes with multiple plugins.
- [**IM**](https://www.grycap.upv.es/im/index.php), an open-source virtual infrastructure provisioning tool for multi-Clouds.
- [**ONEDATA**](https://github.com/grycap/clues), global data storage backed by computing centers and storage providers worldwide.
