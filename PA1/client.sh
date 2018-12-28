#!/bin/bash

#Run script for client distributed as part of 
#Assignment 1
#Computer Networks (CS 456)
#Number of parameters: 4
#Parameter:
#    $1: <server_address>
#    $2: <n_port>
#    $3: <req_code>
#    $4: message

#Uncomment exactly one of the following commands depending on your implementation

#For C/C++ implementation
#./client $1 $2 $3 "$4"

#For Java implementation
#java client $1 $2 $3 "$4"

#For Python implementation
python client.py $1 $2 $3 "$4"

#For Ruby implementation
#ruby client.rb $1 $2 $3 "$4"