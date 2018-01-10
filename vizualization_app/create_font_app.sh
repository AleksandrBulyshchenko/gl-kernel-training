#!/bin/bash

gcc crfont.c --I/usr/include/freetype2 -lfreetype -lm -o fontcreator
