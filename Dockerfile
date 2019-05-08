#Download base image ubuntu 16.04
FROM ubuntu:16.04

# Set root user
USER root

# Create a user for jupyter server
RUN useradd -ms /bin/bash jupyserver

# Update Ubuntu Software repository
RUN apt-get update

RUN apt-get install -y nano
RUN apt-get install -y curl
RUN apt-get install -y sshpass

# Install python and jupyter
RUN apt-get install -y python3 python2.7

RUN apt-get install -y python3-pip
RUN apt-get install -y python-pip
RUN pip3 install --upgrade pip
RUN pip2 install --upgrade pip
RUN pip3 install jupyter
RUN pip2 install ec3-cli

# Install git
RUN apt-get install -y git

# Create the script to init jupyter server
RUN echo "#!/bin/bash" > /bin/jupyter-apricot
RUN echo "jupyter notebook --ip 0.0.0.0 --no-browser" >> /bin/jupyter-apricot

# Add execution permissions
RUN chmod +x /bin/jupyter-apricot

# Change to jupyter server user
USER jupyserver
WORKDIR /home/jupyserver

# Clone git
RUN git clone https://github.com/grycap/apricot.git

# Install apricot
WORKDIR /home/jupyserver/apricot
RUN sh install.sh
WORKDIR /home/jupyserver
RUN cp -r apricot/examples .

# Remove download files
RUN rm -r apricot

# Set entry point
ENTRYPOINT ["/bin/jupyter-apricot"]
