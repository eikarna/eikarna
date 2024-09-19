#!/bin/sh

sudo apt update -y &> /dev/null
sudo apt install wget clang -y &> /dev/null
wget https://raw.githubusercontent.com/eikarna/eikarna/main/mludp.c &> /dev/null
clang -Ofast -o mludp mludp.c
wget https://raw.githubusercontent.com/eikarna/eikarna/main/mludp2.c &> /dev/null
clang -Ofast -o mludp2 mludp2.c

# Set ulimit globally
for opt in $(ulimit -a | sed 's/.*\-\([a-z]\)[^a-zA-Z].*$/\1/'); do
     ulimit -$opt ulimited
 done
