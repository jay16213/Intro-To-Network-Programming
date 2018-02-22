#!/bin/sh

clear
rm -f server client
gcc server.c -o server
gcc client.c -o client
