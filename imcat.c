// imcat.c
//
// by Abraham Stolk.
// This software is in the Public Domain.

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>

#if defined(_WIN64)
#	define STBI_NO_SIMD
#endif
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

static int termw=0, termh=0;

#if defined(_WIN64)
#	include <windows.h>
static void get_terminal_size(void)
{
	const HANDLE hStdout = GetStdHandle( STD_OUTPUT_HANDLE );
	CONSOLE_SCREEN_BUFFER_INFO info;
	GetConsoleScreenBufferInfo( hStdout, &info );
	termw = info.dwSize.X;
	termh = info.dwSize.Y;
	if ( !termw ) termw = 80;
}
static void set_console_mode(void)
{
	int mode=0;
	const HANDLE hStdout = GetStdHandle( STD_OUTPUT_HANDLE );
	GetConsoleMode( hStdout, &mode );
	mode = mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING;
	SetConsoleMode( hStdout, mode );
}
#else
static void get_terminal_size(void)
{
	FILE* f = popen( "stty size", "r" );
	if ( !f )
	{
		fprintf( stderr, "Failed to determine terminal size using stty.\n" );
		exit( 1 );
	}
	const int num = fscanf( f, "%d %d", &termh, &termw );
	assert( num == 2 );
	pclose( f );
}
static void set_console_mode() {}
#endif



#define RESETALL  "\x1b[0m"


static void print_image( int w, int h, unsigned char* data )
{
	const int linesz = 16384;
	char line[ linesz ];
	unsigned char* reader = data;

	for ( int y=0; y<h; ++y )
	{
		line[0] = 0;
		for ( int x=0; x<w; ++x )
		{
			strncat( line, "\x1b[48;2;", sizeof(line) - strlen(line) - 1 );
			char tripl[80];
			unsigned char r = *reader++;
			unsigned char g = *reader++;
			unsigned char b = *reader++;
			unsigned char a = *reader++;
			(void) a;
			snprintf( tripl, sizeof(tripl), "%d;%d;%dm ", r,g,b );
			strncat( line, tripl, sizeof(line) - strlen(line) - 1 );
		}
		strncat( line, RESETALL, sizeof(line) - strlen(line) - 1 );
		puts( line );
	}
	assert( reader = data + w * h * 4 );
}


static int process_image( const char* nm )
{
	int imw=0,imh=0,n=0;
	unsigned char *data = stbi_load( nm, &imw, &imh, &n, 4 );
	if ( !data )
		return -1;
	//fprintf( stderr, "%s has dimension %dx%d w %d components.\n", nm, imw, imh, n );

	float aspectratio = imw / (float) imh;
	float pixels_per_char = imw / (float)termw;
	if ( pixels_per_char < 1 ) pixels_per_char = 1;
	int kernelsize = (int) floorf( pixels_per_char );
	if ( (kernelsize&1) == 0 ) kernelsize--;
	if ( !kernelsize ) kernelsize=1;
	const int kernelradius = (kernelsize-1)/2;

	const int outw = imw < termw ? imw : termw;
	const int outh = (int) roundf( outw / aspectratio );

	//fprintf( stderr, "pixels per char: %f, kernelsize: %d, out: %dx%d\n", pixels_per_char, kernelsize, outw, outh );

	unsigned char out[ outh ][ outw ][ 4 ];
	for ( int y=0; y<outh; ++y )
		for ( int x=0; x<outw; ++x )
		{
			const int cx = (int) roundf( pixels_per_char * x );
			const int cy = (int) roundf( pixels_per_char * y );
			int acc[4] = {0,0,0,0};
			int numsamples=0;
			int sy = cy-kernelradius;
			sy = sy < 0 ? 0 : sy;
			int ey = cy+kernelradius;
			ey = ey >= imh ? imh-1 : ey;
			int sx = cx-kernelradius;
			sx = sx < 0 ? 0 : sx;
			int ex = cx+kernelradius;
			ex = ex >= imw ? imw-1 : ex;
			for ( int yy = sy; yy <= ey; ++yy )
				for ( int xx = sx; xx <= ex; ++xx )
				{
					unsigned char* reader = data + ( yy * imw * 4 ) + xx * 4;
					acc[ 0 ] += reader[0];
					acc[ 1 ] += reader[1];
					acc[ 2 ] += reader[2];
					acc[ 3 ] += reader[3];
					numsamples++;
				}
			out[ y ][ x ][ 0 ] = acc[ 0 ] / numsamples;
			out[ y ][ x ][ 1 ] = acc[ 1 ] / numsamples;
			out[ y ][ x ][ 2 ] = acc[ 2 ] / numsamples;
			out[ y ][ x ][ 3 ] = acc[ 3 ] / numsamples;
		}

	stbi_image_free( data );
	data = 0;

	print_image( outw, outh, (unsigned char*) out );
	return 0;
}


int main( int argc, char* argv[] )
{
	if ( argc == 1 || !strcmp( argv[1], "--help" ) )
	{
		fprintf( stderr, "Usage: %s image [image2 .. imageN]\n", argv[0] );
		exit( 0 );
	}

	// Step 0: Windows cmd.exe needs to be put in proper console mode.
	set_console_mode();

	// Step 1: figure out the width and height of terminal.
	get_terminal_size();
	//fprintf( stderr, "Your terminal is size %dx%d\n", termw, termh );

	// Step 2: Process all images on the command line.
	for ( int i=1; i<argc; ++i )
	{
		const char* nm = argv[ i ];
		int rv = process_image( nm );
		if ( rv < 0 )
			fprintf( stderr, "Could not load image %s\n", nm );
	}

	return 0;
}

