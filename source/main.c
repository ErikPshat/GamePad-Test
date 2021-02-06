#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <math.h>

#include <ppu-lv2.h>

#include <io/pad.h>

#include <tiny3d.h>
#include <libfont.h>

#include <ft2build.h>
#include <freetype/freetype.h> 
#include <freetype/ftglyph.h>

#include "PS_ttf_bin.h"
#include "pad.h"

//*******************************************************
//GUI
//*******************************************************

int ttf_inited = 0;

FT_Library freetype;
FT_Face face;

int TTFLoadFont(char * path, void * from_memory, int size_from_memory)
{
   
    if(!ttf_inited)
        FT_Init_FreeType(&freetype);
    ttf_inited = 1;

    if(path) {
        if(FT_New_Face(freetype, path, 0, &face)) return -1;
    } else {
        if(FT_New_Memory_Face(freetype, from_memory, size_from_memory, 0, &face)) return -1;
        }

    return 0;
}

void TTFUnloadFont()
{
   FT_Done_FreeType(freetype);
   ttf_inited = 0;
}

void TTF_to_Bitmap(u8 chr, u8 * bitmap, short *w, short *h, short *y_correction)
{
    FT_Set_Pixel_Sizes(face, (*w), (*h));
    
    FT_GlyphSlot slot = face->glyph;

    memset(bitmap, 0, (*w) * (*h));

    if(FT_Load_Char(face, (char) chr, FT_LOAD_RENDER )) {(*w) = 0; return;}

    int n, m, ww;

    *y_correction = (*h) - 1 - slot->bitmap_top;
    
    ww = 0;

    for(n = 0; n < slot->bitmap.rows; n++) {
        for (m = 0; m < slot->bitmap.width; m++) {

            if(m >= (*w) || n >= (*h)) continue;
            
            bitmap[m] = (u8) slot->bitmap.buffer[ww + m];
        }
    
    bitmap += *w;

    ww += slot->bitmap.width;
    }

    *w = ((slot->advance.x + 31) >> 6) + ((slot->bitmap_left < 0) ? -slot->bitmap_left : 0);
    *h = slot->bitmap.rows;
}

void cls()
{
	tiny3d_Clear(0xff180018, TINY3D_CLEAR_ALL);
    
    tiny3d_AlphaTest(1, 0x10, TINY3D_ALPHA_FUNC_GEQUAL);

    tiny3d_BlendFunc(1, TINY3D_BLEND_FUNC_SRC_RGB_SRC_ALPHA | TINY3D_BLEND_FUNC_SRC_ALPHA_SRC_ALPHA,
						TINY3D_BLEND_FUNC_DST_RGB_ONE_MINUS_SRC_ALPHA | TINY3D_BLEND_FUNC_DST_ALPHA_ZERO,
						TINY3D_BLEND_RGB_FUNC_ADD | TINY3D_BLEND_ALPHA_FUNC_ADD);
}

void DrawPressure(float x, float y, float h)
{
    tiny3d_SetPolygon(TINY3D_QUADS);
       
    tiny3d_VertexPos(x    , y    , 0);
    tiny3d_VertexColor(0xa00000ff);

    tiny3d_VertexPos(x + 5, y    , 0);

    tiny3d_VertexPos(x + 5, y - h/10, 0);

    tiny3d_VertexPos(x    , y - h/10, 0);

    tiny3d_End();
}

void Init_Graph()
{
    tiny3d_Init(1024*1024);
    tiny3d_Project2D();

    u32 * texture_mem = tiny3d_AllocTexture(64*1024*1024);

    u32 * texture_pointer;

    if(!texture_mem) exit(0); 

    texture_pointer = texture_mem;

    ResetFont();
   // texture_pointer = (u32 *) AddFontFromBitmapArray((u8 *) font  , (u8 *) texture_pointer, 32, 255, 16, 32, 2, BIT0_FIRST_PIXEL);
	
	TTFLoadFont(NULL, (void *) PS_ttf_bin, PS_ttf_bin_size);
    texture_pointer = (u32 *) AddFontFromTTF((u8 *) texture_pointer, 32, 255, 32, 32, TTF_to_Bitmap);
    TTFUnloadFont();

    int videoscale_x = 0;
    int videoscale_y = 0;

    double sx = (double) Video_Resolution.width;
    double sy = (double) Video_Resolution.height;
    double psx = (double) (1000 + videoscale_x)/1000.0;
    double psy = (double) (1000 + videoscale_y)/1000.0;
    
    tiny3d_UserViewport(1, 
        (float) ((sx - sx * psx) / 2.0), 
        (float) ((sy - sy * psy) / 2.0), 
        (float) ((sx * psx) / 848.0),
        (float) ((sy * psy) / 512.0),
        (float) ((sx / 1920.0) * psx), 
        (float) ((sy / 1080.0) * psy));
}

//*******************************************************
//Main
//*******************************************************

int main(void)
{
	ioPadInit(7);
	ioPadSetSensorMode(0,1);
	ioPadSetPressMode(0,1);
	padActParam actparam;
	
	Init_Graph();
	int y_max=512,x_max = 848;
	int x_center = 0;
	int x_L3,y_L3, x_R3, y_R3;
	while(1) {
		tiny3d_Flip();
		cls();
		ps3pad_read();
		int x = 0, y=50;
		SetFontAutoCenter(1);
		SetFontSize(20,20);
		SetFontColor(0xffffffff, 0x0);
		DrawString(x, y, "GamePad Test");
		SetFontAutoCenter(0);
		
		DrawFormatString(50, 100 , "X= %d", paddata.SENSOR_X);
		DrawFormatString(50, 130 , "Y= %d", paddata.SENSOR_Y);
		DrawFormatString(50, 160, "Z= %d", paddata.SENSOR_Z);
		DrawFormatString(50, 190, "G= %d", paddata.SENSOR_G);
		
		if (old_pad  & BUTTON_L3 && old_pad  & BUTTON_R2) actparam.large_motor = paddata.PRE_R2; else actparam.large_motor = 0;
		if (old_pad  & BUTTON_L3 && old_pad  & BUTTON_R3) actparam.small_motor = 1; else actparam.small_motor = 0;
		ioPadSetActDirect(0, &actparam);
		
		y=y_max/2; x= x_center;
		if (old_pad  & BUTTON_LEFT) SetFontColor(0xffffffff, 0x00a000ff) ; else SetFontColor(0xffffffff, 0x0);
		DrawPressure(x-6, y+25, paddata.PRE_LEFT);
		x= DrawString(x, y, " } ");
		y-=30;
		if (old_pad  & BUTTON_UP) SetFontColor(0xffffffff, 0x00a000ff) ; else SetFontColor(0xffffffff, 0x0);
		DrawString(x, y, " ~ ");
		DrawPressure(x-6, y+25, paddata.PRE_UP);
		y-=45;
		if (old_pad  & BUTTON_L1) SetFontColor(0xffffffff, 0x00a000ff) ; else SetFontColor(0xffffffff, 0x0);
		DrawString(x, y, " L1 ");
		DrawPressure(x-6, y+25, paddata.PRE_L1);
		y-=30;
		if (old_pad  & BUTTON_L2) SetFontColor(0xffffffff, 0x00a000ff) ; else SetFontColor(0xffffffff, 0x0);
		DrawString(x, y, " L2 ");
		DrawPressure(x-6, y+25, paddata.PRE_L2);
		y=y_max/2 + 30;
		if (old_pad  & BUTTON_DOWN) SetFontColor(0xffffffff, 0x00a000ff) ; else SetFontColor(0xffffffff, 0x0);
		DrawPressure(x-6, y+25, paddata.PRE_DOWN);
		x= DrawString(x, y, " | ");
		y=y_max/2;
		if (old_pad  & BUTTON_RIGHT) SetFontColor(0xffffffff, 0x00a000ff) ; else SetFontColor(0xffffffff, 0x0);
		DrawPressure(x-6, y+25, paddata.PRE_RIGHT);
		x = DrawString(x, y, " { ");
		/*
		y+=120;
		SetFontColor(0xffffffff, 0x0);
		DrawFormatString(x, y, "x= %d", paddata.button[6] - 128 );
		y+=30;
		DrawFormatString(x, y, "y= %d", paddata.button[7] - 128 );
		*/
		y=y_max/2 + 60;
		x_L3 = x + (paddata.button[6] - 128)/5;
		y_L3 = y + (paddata.button[7] - 128)/5;
		if (old_pad  & BUTTON_L3) SetFontColor(0xffffffff, 0x00a000ff) ; else SetFontColor(0xffffffff, 0x0);
		DrawString(x_L3, y_L3, " L3 ");
		y=y_max/2; x+=30;
		if (old_pad  & BUTTON_SELECT) SetFontColor(0xffffffff, 0x00a000ff) ; else SetFontColor(0xffffffff, 0x0);
		x = DrawString(x, y, " SELECT ");
		if(x<x_max/2) x_center = x_max/2 - x;
		x+=25; 
		if (old_pad  & BUTTON_START) SetFontColor(0xffffffff, 0x00a000ff) ; else SetFontColor(0xffffffff, 0x0);
		x=DrawString(x, y, " START ");
		/*
		y+=120;
		SetFontColor(0xffffffff, 0x0);
		DrawFormatString(x, y, "x= %d", paddata.button[4] - 128 );
		y+=30;
		DrawFormatString(x, y, "y= %d", paddata.button[5] - 128 );
		*/
		y=y_max/2 + 60;
		x_R3 = x +(paddata.button[4] - 128)/5;
		y_R3 = y +(paddata.button[5] - 128)/5;
		if (old_pad  & BUTTON_R3) SetFontColor(0xffffffff, 0x00a000ff) ; else SetFontColor(0xffffffff, 0x0);
		DrawString(x_R3, y_R3, " R3 ");
		y=y_max/2; x+=30;
		if (old_pad  & BUTTON_SQUARE) SetFontColor(0xffffffff, 0x00a000ff) ; else SetFontColor(0xffffffff, 0x0);
		DrawPressure(x-6, y+25, paddata.PRE_SQUARE);
		x=DrawString(x , y, " $ ");
		y-=30; 
		if (old_pad  & BUTTON_TRIANGLE) SetFontColor(0xffffffff, 0x00a000ff) ; else SetFontColor(0xffffffff, 0x0);
		DrawString(x, y, " % ");
		DrawPressure(x-6, y+25, paddata.PRE_TRIANGLE);
		y-=45;
		if (old_pad  & BUTTON_R1) SetFontColor(0xffffffff, 0x00a000ff) ; else SetFontColor(0xffffffff, 0x0);
		DrawString(x, y, " R1 ");
		DrawPressure(x-6, y+25, paddata.PRE_R1);
		y-=30;
		if (old_pad  & BUTTON_R2) SetFontColor(0xffffffff, 0x00a000ff) ; else SetFontColor(0xffffffff, 0x0);
		DrawString(x, y, " R2 ");
		DrawPressure(x-6, y+25, paddata.PRE_R2);
		y=y_max/2+30;
		if (old_pad  & BUTTON_CROSS) SetFontColor(0xffffffff, 0x00a000ff) ; else SetFontColor(0xffffffff, 0x0);
		DrawPressure(x-6, y+25, paddata.PRE_CROSS);
		x=DrawString(x, y, " & ");
		y=y_max/2;
		if (old_pad  & BUTTON_CIRCLE) SetFontColor(0xffffffff, 0x00a000ff) ; else SetFontColor(0xffffffff, 0x0);
		DrawString(x, y, " # ");
		DrawPressure(x-6, y+25, paddata.PRE_CIRCLE);
	}
	return 0;
}
