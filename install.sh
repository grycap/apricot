
#!/bin/bash

#Install and enable plugin content

if jupyter nbextension install apricot_plugin --user; then
    echo -e "Plugin installed."
    
else

    echo -e "Plugin installation failed!"
    exit 2
fi

if jupyter nbextension enable apricot_plugin/main --user; then
    echo -e "Plugin enabled."
    
else

    echo -e "Fail enabling plugin!"
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
