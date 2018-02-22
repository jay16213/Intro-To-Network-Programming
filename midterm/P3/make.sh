#!/bin/sh

rm -f server client
clear
gcc server.c -o server
gcc client.c -o client
