// imcat.c
//
// by Abraham Stolk.
// This software is in the Public Domain.

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include <errno.h> // Needed for strtol error checking in get_int function

#if defined(_WIN64)
#	define STBI_NO_SIMD
#endif
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

static int termw=0, termh=0;
static int doubleres=0;
static int blend=0;
static unsigned char termbg[3] = { 0,0,0 };
// variables to hold user passed width and height
static int uw=-1, uh=-1;


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
static int oldcodepage=0;
static void set_console_mode(void)
{
	DWORD mode=0;
	const HANDLE hStdout = GetStdHandle( STD_OUTPUT_HANDLE );
	GetConsoleMode( hStdout, &mode );
	mode = mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING;
	SetConsoleMode( hStdout, mode );
	oldcodepage = GetConsoleCP();
	SetConsoleCP( 437 );
	doubleres = 1;
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
static void set_console_mode()
{
	doubleres=1;
}
#endif



#define RESETALL  "\x1b[0m"


static void print_image_single_res( int w, int h, unsigned char* data )
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

#if defined(_WIN64)
#	define HALFBLOCK "\xdf"		// Uses IBM PC Codepage 437 char 223
#else
#	define HALFBLOCK "▀"		// Uses Unicode char U+2580
#endif

// Note: image has alpha pre-multied. Mimic GL_ONE + GL_ONE_MINUS_SRC_ALPHA
#define BLEND \
{ \
	const int t0 = 255; \
	const int t1 = 255-a; \
	r = ( r * t0 + termbg[0] * t1 ) / 255; \
	g = ( g * t0 + termbg[1] * t1 ) / 255; \
	b = ( b * t0 + termbg[2] * t1 ) / 255; \
}

static void print_image_double_res( int w, int h, unsigned char* data )
{
	if ( h & 1 )
		h--;
	const int linesz = 32768;
	char line[ linesz ];


	for ( int y=0; y<h; y+=2 )
	{
		const unsigned char* row0 = data + (y+0) * w * 4;
		const unsigned char* row1 = data + (y+1) * w * 4;
		line[0] = 0;
		for ( int x=0; x<w; ++x )
		{
			// foreground colour.
			strncat( line, "\x1b[38;2;", sizeof(line) - strlen(line) - 1 );
			char tripl[80];
			unsigned char r = *row0++;
			unsigned char g = *row0++;
			unsigned char b = *row0++;
			unsigned char a = *row0++;
			if ( blend )
				BLEND
			snprintf( tripl, sizeof(tripl), "%d;%d;%dm", r,g,b );
			strncat( line, tripl, sizeof(line) - strlen(line) - 1 );
			// background colour.
			strncat( line, "\x1b[48;2;", sizeof(line) - strlen(line) - 1 );
			r = *row1++;
			g = *row1++;
			b = *row1++;
			a = *row1++;
			if ( blend )
				BLEND
			snprintf( tripl, sizeof(tripl), "%d;%d;%dm" HALFBLOCK, r,g,b );
			strncat( line, tripl, sizeof(line) - strlen(line) - 1 );
		}
		strncat( line, RESETALL, sizeof(line) - strlen(line) - 1 );
		puts( line );
	}
}


static int process_image( const char* nm )
{
	int imw=0,imh=0,n=0;
	int outw, outh;

	unsigned char *data = stbi_load( nm, &imw, &imh, &n, 4 );
	if ( !data )
		return -1;
	//fprintf( stderr, "%s has dimension %dx%d w %d components.\n", nm, imw, imh, n );

	float aspectratio = imw / (float) imh;

	if ( uw != -1 )
	{
		outw = uw;
		outh = (int) roundf( outw / aspectratio );
	}
	else if ( uh != -1 )
	{
		outh = uh;
		outw = (int) roundf( outh * aspectratio );
	}
	else
	{
		outw = imw < termw ? imw : termw;
		outh = (int) roundf( outw / aspectratio );
	}

	float pixels_per_char = imw / (float)outw;
	if ( pixels_per_char < 1 ) pixels_per_char = 1;
	int kernelsize = (int) floorf( pixels_per_char );
	if ( (kernelsize&1) == 0 ) kernelsize--;
	if ( !kernelsize ) kernelsize=1;
	const int kernelradius = (kernelsize-1)/2;

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
					const int a = reader[3];
					acc[ 0 ] += a * reader[0] / 255;
					acc[ 1 ] += a * reader[1] / 255;
					acc[ 2 ] += a * reader[2] / 255;
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

	if ( doubleres )
		print_image_double_res( outw, outh, (unsigned char*) out );
	else
		print_image_single_res( outw, outh, (unsigned char*) out );
	return 0;
}


int get_int( const char* num,const char* msg )
{
    // Code loosely based on strtol's man page
	char *endptr;
	long val;

	errno = 0; // Catches error status from next line
	val = strtol( num, &endptr, 10 );

	// check for possible errors
	if ( errno != 0 || endptr == num ) {
		fprintf( stderr, msg, num );
		exit( 1 );
	}

	return val;
}



int main( int argc, char* argv[] )
{
	if ( argc == 1 || !strcmp( argv[1], "--help" ) )
	{
		fprintf( stderr, "Usage:\n"
						 " %s image [options] [image2 .. imageN]\n"
						 "Displays image in terminal\n\nOptions:\n"
						 " -h, --height   set height of output image (image will maintain aspect ratio)\n"
						 " -w, --width    set width of output image (image will maintain aspect ratio)\n"
						 "     --help     display this help and exit\n\n",
				 argv[0] );
		exit( 0 );
	}

	// Get location and value of passed flags
	int hloc=-1, wloc=-1;
	for ( int i=1; i<argc; ++i )
	{

		if ( !strcmp( argv[i], "-w" ) || !strcmp( argv[i], "--width" ) )
		{
			wloc = i;
			if ( i+1 < argc )
			{
				uw = get_int( argv[i+1], "Invalid width value \"%s\"\n" );
			}
			else
			{
				fprintf( stderr, "Missing width value\n" );
				exit( -1 );
			}
		}
		else if ( !strcmp( argv[i], "-h" ) || !strcmp( argv[i], "--height" ) )
		{
			hloc = i;
			if ( i+1 < argc )
			{
				uh = get_int( argv[i+1], "Invalid height value \"%s\"\n" );
			}
			else
			{
				fprintf( stderr, "Missing height value\n" );
				exit( -1 );
			}
		}
	}

	// Parse environment variable for terminal background colour.
	const char* imcatbg = getenv( "IMCATBG" );
	if ( imcatbg )
	{
		const int bg = strtol( imcatbg+1, 0, 16 );
		termbg[ 2 ] = ( bg >>  0 ) & 0xff;
		termbg[ 1 ] = ( bg >>  8 ) & 0xff;
		termbg[ 0 ] = ( bg >> 16 ) & 0xff;
		blend = 1;
	}

	// Step 0: Windows cmd.exe needs to be put in proper console mode.
	set_console_mode();

	// Step 1: figure out the width and height of terminal.
	get_terminal_size();
	//fprintf( stderr, "Your terminal is size %dx%d\n", termw, termh );

	// Step 2: Process all images on the command line.
	for ( int i=1; i<argc; ++i )
	{
	    // Ignore arguments that are not image paths
		if ( i == hloc || i == hloc + 1 || i == wloc || i == wloc + 1 )
		{
			continue;
		}
		const char* nm = argv[ i ];
		int rv = process_image( nm );
		if ( rv < 0 )
			fprintf( stderr, "Could not load image %s\n", nm );
	}

#if defined(_WIN64)
	SetConsoleCP( oldcodepage );
#endif

	return 0;
}
