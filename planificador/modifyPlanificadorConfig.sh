#!/bin/bash  

IP=$1
PORT=$2

CONFIGS=$(find . -name '*.cfg' )

for CONFIG in $CONFIGS
do
	chmod 777 $CONFIG
	sed -i "5s/.*/IP_COORDINADOR=${IP}/" $CONFIG
	sed -i "6s/.*/PORT_COORDINADOR=${PORT}/" $CONFIG
done