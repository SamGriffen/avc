# avc
Repo for AVC

##Notes on the tranfer.sh file
This file is made to transfer the code to the raspberry pi, I am in the process of making it run the script, but for now it should be compiling it at the very least. To use this script simply run this command in the root directory of the project (Which is the directory this readme lives in):

`./tranfer.sh /path/to/folder/you/want/to/copy ip.of.raspberry.pi`

The script will copy every file in the specified path, and copy them to the AVC folder on the raspberry pi. It will then compile AVC.cpp to AVC

Note: The current IP is 10.140.30.120
