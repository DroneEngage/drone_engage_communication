[![Ardupilot Cloud EcoSystem](https://cloud.ardupilot.org/_static/ardupilot_logo.png "Ardupilot Cloud EcoSystem")](https://cloud.ardupilot.org "Ardupilot Cloud EcoSystem") **Drone Engage** is part of Ardupilot Cloud Eco System

------------

![Drone Engage Communicator Module](https://raw.githubusercontent.com/DroneEngage/droneengage_communication/master/resources/de_logo_title.png) 

#Drone Engage Communicator Module

# What is Drone-Engage ?

Drone-Engage (D.E.) is a distibuted system that allows monitoring & controlling drones via Internet. Drone-Engage objective is to provide a Linux-based alternative for [Andruav](https://play.google.com/store/apps/details?id=arudpilot.andruav&hl=en&gl=US "Andruav") on Android mobiles.

# Communication Module

This repository is for **Drone-Engage Communication Module**. This module runs on the vehicle and communicates with other modules such as [Mavlink_Module "de_mavlink"](https://github.com/HefnySco/de_mavlink "Mavlink_Module") and Video_Module. On the other side it communicates with Andruav Server. This module receives messages from all plugins and forward them to [WebClients](https://github.com/HefnySco/andruav_webclient "WebClients") and other vehicles, and receives commands from Webclients via Internet Server and process them or forward them to attached modules to process them.

For more Information Please Goto [Cloud.Ardupilot.org](https://cloud.ardupilot.org/ "Cloud.Ardupilot.org")




# Build the Code
**Prerequisits**
`sudo apt-get install libcurl4-openssl-dev`
`sudo apt install libssl-dev`

**Building**
mkdir build 
cd build
make


