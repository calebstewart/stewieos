#!/usr/bin/python

import os,sys
import Image

image = Image.open("poop.png")
pix = list(image.getdata());

line = ''

for y in range(0,24):
	for x in range(0,80):
		line = line + str(pix[x+y*80]) + ','
	print '\t' + line
	line = ''