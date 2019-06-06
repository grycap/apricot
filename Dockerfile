#Download base image ubuntu 18.04
FROM ubuntu:18.04

# Set root user
USER root

# Update Ubuntu Software repository and install python, jupyter and git
RUN apt-get update && apt-get install -y nano && apt-get install -y curl && apt-get install -y sshpass && \
    apt-get install -y python3 python2.7 && apt-get install -y python3-pip && apt-get install -y python-pip && \
    python3 -m pip install --upgrade pip && python2 -m pip install --upgrade pip && python3 -m pip install jupyter &&\
    python2 -m pip install ec3-cli && apt-get install -y git && python3 -m pip install numpy scipy matplotlib

# Create the script to init jupyter server
RUN echo "#!/bin/bash" > /bin/jupyter-apricot && \
    echo "jupyter notebook --ip 0.0.0.0 --no-browser" >> /bin/jupyter-apricot && \
    chmod +x /bin/jupyter-apricot

# Create a user for jupyter server
RUN useradd -ms /bin/bash jupyserver

# Change to jupyter server user
USER jupyserver
WORKDIR /home/jupyserver

# Clone git, install, get the examples and clear files
RUN git clone https://github.com/grycap/apricot.git && cd /home/jupyserver/apricot \
    && sh install.sh && cd /home/jupyserver && cp -r apricot/examples . && mv apricot .apricot_git

# Set entry point
ENTRYPOINT ["/bin/jupyter-apricot"]
