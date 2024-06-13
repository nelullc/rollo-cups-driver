//
// Rollo dithering code.
//
// This dithering code is based (with permission) on the LPrint dithering code.
//
// Copyright © 2020-2024 by Nelu LLC.
// Copyright © 2019-2023 by Michael R Sweet.
//
// This program is free software.  Distribution and use rights are outlined in
// the file "COPYING".
//

#include "rollo-dither.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>


//
// Local constants...
//

#define ROLLO_WHITE	56
#define ROLLO_BLACK	199


//
// 'rollo_dither_alloc()' - Allocate memory for a dither buffer.
//

bool					// O - `true` on success, `false` on error
rollo_dither_alloc(
    rollo_dither_t       *dither,	// I - Dither buffer
    const rollo_matrix_t m,		// I - Dither matrix
    cups_page_header2_t  *header,	// I - Page header
    cups_cspace_t        out_cspace)	// I - Output color space
{
  int		i, j;			// Looping vars
  unsigned	right;			// Right margin


  // Initialize dither buffer
  memset(dither, 0, sizeof(rollo_dither_t));

  // Adjust dithering array and compress to a range of ROLLO_WHITE to ROLLO_BLACK
  for (i = 0; i < 16; i ++)
  {
    for (j = 0; j < 16; j ++)
      dither->dither[i][j] = (unsigned char)((ROLLO_BLACK - ROLLO_WHITE) * m[i][j] / 255 + ROLLO_WHITE);
  }

  // Calculate margins and dimensions...
  if (!header->cupsInteger[CUPS_RASTER_PWG_ImageBoxBottom])
    header->cupsInteger[CUPS_RASTER_PWG_ImageBoxBottom] = header->cupsHeight - 1;
  if (!header->cupsInteger[CUPS_RASTER_PWG_ImageBoxRight])
    header->cupsInteger[CUPS_RASTER_PWG_ImageBoxRight] = header->cupsWidth - 1;

  dither->in_left   = header->cupsInteger[CUPS_RASTER_PWG_ImageBoxLeft];
  right             = header->cupsInteger[CUPS_RASTER_PWG_ImageBoxRight];
  dither->in_top    = header->cupsInteger[CUPS_RASTER_PWG_ImageBoxTop];
  dither->in_bottom = header->cupsInteger[CUPS_RASTER_PWG_ImageBoxBottom];
  dither->in_width  = right - dither->in_left + 1;
  dither->in_height = dither->in_bottom - dither->in_top + 1;
  dither->out_width = (right - dither->in_left + 8) / 8;

  if (dither->in_width > 65536)
  {
    fprintf(stderr, "ERROR: Page too wide.\n");
    return (false);			// Protect against large allocations
  }

  // Calculate input/output color values
  dither->in_bpp = header->cupsBitsPerPixel;

  switch (header->cupsColorSpace)
  {
    case CUPS_CSPACE_W :
    case CUPS_CSPACE_SW :
    case CUPS_CSPACE_RGB :
    case CUPS_CSPACE_SRGB :
    case CUPS_CSPACE_ADOBERGB :
        dither->in_white = 255;
        break;

    default :
        dither->in_white = 0;
        break;
  }

  switch (out_cspace)
  {
    case CUPS_CSPACE_W :
    case CUPS_CSPACE_SW :
    case CUPS_CSPACE_RGB :
    case CUPS_CSPACE_SRGB :
    case CUPS_CSPACE_ADOBERGB :
        dither->out_white = 255;
        break;

    default :
        dither->out_white = 0;
        break;
  }

  // Allocate memory...
  if ((dither->input[0] = calloc(4 * dither->in_width, sizeof(unsigned char))) == NULL)
  {
    fprintf(stderr, "ERROR: Unable to allocate input buffer.\n");
    return (false);
  }

  for (i = 1; i < 4; i ++)
    dither->input[i] = dither->input[0] + i * dither->in_width;

  if ((dither->output = malloc(dither->out_width * dither->in_height)) == NULL)
  {
    fprintf(stderr, "ERROR: Unable to allocate output buffer.\n");
    return (false);
  }

  fprintf(stderr, "DEBUG: dither=[\n");
  for (i = 0; i < 16; i ++)
    fprintf(stderr, "DEBUG:   [ %3u %3u %3u %3u %3u %3u %3u %3u %3u %3u %3u %3u %3u %3u %3u %3u ]\n", dither->dither[i][0], dither->dither[i][1], dither->dither[i][2], dither->dither[i][3], dither->dither[i][4], dither->dither[i][5], dither->dither[i][6], dither->dither[i][7], dither->dither[i][8], dither->dither[i][9], dither->dither[i][10], dither->dither[i][11], dither->dither[i][12], dither->dither[i][13], dither->dither[i][14], dither->dither[i][15]);
  fprintf(stderr, "DEBUG: ]\n");
  fprintf(stderr, "DEBUG: in_bottom=%u\n", dither->in_bottom);
  fprintf(stderr, "DEBUG: in_left=%u\n", dither->in_left);
  fprintf(stderr, "DEBUG: in_top=%u\n", dither->in_top);
  fprintf(stderr, "DEBUG: in_width=%u\n", dither->in_width);
  fprintf(stderr, "DEBUG: in_bpp=%u\n", dither->in_bpp);
  fprintf(stderr, "DEBUG: in_white=%u\n", dither->in_white);
  fprintf(stderr, "DEBUG: out_white=%u\n", dither->out_white);
  fprintf(stderr, "DEBUG: out_width=%u\n", dither->out_width);

  // Return indicating success...
  return (true);
}


//
// 'rollo_dither_free()' - Free memory for a dither buffer.
//

void
rollo_dither_free(
    rollo_dither_t *dither)		// I - Dither buffer
{
  free(dither->input[0]);
  free(dither->output);

  memset(dither, 0, sizeof(rollo_dither_t));
}


//
// 'rollo_dither_line()' - Copy and dither a line.
//
// This function copies the current line and dithers it as needed.  `true` is
// returned if the output line needs to be sent to the printer - the `output`
// member points to the output bitmap and `outwidth` specifies the bitmap width
// in bytes.
//
// Dithering is always 1 line behind the current line, so you need to call this
// function one last time in the endpage callback with `y` == `cupsHeight` to
// get the last line.
//

bool					// O - `true` if line dithered, `false` to skip
rollo_dither_line(
    rollo_dither_t      *dither,	// I - Dither buffer
    unsigned            y,		// I - Input line number (starting at `0`)
    const unsigned char *line)		// I - Input line
{
  unsigned	x,			// Current column
		count;			// Remaining count
  unsigned char	*current,		// Current line
		*prev,			// Previous line
		*next;			// Next line
  unsigned char	*dline,			// Dither line
		*outptr,		// Pointer into output
		byte,			// Current byte
		bit;			// Current bit


  // Copy current input line...
  count = dither->in_width;
  next  = dither->input[y & 3];

  memset(next, 0, count);

  if (line)
  {
    switch (dither->in_bpp)
    {
      case 1 : // 1-bit black
	  for (line += dither->in_left / 8, byte = *line++, bit = 128 >> (dither->in_left & 7); count > 0; count --, next ++)
	  {
	    // Convert to 8-bit black...
	    if (byte & bit)
	      *next = 255;

	    if (bit > 1)
	    {
	      bit /= 2;
	    }
	    else
	    {
	      bit  = 128;
	      byte = *line++;
	    }
	  }
	  break;

      case 8 : // Grayscale or 8-bit black
	  if (dither->in_white)
	  {
	    // Convert grayscale to black...
	    for (line += dither->in_left; count > 0; count --, next ++, line ++)
	    {
	      if (*line < ROLLO_WHITE)
		*next = 255;
	      else if (*line > ROLLO_BLACK)
		*next = 0;
	      else
		*next = 255 - *line;
	    }
	  }
	  else
	  {
	    // Copy with clamping...
	    for (line += dither->in_left; count > 0; count --, next ++, line ++)
	    {
	      if (*line < ROLLO_WHITE)
		*next = 255;
	      else if (*line > ROLLO_BLACK)
		*next = 0;
	      else
		*next = *line;
	    }
	  }
	  break;

      default : // Something else...
	  return (false);
    }
  }

  // If we are outside the imageable area then don't dither...
  if (y < (dither->in_top + 1) || y > (dither->in_bottom + 1))
    return (false);

  // Dither...
  for (x = 0, count = dither->in_width, prev = dither->input[(y - 2) & 3], current = dither->input[(y - 1) & 3], next = dither->input[y & 3], outptr = dither->output + (y - dither->in_top - 1) * dither->out_width, byte = dither->out_white, bit = 128, dline = dither->dither[y & 15]; count > 0; x ++, count --, prev ++, current ++, next ++)
  {
    if (*current)
    {
      // Not pure white/blank...
      if (*current == 255)
      {
        // 100% black...
        byte ^= bit;
      }
      else
      {
        // Only dither if this pixel does not border 100% white or black...
	if ((x > 0 && (current[-1] == 255 || current[-1] == 0)) ||
	    (count > 1 && (current[1] == 255 || current[1] == 0)) ||
	    *prev == 255 || *prev == 0 || *next == 255 || *next == 0)
        {
          // Threshold
          if (*current > 127)
	    byte ^= bit;
        }
        else if (*current > dline[x & 15])
        {
          // Dither anything else
	  byte ^= bit;
	}
      }
    }

    // Next output bit...
    if (bit > 1)
    {
      bit /= 2;
    }
    else
    {
      *outptr++ = byte;
      byte      = dither->out_white;
      bit       = 128;
    }
  }

  // Save last byte of output as needed and return...
  if (bit < 128)
    *outptr = byte;

  return (true);
}
