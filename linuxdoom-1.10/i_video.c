// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id:$
//
// Copyright (C) 1993-1996 by id Software, Inc.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// $Log:$
//
// DESCRIPTION:
//	DOOM graphics stuff for X11, UNIX.
//
//-----------------------------------------------------------------------------

#include <string.h>

#include "doomstat.h"
#include "i_system.h"
#include "ib_video.h"
#include "v_video.h"
#include "m_argv.h"
#include "d_main.h"

#include "doomdef.h"

#include "i_video.h"

#define SCALER

// TODO - Dynamically allocate these according to bytes_per_pixel
static unsigned char colors[256 * 4];

static unsigned int bytes_per_pixel;

static unsigned char colored_screen[SCREENWIDTH * SCREENHEIGHT * 4];

#ifdef SCALER
static unsigned char **upscale_lut;
#endif

static size_t output_width;
static size_t output_height;

// Blocky mode,
// replace each 320x200 pixel with multiply*multiply pixels.
// According to Dave Taylor, it still is a bonehead thing
// to use ....
static int	multiply=1;


void I_ShutdownGraphics(void)
{
    IB_ShutdownGraphics();
}


//
// I_StartFrame
//
void I_StartFrame (void)
{
    // er?
}


//
// I_StartTic
//
void I_StartTic (void)
{
    IB_StartTic();
}


//
// I_UpdateNoBlit
//
void I_UpdateNoBlit (void)
{
    // what is this?
}


//
// I_FinishUpdate
//
void I_FinishUpdate (void)
{

    static int	lasttic;
    int		tics;
    int		i;
    // UNUSED static unsigned char *bigscreen=0;

    // draws little dots on the bottom of the screen
    if (devparm)
    {

	i = I_GetTime();
	tics = i - lasttic;
	lasttic = i;
	if (tics > 20) tics = 20;

	for (i=0 ; i<tics*2 ; i+=2)
	    screens[0][ (SCREENHEIGHT-1)*SCREENWIDTH + i] = 0xff;
	for ( ; i<20*2 ; i+=2)
	    screens[0][ (SCREENHEIGHT-1)*SCREENWIDTH + i] = 0x0;
    
    }

    // Step 1. Color the screen
    unsigned char *indexed_pixels = screens[0];
    unsigned char *colored_screen_pointer = colored_screen;

    for (size_t i = 0; i < SCREENWIDTH * SCREENHEIGHT; ++i)
    {
	unsigned char *color = &colors[*indexed_pixels++ * bytes_per_pixel];

	for (unsigned int j = 0; j < bytes_per_pixel; ++j)
	    *colored_screen_pointer++ = color[j];
    }

    unsigned char *pixels;
    size_t pitch;
    IB_GetFramebuffer(&pixels, &pitch);

//    for (size_t y = 0; y < SCREENHEIGHT; ++y)
//	memcpy(&pixels[y * pitch], &colored_screen[y * SCREENWIDTH * bytes_per_pixel], SCREENWIDTH * bytes_per_pixel);

#ifdef SCALER
    unsigned char **upscale_lut_pointer = upscale_lut;

    for (size_t y = 0; y < output_height; ++y)
    {
	unsigned char *dst_row = &pixels[y * pitch];

	for (size_t x = 0; x < output_width; ++x)
	{
	    unsigned char *pixel = *upscale_lut_pointer++;

	    for (unsigned int i = 0; i < bytes_per_pixel; ++i)
		*dst_row++ = pixel[i];
	}
    }
#else
    // scales the screen size before blitting it
    if (multiply == 1)
    {
	for (size_t y = 0; y < SCREENHEIGHT; ++y)
	    memcpy(&pixels[y * pitch], &colored_screen[y * SCREENWIDTH * bytes_per_pixel], SCREENWIDTH * bytes_per_pixel);
    }
    else
    {
	const unsigned char *src_pointer = colored_screen;

	for (size_t y = 0; y < SCREENHEIGHT; ++y)
	{
	    unsigned char *dst_row = &pixels[y * 4 * pitch];
	    unsigned char *dst_pointer = dst_row;

	    for (size_t x = 0; x < SCREENWIDTH; ++x)
	    {
		for (int i = 0; i < multiply; ++i)
		    for (unsigned int j = 0; j < bytes_per_pixel; ++j)
			*dst_pointer++ = src_pointer[j];

		src_pointer += bytes_per_pixel;
	    }

	    for (int i = 1; i < multiply; ++i)
		memcpy(&dst_row[i * pitch], dst_row, output_width * bytes_per_pixel);
	}
    }
#endif

    IB_FinishUpdate();
}


//
// I_ReadScreen
//
void I_ReadScreen (byte* scr)
{
    memcpy(scr, screens[0], SCREENWIDTH*SCREENHEIGHT);
}


//
// I_SetPalette
//
void I_SetPalette (const byte* palette)
{
    register int	i;
    const unsigned char* gamma = gammatable[usegamma];

    for (i=0 ; i<256 ; i++)
    {
	IB_GetColor(&colors[i * bytes_per_pixel], gamma[palette[0]], gamma[palette[1]], gamma[palette[2]]);
	palette += 3;
    }
}


void I_InitGraphics(void)
{
    static int		firsttime=1;

    if (!firsttime)
	return;
    firsttime = 0;

    if (M_CheckParm("-2"))
	multiply = 2;

    if (M_CheckParm("-3"))
	multiply = 3;

    if (M_CheckParm("-4"))
	multiply = 4;

    output_width = SCREENWIDTH * multiply;
#ifdef SCALER
    output_height = SCREENWIDTH * multiply * (3.0/4.0);
#else
    output_height = SCREENHEIGHT * multiply;
#endif

    IB_InitGraphics(output_width, output_height, &bytes_per_pixel);

    I_GrabMouse(true);

#ifdef SCALER
    // Let's create the upscale LUT
    upscale_lut = malloc(output_width * output_height * sizeof(*upscale_lut));
    unsigned char **upscale_lut_pointer = upscale_lut;

    for (size_t y = 0; y < output_height; ++y)
	for (size_t x = 0; x < output_width; ++x)
	    *upscale_lut_pointer++ = &colored_screen[(((y * SCREENHEIGHT / output_height) * SCREENWIDTH) + (x * SCREENWIDTH / output_width)) * bytes_per_pixel];
#endif
}


void I_GrabMouse(boolean grab)
{
    IB_GrabMouse(grab);
}
