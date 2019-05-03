
#!/bin/bash

PWD=`pwd`

#Create plugin folder
pluginDir=`pip show jupyter_contrib_nbextensions | grep "Location:" | cut -d ':' -f 2`

echo "Plugin directory: $pluginDir"

rm -r $pluginDir &> /dev/null

if mkdir $pluginDir; then
    echo -e "Plugin folder created: $pluginDir"
else
    echo -e "Unable to create plugin folder: $pluginDir"
    exit 1
fi

#Copy plugin content
if cp -r apricot_plugin/* $pluginDir; then

    echo -e "Plugin installed."
    
else

    echo -e "Plugin installation failed!"
    echo -e "Unable to copy files from $PWD/plugin to $pluginDir"
    exit 2
fi

#Install apricot magics (default python)
if python -m pip install --user -e apricot_magic; then

    echo -e "magics succesfuly installed"
    
else
    echo -e "Unable to install apricop magics"
    exit 3
fi

#Install apricot magics (python3)
if python3 -m pip install --user -e apricot_magic; then

    echo -e "magics succesfuly installed"
    
else
    echo -e "Unable to install apricop magics with python3"
fi

#Install apricot magics (python2.7)
if python2.7 -m pip install --user -e apricot_magic; then

    echo -e "magics succesfuly installed"
    
else
    echo -e "Unable to install apricop magics with python2.7"
fi
