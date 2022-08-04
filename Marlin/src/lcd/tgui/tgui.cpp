/*******************************************************************************
*
*     Base user interface for LCD with touch
*
********************************************************************************/

#include "tgui.h"
#include "../tft/tft.h"
#include "../../MarlinCore.h"
#include "../marlinui.h"


volatile uint8_t 		__attribute__ ((aligned (4))) tguiDBuff[UIDBUFF_SIZE]; // __attribute__ ((section (".ccmram")));
volatile uint8_t		tguiFBuff[UIFBUFF_SIZE] __attribute__ ((aligned (4))) ; // __attribute__ ((section (".ccmram")));
char				tfname[512];




void		_tgui_DrawFileRawImg(FIL *file, int16_t x, int16_t y, uint16_t bwidth, uint16_t bheight, uint32_t imgbase, uint8_t fliprows)
{
	DWORD		freaded = imgbase;
	UINT		readed = 0;
	uint16_t	bufinpos = UIFBUFF_SIZE;
	uint16_t	bufoutpos = 0;
	uint8_t		len = 0;
	uint16_t	val = 0;
	uint8_t		curbuffnum = 0;
	uint16_t	*curbuff = (uint16_t*)tguiDBuff;
	volatile  uint32_t totalpix = 0;
	
	if (fliprows == 1)
	{
		tft.flip_x(1);
		y = TFT_HEIGHT - y - bheight;
	}

	tft.set_window(x, y, x+bwidth-1, y+bheight-1);
	

	do
	{
		if (bufinpos == UIFBUFF_SIZE)
		{
			if (f_read(file, (uint8_t*)tguiFBuff, UIFBUFF_SIZE, &readed) != FR_OK)
			{
				break;
			}
			bufinpos = 0;
		}
		
		len = tguiFBuff[bufinpos];
		if (len & 0x80)
		{
			len = (len & 0x7F) + 1;
			if ((bufoutpos + (len + 1)) > (UIDBUFF_SIZE / 4) )
			{
//				LCD_WaitDMAReady();
				tft.write_sequence(curbuff, bufoutpos);
				totalpix += bufoutpos;
				if ((uint32_t)curbuff == (uint32_t)tguiDBuff)
				{
					curbuff = (uint16_t*)(tguiDBuff + (UIDBUFF_SIZE / 2));
					curbuffnum = 1;
				}
				else
				{
					curbuff = (uint16_t*)(tguiDBuff);
					curbuffnum = 0;
				}
				bufoutpos = 0;
			}
			if ((bufinpos + 4) > UIFBUFF_SIZE)
			{
				freaded += bufinpos;
				f_lseek(file, freaded);
				bufinpos = UIFBUFF_SIZE;
				continue;
			}
			bufinpos++;
			val = ((uint16_t)tguiFBuff[bufinpos+1] << 8) + tguiFBuff[bufinpos];
//			val = *(uint16_t*)(tguiFBuff + bufinpos);
			for (uint8_t i = 0; i < len; i++)
			{
				*(curbuff + bufoutpos) = val;
				bufoutpos++;
			}
			bufinpos += 2;
		}
		else
		{
			len++;
			if ((bufoutpos + (len + 1)) > (UIDBUFF_SIZE / 4))
			{
//				LCD_WaitDMAReady();
//				for (uint32_t i = 0; i < bufoutpos; i++)
//					tft.write_pixel(curbuff[i]);
				tft.write_sequence(curbuff, bufoutpos);
				totalpix += bufoutpos;
				if (curbuffnum == 0)
				{
					curbuff = (uint16_t*)(tguiDBuff + (UIDBUFF_SIZE / 2));
					curbuffnum = 1;
				}
				else
				{
					curbuff = (uint16_t*)(tguiDBuff);
					curbuffnum = 0;
				}
				bufoutpos = 0;
			}
			if ((bufinpos + (len + 1) * 2) > UIFBUFF_SIZE)
			{
				freaded += bufinpos;
				f_lseek(file, freaded);
				bufinpos = UIFBUFF_SIZE;
				continue;
			}
			bufinpos++;
			for (uint8_t i = 0; i < len; i++)
			{
				*(curbuff + bufoutpos) = ((uint16_t)tguiFBuff[bufinpos+1] << 8) + tguiFBuff[bufinpos];
//				*(curbuff + bufoutpos) = *(uint16_t*)(tguiFBuff + bufinpos);
				bufoutpos++;
				bufinpos += 2;
			}
			
		}
	} while (readed == UIFBUFF_SIZE || bufinpos < readed);

	if (bufoutpos > 0)
	{
		tft.write_sequence(curbuff, bufoutpos);
		totalpix += bufoutpos;
	}
	totalpix += 1;

	if (fliprows == 1)
	{
		tft.flip_x(0);
	}
	return;
}
//==============================================================================



uint8_t		_tgui_DrawFileCimgBackground(char* file, uint8_t ui)
{
	UINT	readed = 0;
	uint32_t	bwidth = 0;
	uint32_t	bheight = 0;
	uint8_t		fliprows = 0;
	uint32_t	imgbase;

	strcpy(tfname, DISK_FLASH);
	strcat(tfname, (char*)"/");
	if (ui)
	{
		strcat(tfname, TGUI_DIR);
		strcat(tfname, (char*)"/");
		strcat(tfname, TGUI_FPREFIX);
	}
	strcat(tfname, file);

	if (f_open(&filFlashFile, tfname, FA_OPEN_EXISTING | FA_READ) != FR_OK)
	{
		return 0;
	}

	// Reading CIMG header
	if (f_read(&filFlashFile, (uint8_t*)tguiDBuff, 4, &readed) != FR_OK || readed < 4)
	{
		goto closeexit;
	}
	
	// Image width
	bwidth = *(uint16_t*)(tguiDBuff);
	// Image height
	bheight = *(uint16_t*)(tguiDBuff+2);
	if (bheight & 0x8000)
	{
		// If standart row order
		bheight &= 0x7FFF;
	}
	else
	{
		// If flip row order
		fliprows = 1;
	}
	
	
	// Image data offset
	imgbase = 4;

	if (f_lseek(&filFlashFile, imgbase) != FR_OK)
	{
		goto closeexit;
	}

	_tgui_DrawFileRawImg(&filFlashFile, 0, 0, bwidth, bheight, imgbase, fliprows);

closeexit:
	f_close(&filFlashFile);
	return 1;
}
//==============================================================================
/**/



void		_tgui_GetFileCimgSize(char* file, TSIZE *size, uint8_t ui)
{
	UINT	readed = 0;
	
	size->x_size = 0;
	size->y_size = 0;

	strcpy(tfname, DISK_FLASH);
	if (ui)
	{
		strcat(tfname, TGUI_DIR);
		strcat(tfname, (char*)"/");
	}
	strcat(tfname, file);

	if (f_open(&filFlashFile, tfname, FA_OPEN_EXISTING | FA_READ) != FR_OK)
	{
		return;
	}

	// Reading CIMG header
	if (f_read(&filFlashFile, (uint8_t*)tguiDBuff, 4, &readed) != FR_OK || readed < 4)
	{
		f_close(&filFlashFile);
		return;
	}
	
	// Image width
	size->x_size = *(uint16_t*)(tguiDBuff);
	// Image height
	size->y_size = *(uint16_t*)(tguiDBuff+2);
	if (size->y_size & 0x8000)
	{
		// If standart row order
		size->y_size &= 0x7FFF;
	}
	f_close(&filFlashFile);
	return;
}
//==============================================================================



void		TGUI_DrawLogo()
{
	_tgui_DrawFileCimgBackground(TGUI_FNAME_LOGO);
}
//==============================================================================




