//
// Rollo dithering definitions.
//
// This dithering code is based (with permission) on the LPrint dithering code.
//
// Copyright © 2020-2024 by Nelu LLC.
// Copyright © 2019-2023 by Michael R Sweet.
//
// This program is free software.  Distribution and use rights are outlined in
// the file "COPYING".
//

#ifndef ROLLO_DITHER_H
#  define ROLLO_DITHER_H
#  include <stdbool.h>
#  include <cups/raster.h>


//
// Constants...
//

#define ROLLO_DOT_WIDTH		832	// Width of print head in dots
#define ROLLO_BYTE_WIDTH	104	// Width of print head in bytes


//
// Types...
//


typedef unsigned char rollo_matrix_t[16][16];
					// Dither matrix

typedef struct rollo_dither_s		// Dithering state
{
  rollo_matrix_t dither;		// Dither matrix to use
  unsigned char	*input[4];		// Input lines for dithering (only 3 are needed, 4 makes it easier for ring buffer)
  unsigned	in_width,		// Width in pixels
		in_height,		// Height in lines
		in_left,		// Left (starting) pixel
		in_top,			// Top-most pixel
		in_bottom;		// Bottom-most pixel
  unsigned char	in_bpp,			// Input bits per pixel (1 or 8)
		in_white;		// Input white pixel value (0 or 255)
  unsigned char	*output,		// Output bitmap
		out_white;		// Output white pixel value (0 or 255)
  unsigned	out_width;		// Output width in bytes
} rollo_dither_t;


//
// Functions...
//

extern bool	rollo_dither_alloc(rollo_dither_t *dither, const rollo_matrix_t m, cups_page_header2_t *header, cups_cspace_t out_cspace);
extern void	rollo_dither_free(rollo_dither_t *dither);
extern bool	rollo_dither_line(rollo_dither_t *dither, unsigned y, const unsigned char *line);


#endif // !ROLLO_DITHER_H
