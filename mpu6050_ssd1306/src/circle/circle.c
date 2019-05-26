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

#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>

#define MPU6050_DEV	"/dev/mpu6050_x"
/* ----------------------------------------------------------------------------
 * check key pressed
 * ------------------------------------------------------------------------- */
int kbhit()
{
	struct termios oldt, newt;
	int ch;
	int oldf;

	tcgetattr(STDIN_FILENO, &oldt);
	newt = oldt;
	newt.c_lflag &= ~(ICANON | ECHO);
	tcsetattr(STDIN_FILENO, TCSANOW, &newt);
	oldf = fcntl(STDIN_FILENO, F_GETFL, 0);
	fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);

	ch = getchar();

	tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
	fcntl(STDIN_FILENO, F_SETFL, oldf);
	if(ch != EOF)
	{
		ungetc(ch, stdin);
		return 1;
	}

	return 0;
}
/* ----------------------------------------------------------------------------
 * moveCircle 
 * ------------------------------------------------------------------------- */
void moveCircle(_Point *c, int r, int dx, int dy, lcd_type *l)
{
	int x = c->X + dx;
	int y = c->Y + dy;
	if (x < r) x = r;
	if (y < r) y = r;
	if (x + r > l->width)  x = l->width  - r;
	if (y + r > l->height) y = l->height - r;
	c->X = x;
	c->Y = y;
}
/* ----------------------------------------------------------------------------
 *  
 * ------------------------------------------------------------------------- */
int main(int argc, char **argv)
{	
	char buf_str[10224];
	_Point center;
	int radius = 0;
	int r = 0;
	int rgx, rgy, rgz, rax, ray, raz, rt, rgg, rga;
	double fgx, fgy, fgz;
	int dx, dy;

	if (!fb_init("/dev/fb0"))
		exit(1);

	int mpu6050;
	mpu6050 = open(MPU6050_DEV, O_RDONLY); 
	if (-1 == mpu6050) 
	{
		fb_cleanup();
		fprintf(stderr, MPU6050_DEV" Cant be openned...%d (%s)\n", errno, strerror(errno));
		
		exit(1);
	}

	Graphic_ClearScreen();

	center.X	= lcd.width / 2;
	center.Y	= lcd.height / 2;
	radius		= 10;
	Graphic_drawCircle(center, radius);

	for(; !kbhit(); ){

		r = read(mpu6050, buf_str, sizeof(buf_str) - 1);
		if (r > 0) {
			//fprintf(stdout, "Readed buf: %s\n", buf_str);
			sscanf(buf_str, "%d %d %d %d %d %d %d %d %d", &rgx, &rgy, &rgz, &rax, &ray, &raz, &rt, &rgg, &rga);
			//fprintf(stdout, "[%d %d %d %d %d %d %d %d %d]\n", gx, gy, gz, ax, ay, az, t, gg, ga);
			
			fgx = (double)rgx * 10.0 / rgg; 
			fgy = (double)rgy * 10.0 / rgg; 
			fgz = (double)rgz * 10.0 / rgg; 

			dx = (int) (fgx * lcd.width  / 360.0);
			dy = (int) (fgy * lcd.height / 360.0);

			fprintf(stdout, "[%5.1f %5.1f %5.1f] [%5d %5d]\n", fgx, fgy, fgz, dx, dy);
			
			moveCircle(&center, radius, dx, dy, &lcd);
			Graphic_ClearScreen();
			Graphic_drawCircle(center, radius);

			Graphic_UpdateScreen();
		}
		usleep(500000);
	}

	Graphic_UpdateScreen();

	close(mpu6050);
	fb_cleanup();
	return(0);
}
