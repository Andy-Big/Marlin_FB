#ifndef __spi_flash_H
#define __spi_flash_H



#include "../../../src/MarlinCore.h"
#include "fst_spi.h"
#include "../fatfs/ff.h"
#include "../fatfs/diskio.h"
#include "../fatfs/fatfs_shared.h"

#define	ZB25VQ_INSTEAD_W25Q

#define W25Q_FLAG_BUSY					(uint32_t)(1 << 0)

#define W25Q_STATUS1READ_CMD		(uint8_t)0x05
#define W25Q_STATUS1WRITE_CMD		(uint8_t)0x01
#define W25Q_STATUS2READ_CMD		(uint8_t)0x35
#define W25Q_STATUS2WRITE_CMD		(uint8_t)0x31
#define W25Q_STATUS3READ_CMD		(uint8_t)0x15
#define W25Q_STATUS3WRITE_CMD		(uint8_t)0x11
#define W25Q_READID_CMD					(uint8_t)0x9F
#define W25Q_READDATA_CMD				(uint8_t)0x03
#define W25Q_WRITEENABLE_CMD		(uint8_t)0x06
#define W25Q_ERASESECTOR_CMD		(uint8_t)0x20
#define W25Q_PROGRAMPAGE_CMD		(uint8_t)0x02


typedef struct
{
	uint32_t	sector_size;
	uint32_t	sectors_count;
	uint32_t	page_size;
} w25q_info_t;

extern FIL    filFlashFile;

class	W25Q_storage
{
	private:
		w25q_info_t	_info = {0, 0};
		uint32_t		_bust_counts;
		static inline void		_wait_cs() { volatile uint8_t i = 100; while (i)	i--; };


		uint32_t		_ReadStatus();
		void				_WriteStatus(uint32_t val);
		void				_WaitBusy();
		void				_WriteEnable();

	public:
		bool				Init();
		FORCE_INLINE uint32_t		GetSectorSize() {
            return _info.sector_size;
          };
		FORCE_INLINE uint32_t		GetSectorsCount() {
          if (_info.sectors_count > 100)
            return _info.sectors_count;
          else
            return 0;
          };
;
		uint32_t		ReadID();
		void				ReadBuff(uint32_t addr, uint32_t dlen, uint8_t *dbuff);
		void				ReadBuffDMA(uint32_t addr, uint32_t dlen, uint8_t *dbuff);
		void				EraseSector(uint32_t addr);
		void				WriteBuff(uint32_t addr, uint32_t dlen, uint8_t *dbuff, bool erase = true);

		bool				InitFS();
};

extern W25Q_storage	spiflash;


#endif /*__spi_flash_H */

