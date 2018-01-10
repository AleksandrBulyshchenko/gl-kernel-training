/*
 ============================================================================
 Name        : fonts.c
 Author      : dkirtoka
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdint.h>
#include <ft2build.h>
#include FT_FREETYPE_H

#define WIDTH    12
#define HEIGHT   12
#define NUM_CHAR 128

struct symbol
{
	uint8_t left;
	uint8_t top;
	uint8_t width;
	uint8_t height;
	uint8_t bitmap[ HEIGHT * WIDTH ];
};

void draw_bitmap( FT_Bitmap* bitmap, FT_Int x, FT_Int y, uint8_t *image )
{
	FT_Int i, j, p, q;
	FT_Int x_max = x + bitmap->width;
	FT_Int y_max = y + bitmap->rows;

	for (i = x, p = 0; i < x_max; i++, p++)
	{
		for (j = y, q = 0; j < y_max; j++, q++)
		{
			if (i < 0 || j < 0 ||
			        i >= WIDTH || j >= HEIGHT)
				continue;

			image[ j * WIDTH + i ] = bitmap->buffer[ q * bitmap->width + p ];

		}
	}
}

void show_image( unsigned char *image, int size )
{
  int  i, j;

  for ( i = 0; i < size; i++ )
    {
      for ( j = 0; j < size; j++ )
        putchar( image[i*size+j] < 128 ? ' ' : '*' );
      putchar( '\n' );
    }
}

int main( int argc, char** argv )
{
	FT_Library library;
	FT_Face face;

	FT_GlyphSlot slot;
	FT_Error error;

	char* filename;
	char* fontname;
	char text[256];

	int target_height;
	int n, num_chars;

	struct symbol sym;



	if (argc != 4)
	{
		fprintf( stderr, "usage: %s font sample-text\n", argv[ 0 ] );
		exit( 1 );
	}

	filename = argv[ 1 ]; /* first argument     */
	fontname = argv[ 2 ]; /* second argument    */
	;
	FILE *outf = fopen( fontname, "wb" );
	if (!outf)
	{
		fprintf( stderr, "cant't create file\n" );
		exit( 1 );
	}

	FILE *tf = fopen( argv[3], "r" );
	if (!tf)
	{
		fprintf( stderr, "cant't create file\n" );
		exit( 1 );
	}
	fread(text, 256, 1, tf);

	num_chars = strlen(text);
	target_height = HEIGHT;

	fwrite( &num_chars, 4, 1, outf );
	fwrite( &target_height, 4, 1, outf );

	error = FT_Init_FreeType( &library ); /* initialize library */
	/* error handling omitted */

	error = FT_New_Face( library, filename, 0, &face );/* create face object */
	/* error handling omitted */

	/* use 50pt at 100dpi */
	error = FT_Set_Char_Size( face, 0, HEIGHT * 64,
	        0, 0 ); /* set character size */
	/* error handling omitted */

	slot = face->glyph;

	for (n = 0; n < num_chars; n++)
	{
		error = FT_Load_Char( face, text[n], 0 );
		if (error)
			continue; /* ignore errors */

		error = FT_Render_Glyph( face->glyph,  FT_RENDER_MODE_NORMAL );
		/* now, draw to our target surface (convert position) */

		sym.left = slot->bitmap_left;
		sym.top = slot->bitmap_top;
		sym.width = slot->bitmap.width;
		sym.height = slot->bitmap.rows;

		memset( sym.bitmap, 0, sizeof(sym.bitmap));
		draw_bitmap( &slot->bitmap, slot->bitmap_left,
				target_height - slot->bitmap_top, sym.bitmap );
		show_image((void*) sym.bitmap, target_height);
		fprintf( stderr, "sym: '%c' left: %d top: %d, width: %d rows: %d\n", text[n], slot->bitmap_left,
		        slot->bitmap_top, slot->bitmap.width, slot->bitmap.rows );
		fwrite( &sym, sizeof(sym), 1, outf );
	}

	fclose( outf );
	FT_Done_Face( face );
	FT_Done_FreeType( library );

	return 0;
}
