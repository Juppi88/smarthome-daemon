#!/bin/bash
cd ~/smarthome-daemon
screen -S smarthome -d -m bash -c 'while true; do ./smarthome; sleep 2; done'
