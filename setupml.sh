#!/bin/sh

apt update -y
apt install wget clang -y
wget https://raw.githubusercontent.com/eikarna/eikarna/main/mludp.c
clang -o mludp mludp.c
wget https://raw.githubusercontent.com/eikarna/eikarna/main/mludp2.c
clang -o mludp2 mludp2.c
