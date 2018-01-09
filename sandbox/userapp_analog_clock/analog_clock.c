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

typedef unsigned char 	u8;
typedef unsigned short 	u16;

#define SSD1306_WIDTH	128
#define SSD1306_HEIGHT	64



typedef struct{
	u16 	X;
	u16 	Y;
}_Point;

typedef enum {
		SSD1306_COLOR_BLACK = 0x00,   /*!< Black color, no pixel */
		SSD1306_COLOR_WHITE = 0x01    /*!< Pixel is set. Color depends on LCD */
} ssd1306_COLOR_t;


typedef struct {
	struct fb_var_screeninfo vinfo;
	struct fb_fix_screeninfo finfo;
	int width;
	int height;
	long int screensize;
	char *fbp;

}lcd_type;

static lcd_type lcd;


static u8 ssd1306_Buffer[SSD1306_WIDTH * SSD1306_HEIGHT / 8];



static int ssd1306_DrawPixel(u16 x, u16 y, ssd1306_COLOR_t color) {

	if ( x > SSD1306_WIDTH || y > SSD1306_HEIGHT ) {
		return -1;
	}

	/* Set color */
	if (color == SSD1306_COLOR_WHITE) {
		ssd1306_Buffer[x/8 + (y * (SSD1306_WIDTH / 8))] |= (1 << (x % 8));
	}
	else {
		ssd1306_Buffer[x/8 + (y * (SSD1306_WIDTH / 8))] &= ~(1 << (x % 8));
	}

	return 0;
}



static void Graphic_setPoint(const u16 X, const u16 Y, ssd1306_COLOR_t color)
{
	ssd1306_DrawPixel(X, Y, color);
}



static void Graphic_drawLine(_Point p1, _Point p2, ssd1306_COLOR_t color){
	int dx, dy, inx, iny, e;
	u16 x1 = p1.X, x2 = p2.X;
	u16 y1 = p1.Y, y2 = p2.Y;

	dx = x2 - x1;
	dy = y2 - y1;
	inx = dx > 0 ? 1 : -1;
	iny = dy > 0 ? 1 : -1;

	dx = (dx > 0) ? dx : -dx;
	dy = (dy > 0) ? dy : -dy;


	if(dx >= dy) {
		dy <<= 1;
		e = dy - dx;
		dx <<= 1;
		while (x1 != x2){
			Graphic_setPoint(x1, y1, color);
			if(e >= 0){
				y1 += iny;
				e-= dx;
			}
			e += dy;
			x1 += inx;
		}
	}
	else{
		dx <<= 1;
		e = dx - dy;
		dy <<= 1;
		while (y1 != y2){
			Graphic_setPoint(x1, y1, color);
			if(e >= 0){
				x1 += inx;
				e -= dy;
			}
			e += dx;
			y1 += iny;
		}	}
	Graphic_setPoint(x1, y1, color);
}
// ---------------------------------------------------------------------------


static void Graphic_drawLine_(u16 x1, u16 y1, u16 x2, u16 y2,
	ssd1306_COLOR_t color)
{
	_Point p1 = {0}, p2 = {0};
	p1.X = x1;
	p1.Y = y1;
	p2.X = x2;
	p2.Y = y2;

	Graphic_drawLine(p1, p2, color);
}
// ---------------------------------------------------------------------------



void Graphic_drawCircle(_Point center, u16 r){
	int  draw_x0, draw_y0;			//draw points
	int  draw_x1, draw_y1;
	int  draw_x2, draw_y5;
	int  draw_x3, draw_y3;
	int  draw_x4, draw_y4;
	int  draw_x6, draw_y6;
	int  draw_x7, draw_y7;
	int  draw_x5, draw_y2;
	int  xx, yy;					// circle control variable
	int  di;						// decide variable
	
	if( 0 == r)
		return;

	// calculate 8 special point(0��45��90��135��180��225��270degree) display them 
	draw_x0 = draw_x1 = center.X;
	draw_y0 = draw_y1 = center.Y + r;
	if(draw_y0<63)
		Graphic_setPoint(draw_x0, draw_y0, SSD1306_COLOR_WHITE); // 90degree
	draw_x2 = draw_x3 = center.X;
	draw_y2 = draw_y3 = center.Y - r;
	if(draw_y2>=0)
		Graphic_setPoint(draw_x2,draw_y2, SSD1306_COLOR_WHITE);// 270degree
	draw_x4 = draw_x6 = center.X + r;
	draw_y4 = draw_y6 = center.Y;
	if(draw_x4<127)
		Graphic_setPoint(draw_x4, draw_y4, SSD1306_COLOR_WHITE);// 0degree
	draw_x5 = draw_x7 = center.X - r;
	draw_y5 = draw_y7 = center.Y;
	if(draw_x5>=0)
		Graphic_setPoint(draw_x5, draw_y5, SSD1306_COLOR_WHITE); // 180degree
	if(1==r)   // if the radius is 1, finished.
		return;
	//using Bresenham 
	di = 3 - 2*r;
	xx = 0;
	yy = r;
	while(xx<yy){
		if(di<0){
			di += 4*xx + 6;
		}else{
			di += 4*(xx - yy) + 10;
			yy--;
			draw_y0--;
			draw_y1--;
			draw_y2++;
			draw_y3++;
			draw_x4--;
			draw_x5++;
			draw_x6--;
			draw_x7++;
		}
		xx++;
		draw_x0++;
		draw_x1--;
		draw_x2++;
		draw_x3--;
		draw_y4++;
		draw_y5++;
		draw_y6--;
		draw_y7--;
		//judge current point in the avaible range
		if( (draw_x0<=127)&&(draw_y0>=0) ){
			Graphic_setPoint(draw_x0, draw_y0, SSD1306_COLOR_WHITE);
		}	
		if( (draw_x1>=0)&&(draw_y1>=0) ){
			Graphic_setPoint(draw_x1, draw_y1, SSD1306_COLOR_WHITE);
		}
		if( (draw_x2<=127)&&(draw_y2<=63) ){
			Graphic_setPoint(draw_x2, draw_y2, SSD1306_COLOR_WHITE);
		}
		if( (draw_x3>=0)&&(draw_y3<=63) ){ 
			Graphic_setPoint(draw_x3, draw_y3, SSD1306_COLOR_WHITE);
		}
		if( (draw_x4<=127)&&(draw_y4>=0) ){
			Graphic_setPoint(draw_x4, draw_y4, SSD1306_COLOR_WHITE);
		}
		if( (draw_x5>=0)&&(draw_y5>=0) ){
			Graphic_setPoint(draw_x5, draw_y5, SSD1306_COLOR_WHITE);
		}
		if( (draw_x6<=127)&&(draw_y6<=63) ){
			Graphic_setPoint(draw_x6, draw_y6, SSD1306_COLOR_WHITE);
		}
		if( (draw_x7>=0)&&(draw_y7<=63) ){ 
			Graphic_setPoint(draw_x7, draw_y7, SSD1306_COLOR_WHITE);
		}
	}
}


static void Graphic_ClearScreen(void){
	memset(ssd1306_Buffer, 0, sizeof(ssd1306_Buffer));
}


int fb;

static void Graphic_UpdateScreen(void){

	int i = 0;

	for (i=0; i < lcd.screensize; i++) {

		*(lcd.fbp+i) = ssd1306_Buffer[i];
	}
}



struct fb_var_screeninfo fb_var;
struct fb_var_screeninfo fb_screen;
struct fb_fix_screeninfo fb_fix;
unsigned char *fb_mem = NULL;
unsigned char *fb_screenMem = NULL;



#ifndef PAGE_SHIFT
	#define PAGE_SHIFT 12
#endif
#ifndef PAGE_SIZE
	#define PAGE_SIZE (1UL << PAGE_SHIFT)
#endif
#ifndef PAGE_MASK
	#define PAGE_MASK (~(PAGE_SIZE - 1))
#endif

int fb_init(char *device) {

	int fb_mem_offset;

	// get current settings (which we have to restore)
	if (-1 == (fb = open(device, O_RDWR))) {
		fprintf(stderr, "Open %s error\n", device);
		return 0;
	}
	if (-1 == ioctl(fb, FBIOGET_VSCREENINFO, &fb_var)) {
		fprintf(stderr, "Ioctl FBIOGET_VSCREENINFO error.\n");
		return 0;
	}
	if (-1 == ioctl(fb, FBIOGET_FSCREENINFO, &fb_fix)) {
		fprintf(stderr, "Ioctl FBIOGET_FSCREENINFO error.\n");
		return 0;
	}
	if (fb_fix.type != FB_TYPE_PACKED_PIXELS) {
		fprintf(stderr, "Can handle only packed pixel frame buffers.\n");
		goto err;
	}

	fb_mem_offset = (unsigned long)(fb_fix.smem_start) & (~PAGE_MASK);
	fb_mem = mmap(NULL, fb_fix.smem_len + fb_mem_offset, PROT_READ | PROT_WRITE, MAP_SHARED, fb, 0);
	if (-1L == (long)fb_mem) {
		fprintf(stderr, "Mmap error.\n");
		goto err;
	}
	// move viewport to upper left corner
	if (fb_var.xoffset != 0 || fb_var.yoffset != 0) {
		fb_var.xoffset = 0;
		fb_var.yoffset = 0;
		if (-1 == ioctl(fb, FBIOPAN_DISPLAY, &fb_var)) {
			fprintf(stderr, "Ioctl FBIOPAN_DISPLAY error.\n");
			munmap(fb_mem, fb_fix.smem_len);
			goto err;
		}
	}
 

	lcd.width 		= fb_var.xres;
	lcd.height 		= fb_var.yres;
	lcd.screensize 	= fb_var.xres * fb_var.yres * fb_var.bits_per_pixel / 8;
	lcd.fbp 		= fb_mem;

	printf("lcd.width %d\n", lcd.width);
	printf("lcd.height %d\n", lcd.height);
	printf("lcd.screensize %ld\n", lcd.screensize);
	printf("lcd.fbp %p\n", lcd.fbp);

	return 1;

err:
	if (-1 == ioctl(fb, FBIOPUT_VSCREENINFO, &fb_var))
		fprintf(stderr, "Ioctl FBIOPUT_VSCREENINFO error.\n");
	if (-1 == ioctl(fb, FBIOGET_FSCREENINFO, &fb_fix))
		fprintf(stderr, "Ioctl FBIOGET_FSCREENINFO.\n");
	return 0;
}

void fb_cleanup(void) {

	if (-1 == ioctl(fb, FBIOPUT_VSCREENINFO, &fb_var))
		fprintf(stderr, "Ioctl FBIOPUT_VSCREENINFO error.\n");
	if (-1 == ioctl(fb, FBIOGET_FSCREENINFO, &fb_fix))
		fprintf(stderr, "Ioctl FBIOGET_FSCREENINFO.\n");
	munmap(fb_mem, fb_fix.smem_len);
	close(fb);
}


void draw_hatches(_Point center, int radius)
{
	float angle2 = 0.0;
	for(angle2 = 0.0; angle2 <= 360; ){
		Graphic_drawLine_(
			center.X + (radius -4) * cos((M_PI/180.0)*angle2 - M_PI/2),
			center.Y + (radius -4) * sin((M_PI/180.0)*angle2 - M_PI/2),

			center.X + radius * cos((M_PI/180.0)*angle2 - M_PI/2),
			center.Y + radius * sin((M_PI/180.0)*angle2 - M_PI/2),
			SSD1306_COLOR_WHITE);
			
			angle2 += 90.0;
	}
	printf("%s\n", __func__);
}

int main(int argc, char **argv)
{	
	int x = 0, y = 0, i = 0;
	u16 x2_prev = 0, y2_prev = 0;
	float angle_prev = 0.0;
	long int location = 0;
	
	_Point center;
	int radius = 0;
	float angle = 0.0;

	if (!fb_init("/dev/fb0"))
		exit(1);

	Graphic_ClearScreen();

	center.X 	= lcd.width / 2;
	center.Y 	= lcd.height / 2;
	radius 		= 31;
	Graphic_drawCircle(center, radius);

	//for(angle = 0.0; angle <= 360; ){
	for(angle = 0.0; ; ){
		Graphic_drawLine_(center.X, center.Y, 
				x2_prev, y2_prev, SSD1306_COLOR_BLACK);

		if (fmod(angle_prev, 90.0) == 0)
			draw_hatches(center, radius);

		x2_prev = center.X + (radius -4) * cos((M_PI/180.0)*angle - M_PI/2);
		y2_prev = center.Y + (radius -4) * sin((M_PI/180.0)*angle - M_PI/2);
		angle_prev = angle;

		Graphic_drawLine_(center.X, center.Y, 
				x2_prev, y2_prev, SSD1306_COLOR_WHITE);
		angle += 6.0;
		Graphic_UpdateScreen();
		sleep(1);
	}

	Graphic_UpdateScreen();
		  

	fb_cleanup();
	return(0);
}