#!/bin/sh

apt update -y &> /dev/null
apt install wget clang -y &> /dev/null
wget https://raw.githubusercontent.com/eikarna/eikarna/main/mludp.c &> /dev/null
clang -Ofast -o mludp mludp.c
wget https://raw.githubusercontent.com/eikarna/eikarna/main/mludp2.c &> /dev/null
clang -Ofast -o mludp2 mludp2.c
