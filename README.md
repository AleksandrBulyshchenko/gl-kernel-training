# Linux kernel general course
GlobalLogic Kharkiv, 2017
* Mail group: _globallogic-kernel-training-hrk-2017@googlegroups.com_

This repository is aimed for sharing our exercises and your solutions in scope of the course.
Mail group: _globallogic-kernel-training-hrk-2017@googlegroups.com_

Please refer to [wiki](https://github.com/Kernel-GL-HRK/gl-kernel-training-2017/wiki) for details.
This repository is aimed for sharing our exercises and your solutins in scope of the cource.

# gl-kernel-training
Linux kernel general course by GlobalLogic Kharkiv, 2017

# SandBox for Linux kernel drivers learning

## ssd1306
A custom Linux Kernel module for the SSD1306 based 128x64 pixel OLED display.
These displays are small, only about 1" diameter, but very readable due to the high contrast of an OLED display. This display is made of 128x64 individual white OLED pixels, each one is turned on or off by the controller chip. Because the display makes its own light, no backlight is required. 
SSD1306 communicates via I2C serial interface.
[DataSheet](https://www.olimex.com/Products/Modules/LCD/MOD-OLED-128x64/resources/SSD1306.pdf)

## userapp_analog_clock
A Linux userspace application. Draws a simple analog clock on fb0 FrameBuffer device.
[Low-Level Graphics on Linux](http://betteros.org/tut/graphics1.php)

![alt tag](https://www.mathworks.com/help/supportpkg/beagleboneio/ug/beaglebone_black_pinmap.png)

$KERNEL/arch/arm/boot/dts/
am335x-boneblack.dts
am335x-bone-common.dtsi

