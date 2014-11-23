#!/bin/sh

stty -F /dev/ttyUSB0 38400 cs8
cat </dev/ttyUSB0
