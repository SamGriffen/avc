#!/bin/bash
# Code for uploading AVC code and executing it, takes file to clone, and ip address as an argument

#pi@10.140.30.120

scp -o PreferredAuthentications=password -o PubkeyAuthentication=no -o StrictHostKeyChecking=no -r $1/* pi@$2:/home/pi/AVC/
ssh -x -o PreferredAuthentications=password -o PubkeyAuthentication=no -o StrictHostKeyChecking=no pi@$2 'g++ -o /home/pi/AVC/AVC /home/pi/AVC/AVC.cpp -le101 | cd /home/pi/AVC/ && ./AVC'
ssh -x -o PreferredAuthentications=password -o PubkeyAuthentication=no -o StrictHostKeyChecking=no pi@$2
