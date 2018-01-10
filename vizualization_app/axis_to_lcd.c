/*
 * axis_to_lcd.c
 *
 *  Created on: 18 дек. 2017 г.
 *      Author: dkirtoka
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>

#define AXISDEVNAME argv[1] //"/dev/mpu6050_0"
#define FBDEV       argv[2] //"/dev/fb1"
#define FONTNAME    argv[3] //"9x9.fnt"

#define LCD_WIDTH  128
#define LCD_HEIGHT  64

#define TEXT_MAX 16
enum {
	X,
	Y,
	Z,
	AXES_NUM
};

#define HEAD_AXIS "AXIS"
#define HEAD_ACCEL "ACCEL"
#define HEAD_TEMP "TEMP:"
struct mpu6050_data {
	int axis[AXES_NUM];
	int temerature;
	int accel[AXES_NUM];
};

struct image {
	int width;
	int height;
	uint8_t *buffer;
};


struct symbol {
	uint8_t left;
	uint8_t top;
	uint8_t width;
	uint8_t rows;
	uint8_t buffer[0];
};

struct font {
	int interval;
	int num_chars;
	int target_height;
	struct symbol symbol[0];
};

static struct font *font;

void draw_bitmap( struct symbol *bitmap, int x, int y, int sym_height, struct image *image )
{
	int i, j, p, q, x_max, y_max;

	x += bitmap->left;
	y += sym_height - bitmap->top;

	x_max = x + bitmap->width;
	y_max = y + bitmap->rows;

	for (i = x, p = 0; i < x_max; ++i, ++p)
	{
		for (j = y, q = 0; j < y_max; ++j, ++q)
		{
			if (i < 0 || j < 0 ||
			        i >= image->width || j >= image->height)
				continue;

			char c = bitmap->buffer[q * sym_height + p];
			image->buffer[j * image->width + i] = c;
		}
	}
}

static int get_symbol_offset(char c)
{
	return (c < '!' || c > 127) ? 0 : c - '!';
}

static struct symbol *get_symbol_addr(char c)
{
	int buf_size = sizeof(struct symbol) + font->target_height * font->target_height;

	return (void*) ((uint8_t *) font->symbol + get_symbol_offset(c) * buf_size);
}

static int get_text_width(char *text)
{
	int i, result;
	for (i = 0, result = 0; i < strlen(text); ++i) {
		struct symbol *sym = get_symbol_addr(text[i]);
		result += sym->width + font->interval;
	}
	return result;
}

static void draw_text(int x, int y, char *text, struct image *img)
{
	int i, shift_x;

	printf("%s: x: %d, y: %d, text: %s, img: %p\n", __FUNCTION__, x, y, text, img);

	for (i = 0, shift_x = 0; i < strlen(text); ++i) {
		struct symbol *sym = get_symbol_addr(text[i]);
		draw_bitmap(sym, x + shift_x, y, font->target_height, img);
		shift_x += sym->width + font->interval;
	}
}

static void show_image( struct image *image )
{
  int  i, j, w, h;

  w = image->width; h = image->height;
  for ( i = 0; i < h; i++ )
    {
      for ( j = 0; j < w; j++ )
        putchar( image->buffer[i*w+j] < 32 ? ' ' : '*' );
      putchar( '\n' );
    }
}

static int load_font(struct font *font,  int fd)
{
	int i, num_chars, buf_size;
	struct image img;
	char buf[16*16];
	struct symbol *sym;
	int target_height;

	img.width = img.height = 16;
	img.buffer = buf;

	fprintf(stderr, "%s(%p, %d)\n", __FUNCTION__, font, fd);

	font->interval = 3;
	read(fd, &font->num_chars, 4);
	read(fd, &font->target_height, 4);

	num_chars = font->num_chars;
	target_height = font->target_height;
	buf_size = sizeof(struct symbol) + font->target_height * font->target_height;

	fprintf(stderr, "%s num_chars: %d buf_size: %d\n", __FUNCTION__, num_chars, buf_size);

	for (i = 0; i < num_chars; ++i) {
		uint8_t * buf_pos = (uint8_t *) font->symbol + i * buf_size;
		if (read(fd, buf_pos, buf_size) != buf_size) {
			fprintf(stderr, "error read %d cunk\n", i);
			return 1;
		}
		sym = (void*) buf_pos;
		memset(img.buffer, 0, sizeof(buf));
		draw_bitmap(sym, 0, 0, target_height, &img);
		show_image(&img);
	}
	return 0;
}

static int trim_right(int start, int width, char *text)
{
	return width - start - get_text_width(text);
}

int main(int argc, char **argv)
{
	uint8_t buffer[LCD_HEIGHT * LCD_WIDTH];
	struct image img = {
			.height = LCD_HEIGHT,
			.width  = LCD_WIDTH,
			.buffer = buffer
	};
	char text[TEXT_MAX];
	struct mpu6050_data idata;
	int ifd, ofd, fontfd, buf_size;
	int row_step;

	if (argc != 4) {
		fprintf( stderr, "usage: %s <in> <out> <font>\n", argv[0] );
		exit( 1 );
	}

	ifd = open(AXISDEVNAME, O_RDONLY);
	if (ifd < 0) {
		printf("can' open %s\n", AXISDEVNAME);
		return 1;
	}

	ofd = open(FBDEV, O_WRONLY);
	if (ifd < 0) {
		printf("can' open %s\n", FBDEV);
		return 1;
	}

	fontfd = open(FONTNAME, O_RDONLY);
	if (fontfd < 0) {
		printf("can' open %s\n", FONTNAME);
		return 1;
	}

	buf_size = 0x8000;//getfsize
	font = calloc(1, buf_size);
	if (!font) {
		printf("can't alloc %d\n", buf_size);
		return 1;
	}
	if (load_font(font, fontfd)) {
		printf("can't load %s\n", FONTNAME);
		return 1;
	}
	close(fontfd);

	for(;;) {
		int i=0;
		int err = read(ifd, &idata, sizeof (idata));

		if (err != sizeof (idata)) {
			printf("%s read error %d\n", AXISDEVNAME, err);
			return 1;
		}

		memset(img.buffer, 0, LCD_HEIGHT * LCD_WIDTH);
		row_step = font->target_height + 1;

		//head
		draw_text(trim_right(img.width / 2, img.width, HEAD_AXIS),
				0, HEAD_AXIS, &img);
		draw_text(trim_right(0, img.width, HEAD_ACCEL),
				0, HEAD_ACCEL, &img);
		// X Y Z
		for (i = 0; i < AXES_NUM; ++i) {
			snprintf(text, TEXT_MAX - 1, "%c", 'X' + i);
			draw_text(0, row_step * (i + 1), text, &img);
		}
		// axes
		for (i = 0; i < AXES_NUM; ++i) {
			snprintf(text, TEXT_MAX - 1, "%d", idata.axis[i]);
			draw_text(
					trim_right(img.width / 2, img.width, text),
					row_step * (i + 1),
					text, &img);
		}
		// accel
		for (i = 0; i < AXES_NUM; ++i) {
			snprintf(text, TEXT_MAX - 1, "%d", idata.accel[i]);
			draw_text(
					trim_right(0, img.width, text),
					row_step * (i + 1),
					text, &img);
		}
		// TEMP:temperature
		snprintf(text, TEXT_MAX - 1, HEAD_TEMP "%d", idata.temerature);
		draw_text(
				(img.width - get_text_width(text)) / 2,
				row_step * 4,
				text, &img);

		lseek(ofd, 0, SEEK_SET);
		if (write(ofd, img.buffer, img.height * img.width) != img.height * img.width)
		{
			printf("%s write error %d\n", FBDEV, err);
		}
	}
	return 0;
}
