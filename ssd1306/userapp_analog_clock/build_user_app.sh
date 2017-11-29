#!/bin/bash -e

export CROSS_COMPILE?=arm-linux-gnueabihf-

echo ${CROSS_COMPILE}


${CROSS_COMPILE}gcc analog_clock.c -o analog_clock -lm


echo "Done!"
