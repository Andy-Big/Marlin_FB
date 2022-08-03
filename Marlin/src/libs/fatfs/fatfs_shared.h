#ifndef FAT_FS_SHARED_H
#define FAT_FS_SHARED_H

#include "ff.h"


#define FF_DEBUG


#define     DISK_SD     "0:"
#define     DISK_FLASH  "1:"


extern FATFS  FS_flash;
extern FATFS FS_sd;


char*		FATFS_GetFileExtension(char *fname);
char*		FATFS_GetFileExtensionUTF(char *fname);
char*   FATFS_GetFilenameFromPath(char *path);
char*   FATFS_GetFilenameFromPathUTF(char *path);
void    FATFS_GetPathFromFilename(char *fname, char *path);
void    FATFS_GetPathFromFilenameUTF(char *fname, char *path);
void    FATFS_DelFileExtension(char *fname);
void		FATFS_DelFileExtensionUTF(char *fname);
char*		FATFS_GetPrevDir(char *fname);

char*		strupper_utf(char *src);
char*		strtrim(char *src);
char*	  strcpy_utf(char *dst, char* src);


#endif