//
// Rollo X1038 label printer driver for CUPS.
//
// Copyright © 2018-2024 by Nelu LLC.
//
// This program is free software.  Distribution and use rights are outlined in
// the file "COPYING".
//

#include <cups/cups.h>
#undef _CUPS_DEPRECATED_1_6_MSG
#define _CUPS_DEPRECATED_1_6_MSG(x)
#include <cups/ppd.h>
#include "rollo-dither.h"
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>


//
// Globals...
//

bool	Canceled = false;		// `true` if job is canceled


//
// Prototypes...
//

void	cancel_job(int sig);
void	end_page(rollo_dither_t *dither, ppd_file_t *ppd, cups_page_header2_t *header);
bool	start_page(rollo_dither_t *dither, ppd_file_t *ppd, cups_page_header2_t *header);


//
// 'main()' - Main entry and processing of driver.
//

int					// O - Exit status
main(int  argc,				// I - Number of command-line arguments
     char *argv[])			// I - Command-line arguments
{
  int			fd;		// File descriptor
  int			num_options;	// Number of options
  cups_option_t		*options;	// Options
  cups_raster_t		*ras;		// Raster stream for printing
  ppd_file_t		*ppd;		// PPD file
  rollo_dither_t	dither;		// Dither data
  cups_page_header2_t	header;		// Page header from file
  unsigned		page = 0;	// Current page
  unsigned		y;		// Current line
  unsigned char		*line;		// Line buffer


  // Make sure status messages are not buffered...
  setbuf(stderr, NULL);

  // Check command-line...
  if (argc < 6 || argc > 7)
  {
    // We don't have the correct number of arguments; write an error message
    // and return.

    fputs("ERROR: Usage: rastertorollo job-id user title copies options [file]\n", stderr);
    return (1);
  }

  // Open the page stream...
  if (argc == 7)
  {
    // Read from a file...
    if ((fd = open(argv[6], O_RDONLY)) == -1)
    {
      perror("ERROR: Unable to open raster file");
      sleep(1);
      return (1);
    }
  }
  else
  {
    // Read from stdin...
    fd = 0;
  }

  ras = cupsRasterOpen(fd, CUPS_RASTER_READ);

  // Register a signal handler to eject the current label if the job is canceled.
  signal(SIGTERM, cancel_job);

  // Open the PPD file and apply options...
  num_options = cupsParseOptions(argv[5], 0, &options);
  if ((ppd = ppdOpenFile(getenv("PPD"))) == NULL)
  {
    ppd_status_t	status;		// PPD error
    int			linenum;	// Line number

    fputs("ERROR: The PPD file could not be opened.\n", stderr);

    status = ppdLastError(&linenum);

    fprintf(stderr, "DEBUG: %s on line %d.\n", ppdErrorString(status), linenum);

    return (1);
  }

  ppdMarkDefaults(ppd);
  cupsMarkOptions(ppd, num_options, options);

  // Process pages as needed...
  while (cupsRasterReadHeader2(ras, &header))
  {
    // Write a status message with the page number and number of copies.
    if (Canceled)
      break;

    // Start the page...
    if (!start_page(&dither, ppd, &header))
      return (1);

    if ((line = malloc(header.cupsBytesPerLine)) == NULL)
    {
      perror("ERROR: Unable to allocate memory");
      return (1);
    }

    page ++;

    fprintf(stderr, "PAGE: %u 1\n", page);

    // Loop for each line on the page...
    for (y = 0; y < header.cupsHeight && !Canceled; y ++)
    {
      // Let the user know how far we have progressed...
      if (Canceled)
	break;

      if ((y & 15) == 0)
        fprintf(stderr, "ATTR: job-media-progress=%u\n", 100 * y / header.cupsHeight);

      // Read a line of graphics...
      if (cupsRasterReadPixels(ras, line, header.cupsBytesPerLine) < 1)
        break;

      // Write it to the printer...
      rollo_dither_line(&dither, y, line);
    }

    // Eject the page...
    end_page(&dither, ppd, &header);

    if (Canceled)
      break;
  }

  // Close the raster stream...
  cupsRasterClose(ras);
  if (fd != 0)
    close(fd);

  // Close the PPD file and free the options...
  ppdClose(ppd);
  cupsFreeOptions(num_options, options);

  return (0);
}


//
// 'cancel_job()' - Cancel the current job...
//

void
cancel_job(int sig)			// I - Signal (unused)
{
  // Tell the main loop to stop...
  (void)sig;

  Canceled = true;
}


//
// 'end_page()' - Finish a page of graphics.
//

void
end_page(rollo_dither_t      *dither,	// I - Dither buffer
	 ppd_file_t          *ppd,	// I - PPD file
	 cups_page_header2_t *header)	// I - Page header
{
  ppd_choice_t	*choice;		// Marked choice
  int		darkness,		// Print darkness
		speed,			// Print speed
		adjust_h,		// Horizontal adjustment
		adjust_v,		// Vertical adjustment
		rotate,			// Rotation
		offset;			// Feed offset
  unsigned	dark_count,		// Number of dark pixels on line
		dark_lines = 0;		// Number of consecutive dark lines
  bool		dark_page = false;	// Is this a dark page?
  unsigned	x, y;			// Current coordinates
  unsigned char	*outptr;		// Pointer into output buffer
  static unsigned char counts[16] =	// Black (0) pixel counts for 4 bits
  {
    4,					// 0000
    3,					// 0001
    3,					// 0010
    2,					// 0011
    3,					// 0100
    2,					// 0101
    2,					// 0110
    1,					// 0111
    3,					// 1000
    2,					// 1001
    2,					// 1010
    1,					// 1011
    2,					// 1100
    1,					// 1101
    1,					// 1110
    0					// 1111
  };


  // Finalize last line...
  rollo_dither_line(dither, header->cupsHeight, NULL);

  // Figure out the print speed and darkness...
  darkness = 8;
  if ((choice = ppdFindMarkedChoice(ppd, "roDarkness")) != NULL && strcasecmp(choice->choice, "Default"))
    darkness = atoi(choice->choice);

  speed = 6;
  if ((choice = ppdFindMarkedChoice(ppd, "roPrintRate")) != NULL && strcasecmp(choice->choice, "Default"))
    speed = atoi(choice->choice);

  for (outptr = dither->output, y = header->cupsHeight; y > 0; y --)
  {
    // Look for a series of dark lines - the X1038 power adapter doesn't put
    // out enough current to print 100% black at full speed...
    for (x = dither->out_width, dark_count = 0; x > 0; x --, outptr ++)
      dark_count += counts[*outptr >> 4] + counts[*outptr & 15];

    if (dark_count >= (6 * dither->out_width))
    {
      dark_lines ++;

      if (dark_lines > 100)
      {
	dark_page = true;
	break;
      }
    }
    else
    {
      dark_lines = 0;
    }
  }

  if (dark_page && darkness >= 2 && speed > 2)
  {
    // Reduce darkness and print speed to print dark page...
    fputs("DEBUG: Dark label, reduced print speed/darkness.\n", stderr);
    darkness -= 2;
    speed    -= 2;
  }
  else
  {
    // Print at full speed...
    fputs("DEBUG: Normal label, full print speed/darkness.\n", stderr);
  }

  // Initialize page...
  printf("SIZE %u mm ,%u mm\n", dither->out_width, (header->cupsHeight + 7) / 8);

  if ((choice = ppdFindMarkedChoice(ppd, "roAdjustHorizontal")) != NULL)
    adjust_h = 8 * atoi(choice->choice);
  else
    adjust_h = 0;

  if ((choice = ppdFindMarkedChoice(ppd, "roAdjustVertical")) != NULL)
    adjust_v = 8 * atoi(choice->choice);
  else
    adjust_v = 0;

  printf("REFERENCE %d,%d\n", adjust_h, adjust_v);

  if ((choice = ppdFindMarkedChoice(ppd, "roRotate")) != NULL)
    rotate = atoi(choice->choice);
  else
    rotate = 0;

  printf("DIRECTION %d,0\n", rotate);

  if ((choice = ppdFindMarkedChoice(ppd, "roMediaTracking")) != NULL)
  {
    if (!strcasecmp(choice->choice, "Continuous"))
      puts("GAP 0 mm,0 mm");
    else if (!strcasecmp(choice->choice, "Gap"))
      puts("GAP 3 mm,0 mm");
    else if (!strcasecmp(choice->choice, "BLine"))
      puts("BLINE 3 mm,0 mm");
  }

  if ((choice = ppdFindMarkedChoice(ppd, "roFeedOffset")) != NULL)
    offset = atoi(choice->choice);
  else
    offset = 0;
  printf("OFFSET %d mm\n", offset);

  printf("DENSITY %d\n", darkness);
  printf("SPEED %d\n", speed);

  if ((choice = ppdFindMarkedChoice(ppd, "AutoDotted")) != NULL && !strcasecmp(choice->choice, "true"))
    puts("SETC AUTODOTTED ON");
  else
    puts("SETC AUTODOTTED OFF");

  puts("SETC PAUSEKEY ON");

  puts("SETC WATERMARK OFF");

  puts("CLS");

  printf("BITMAP 0,0,%u,%u,1,", dither->out_width, header->cupsHeight);
  fwrite(dither->output, 1, dither->out_width * header->cupsHeight, stdout);

  // Eject page
  puts("PRINT 1,1");
  fflush(stdout);

  // Free memory...
  rollo_dither_free(dither);
}


//
// 'start_page()' - Start a page of graphics.
//

bool					// O - `true` on success, `false` on failure
start_page(rollo_dither_t      *dither,	// I - Dither buffer
           ppd_file_t          *ppd,	// I - PPD file
           cups_page_header2_t *header)	// I - Page header
{
  bool		ret;			// Return value
  ppd_choice_t	*choice;		// Marked choice
  static const rollo_matrix_t draft =
  {					// Threshold dither
    { 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127 },
    { 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127 },
    { 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127 },
    { 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127 },
    { 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127 },
    { 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127 },
    { 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127 },
    { 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127 },
    { 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127 },
    { 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127 },
    { 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127 },
    { 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127 },
    { 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127 },
    { 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127 },
    { 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127 },
    { 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127 }
  };
  static const rollo_matrix_t normal =
  {					// Clustered-Dot Dither Matrix
    {  96,  40,  48, 104, 140, 188, 196, 148,  97,  41,  49, 105, 141, 189, 197, 149 },
    {  32,   0,   8,  56, 180, 236, 244, 204,  33,   1,   9,  57, 181, 237, 245, 205 },
    {  88,  24,  16,  64, 172, 228, 252, 212,  89,  25,  17,  65, 173, 229, 253, 213 },
    { 120,  80,  72, 112, 132, 164, 220, 156, 121,  81,  73, 113, 133, 165, 221, 157 },
    { 136, 184, 192, 144, 100,  44,  52, 108, 137, 185, 193, 145, 101,  45,  53, 109 },
    { 176, 232, 240, 200,  36,   4,  12,  60, 177, 233, 241, 201,  37,   5,  13,  61 },
    { 168, 224, 248, 208,  92,  28,  20,  68, 169, 225, 249, 209,  93,  29,  21,  69 },
    { 128, 160, 216, 152, 124,  84,  76, 116, 129, 161, 217, 153, 125,  85,  77, 117 },
    {  98,  42,  50, 106, 142, 190, 198, 150,  99,  43,  51, 107, 143, 191, 199, 151 },
    {  34,   2,  10,  58, 182, 238, 246, 206,  35,   3,  11,  59, 183, 239, 247, 207 },
    {  90,  26,  18,  66, 174, 230, 254, 214,  91,  27,  19,  67, 175, 231, 254, 215 },
    { 122,  82,  74, 114, 134, 166, 222, 158, 123,  83,  75, 115, 135, 167, 223, 159 },
    { 138, 186, 194, 146, 102,  46,  54, 110, 139, 187, 195, 147, 103,  47,  55, 111 },
    { 178, 234, 242, 202,  38,   6,  14,  62, 179, 235, 243, 203,  39,   7,  15,  63 },
    { 170, 226, 250, 210,  94,  30,  22,  70, 171, 227, 251, 211,  95,  31,  23,  71 },
    { 130, 162, 218, 154, 126,  86,  78, 118, 131, 163, 219, 155, 127,  87,  79, 119 }
  };
  static const rollo_matrix_t high =	// Blue-noise dither array
  {
    { 111,  49, 142, 162, 113, 195,  71, 177, 201,  50, 151,  94,  66,  37,  85, 252 },
    {  25,  99, 239, 222,  32, 250, 148,  19,  38, 106, 220, 170, 194, 138,  13, 167 },
    { 125, 178,  79,  15,  65, 173, 123,  87, 213, 131, 247,  23, 116,  54, 229, 212 },
    {  41, 202, 152, 132, 189, 104,  53, 236, 161,  62,   1, 181,  77, 241, 147,  68 },
    {   2, 244,  56,  91, 230,   5, 204,  28, 187, 101, 144, 206,  33,  92, 190, 107 },
    { 223, 164, 114,  36, 214, 156, 139,  70, 245,  84, 226,  48, 126, 158,  17, 135 },
    {  83, 196,  21, 254,  76,  45, 179, 115,  12,  40, 169, 105, 253, 176, 211,  59 },
    { 100, 180, 145, 122, 172,  97, 235, 129, 215, 149, 199,   8,  72,  26, 238,  44 },
    { 232,  31,  69,  11, 205,  58,  18, 193,  88,  60, 112, 221, 140,  86, 120, 153 },
    { 208, 130, 243, 160, 224, 110,  34, 248, 165,  24, 234, 184,  52, 198, 171,   6 },
    { 108, 188,  51,  89, 137, 186, 154,  78,  47, 134,  98, 157,  35, 249,  95,  63 },
    {  16,  75, 219,  39,   0,  67, 228, 121, 197, 240,   3,  74, 127,  20, 227, 143 },
    { 246, 175, 119, 200, 251, 103, 146,  14, 209, 174, 109, 218, 192,  82, 203, 163 },
    {  29,  93, 150,  22, 166, 182,  55,  30,  90,  64,  42, 141, 168,  57, 117,  46 },
    { 216, 233,  61, 128,  81, 237, 217, 118, 159, 255, 185,  27, 242, 102,   4, 133 },
    {  73, 191,   9, 210,  43,  96,   7, 136, 231,  80,  10, 124, 225, 207, 155, 183 }
  };


  // Initialize the dither buffer...
  if ((choice = ppdFindMarkedChoice(ppd, "cupsPrintQuality")) == NULL || !strcasecmp(choice->choice, "Normal"))
    ret = rollo_dither_alloc(dither, normal, header, CUPS_CSPACE_W);
  else if (!strcasecmp(choice->choice, "High"))
    ret = rollo_dither_alloc(dither, high, header, CUPS_CSPACE_W);
  else
    ret = rollo_dither_alloc(dither, draft, header, CUPS_CSPACE_W);

  return (ret);
}
