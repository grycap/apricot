# APRICOT: Advanced Platform for Reproducible Infrastructures in the Cloud via Open Tools

## Intrudoction 

APRICOT is an open-source extension to support infrastructure deployment, management and usage in Jupyter environment. It allows multi-cloud infrastructure deployment using a HTML graphic user interface that guides the user step by step through deployment process. Also, implements Ipython magic functionality to use and manage the deployed infrastructures within Jupyter notebooks.

##

## Components

![Alt text](docs/images/APRICOT_components.png?raw=true "Title")

APRICOT has been constructed using the following components:

- [**Jupyter**](https://jupyter.org/), an open-source web application that allows you to create and share documents that contain live code, equations, visualizations and narrative text. 
- [**CLUES**](https://github.com/grycap/clues), an elasticity manager that horizontally scales in and out the number of nodes of the Kubernetes cluster according to the workload.
- [**EC3**](https://servproject.i3m.upv.es/ec3/), an open-source tool to deploy compute clusters that can horizontally scale in terms of number of nodes with multiple plugins.
- [**IM**](https://www.grycap.upv.es/im/index.php), an open-source virtual infrastructure provisioning tool for multi-Clouds.
- [**ONEDATA**](https://github.com/grycap/clues), global data storage backed by computing centers and storage providers worldwide.

