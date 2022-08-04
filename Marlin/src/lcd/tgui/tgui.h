#ifndef __tgui_H
#define __tgui_H


#include "../../libs/fst_SPI/spi_flash.h"
#include "../../libs/fatfs/fatfs_shared.h"


#define TGUI_DIR														  (char*)"gui_files"

#define	TGUI_FPREFIX											    (char*)"_tgui_"

#define	TGUI_FNAME_LOGO												(char*)"boot_logo.cimg"


typedef struct
{
	uint16_t	x_size;
	uint16_t	y_size;
} TSIZE;


#define UIDBUFF_SIZE			4096
extern volatile uint8_t		tguiDBuff[UIDBUFF_SIZE];
#define UIFBUFF_SIZE			2048
extern volatile uint8_t		tguiFBuff[UIFBUFF_SIZE];

extern TCHAR			      	tfname[512];


void		      _tgui_DrawFileRawImg(FIL *file, int16_t x, int16_t y, uint16_t bwidth, uint16_t bheight, uint32_t imgbase, uint8_t fliprows);
uint8_t		    _tgui_DrawFileCimgBackground(char* file, uint8_t ui = 1);
void		      _tgui_GetFileCimgSize(char* file, TSIZE *size, uint8_t ui = 1);
void		      TGUI_DrawLogo();




#endif /*__tgui_H */

