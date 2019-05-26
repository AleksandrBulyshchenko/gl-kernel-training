#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <linux/fb.h>
#include <linux/kd.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <string.h>

#include <math.h>
#include <unistd.h>

#include "graph.h"

int main(int argc, char **argv)
{	
	//int x = 0, y = 0, i = 0;
	
	_Point center;
	int radius = 0;
	float angle = 0.0;
	float angle2 = 0.0;

	if (!fb_init("/dev/fb0"))
		exit(1);

	Graphic_ClearScreen();

	center.X	= lcd.width / 2;
	center.Y	= lcd.height / 2;
	radius		= 31;
	Graphic_drawCircle(center, radius);

	angle = 0.0;

	//for(angle = 0.0; angle <= 360; ){
	for(angle = 0.0; ; ){

		Graphic_ClearScreen();
		Graphic_drawCircle(center, radius);

		for(angle2 = 0.0; angle2 <= 360; ){
			Graphic_drawLine_(
				center.X + (radius -4) * cos((M_PI/180.0)*angle2 - M_PI/2),
				center.Y + (radius -4) * sin((M_PI/180.0)*angle2 - M_PI/2),

				center.X + radius * cos((M_PI/180.0)*angle2 - M_PI/2),
				center.Y + radius * sin((M_PI/180.0)*angle2 - M_PI/2));
				
				angle2 += 90.0;
		}

		Graphic_drawLine_(center.X, center.Y, 
				center.X + (radius -4) * cos((M_PI/180.0)*angle - M_PI/2),
				center.Y + (radius -4) * sin((M_PI/180.0)*angle - M_PI/2));
		angle += 6.0;
		Graphic_UpdateScreen();
		sleep(1);
	}

	Graphic_UpdateScreen();

	fb_cleanup();
	return(0);
}
