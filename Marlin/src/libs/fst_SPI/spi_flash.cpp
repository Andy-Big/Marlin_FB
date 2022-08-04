#include "spi_flash.h"

FIL           filFlashFile;

W25Q_storage	spiflash;
extern			  SPI_HandleTypeDef	hFstSpi;
volatile      uint32_t		d = 0;










uint32_t		W25Q_storage::_ReadStatus()
{
	uint32_t	retval = 0;
	uint16_t	rxval = 0;

	_wait_cs();
	_flash_CS_Enable();
	fstspi.FlashWriteReadByte(W25Q_STATUS1READ_CMD);
	rxval = fstspi.FlashWriteReadByte(0);
	_flash_CS_Disable();
	retval = rxval;

	_wait_cs();
	_flash_CS_Enable();
	fstspi.FlashWriteReadByte(W25Q_STATUS2READ_CMD);
	rxval = fstspi.FlashWriteReadByte(0);
	_flash_CS_Disable();
	retval += (uint32_t)rxval << 8;

	_wait_cs();
	_flash_CS_Enable();
	fstspi.FlashWriteReadByte(W25Q_STATUS3READ_CMD);
	rxval = fstspi.FlashWriteReadByte(0);
	_flash_CS_Disable();
	retval += (uint32_t)rxval << 16;

	return retval;
}
//==============================================================================




void			W25Q_storage::_WriteStatus(uint32_t val)
{
	uint16_t	txval = 0;

	_wait_cs();
	txval = val & 0xFF;
	_flash_CS_Enable();
	fstspi.FlashWriteReadByte(W25Q_STATUS1WRITE_CMD);
	fstspi.FlashWriteReadByte(txval);
	_flash_CS_Disable();
	txval = (val >> 8) & 0xFF;

	_wait_cs();
	_flash_CS_Enable();
	fstspi.FlashWriteReadByte(W25Q_STATUS2WRITE_CMD);
	fstspi.FlashWriteReadByte(txval);
	_flash_CS_Disable();
	txval = (val >> 16) & 0xFF;

	_wait_cs();
	_flash_CS_Enable();
	fstspi.FlashWriteReadByte(W25Q_STATUS3WRITE_CMD);
	fstspi.FlashWriteReadByte(txval);
	_flash_CS_Disable();

	return;
}
//==============================================================================




void			W25Q_storage::_WaitBusy()
{
	uint16_t	rxval = 0;
	_bust_counts = 0;

	fstspi.FlashEnable();

	while ((fstspi.GetFlags() & SPI_FLAG_BSY) || (fstspi.GetFlags() & SPI_FLAG_TXE) == 0 || fstspi.hFstSpi.State != HAL_SPI_STATE_READY);
	_wait_cs();

	_flash_CS_Enable();
	fstspi.FlashWriteReadByte(W25Q_STATUS1READ_CMD);
	rxval = fstspi.FlashWriteReadByte(0);
	while (rxval & W25Q_FLAG_BUSY)
	{
		d++;
		rxval = fstspi.FlashWriteReadByte(0);
	};
	_flash_CS_Disable();

	return;
}
//==============================================================================




void			W25Q_storage::W25Q_storage::_WriteEnable()
{
	while ((fstspi.GetFlags() & SPI_FLAG_BSY) || (fstspi.GetFlags() & SPI_FLAG_TXE) == 0 || fstspi.hFstSpi.State != HAL_SPI_STATE_READY);
	_WaitBusy();
	_wait_cs();

	_flash_CS_Enable();
	fstspi.FlashWriteReadByte(W25Q_WRITEENABLE_CMD);
	_flash_CS_Disable();

	return;
}
//==============================================================================









bool			W25Q_storage::Init()
{
  bool  res = true;

  SERIAL_ECHOLNPGM("FlasFS: SPI flash init");
	fstspi.Init();
	fstspi.FlashEnable();
	uint32_t f_id = ReadID();
	switch (f_id)
	{
		// w25q32
		case 0xEF4016:
		// zb25vq32
		case 0x5E4016:
			_info.sectors_count = 1024;
			_info.sector_size = 4096;
			_info.page_size = 256;
		  SERIAL_ECHOLNPGM("FlasFS: SPI flash 4 MB found");
			break;

		// w25q64
		case 0xEF4017:
		// zb25vq64
		case 0x5E4017:
			_info.sectors_count = 2048;
			_info.sector_size = 4096;
			_info.page_size = 256;
		  SERIAL_ECHOLNPGM("FlasFS: SPI flash 8 MB found");
			break;

		// w25q128
		case 0xEF4018:
		// w25q128
		case 0x5E4018:
			_info.sectors_count = 4096;
			_info.sector_size = 4096;
			_info.page_size = 256;
		  SERIAL_ECHOLNPGM("FlasFS: SPI flash 16 MB found");
			break;
    
    default:
		  SERIAL_ECHOLNPGM("FlasFS: SPI flash init error");
      res = false;

	}
	_wait_cs();

  return res;
}
//==============================================================================




uint32_t		W25Q_storage::ReadID()
{
	uint32_t	retval = 0;

	while ((fstspi.GetFlags() & SPI_FLAG_BSY) || (fstspi.GetFlags() & SPI_FLAG_TXE) == 0 || fstspi.hFstSpi.State != HAL_SPI_STATE_READY);
	_WaitBusy();
	_wait_cs();

	_flash_CS_Enable();
	fstspi.FlashWriteReadByte(W25Q_READID_CMD);
	retval = fstspi.FlashWriteReadByte(0);
	retval <<= 8;
	retval += fstspi.FlashWriteReadByte(0);
	retval <<= 8;
	retval += fstspi.FlashWriteReadByte(0);

	_flash_CS_Disable();

	return retval;
}
//==============================================================================




void				W25Q_storage::ReadBuff(uint32_t addr, uint32_t dlen, uint8_t *dbuff)
{
	if (dlen == 0 || dbuff == 0)
		return;

	while ((fstspi.GetFlags() & SPI_FLAG_BSY) || (fstspi.GetFlags() & SPI_FLAG_TXE) == 0 || fstspi.hFstSpi.State != HAL_SPI_STATE_READY);
	while (fstspi.IsDMAReady() == 0);
	_WaitBusy();
	_wait_cs();

	_flash_CS_Enable();
	fstspi.FlashWriteReadByte(W25Q_READDATA_CMD);
	fstspi.FlashWriteReadByte((addr >> 16) & 0xFF);
	fstspi.FlashWriteReadByte((addr >> 8) & 0xFF);
	fstspi.FlashWriteReadByte(addr & 0xFF);

	// DMA is not allowed with CCM SRAM (at address 0x10000000)
	if (dlen > 48 && ((uint32_t)dbuff & 0xFF000000) != 0x10000000)
		fstspi.FlashReadBuffDMA(dlen, dbuff);
	else
		fstspi.FlashReadBuff(dlen, dbuff);

	_flash_CS_Disable();

	return;
}
//==============================================================================




void				W25Q_storage::ReadBuffDMA(uint32_t addr, uint32_t dlen, uint8_t *dbuff)
{
	if (dlen == 0 || dbuff == 0)
		return;

	while ((fstspi.GetFlags() & SPI_FLAG_BSY) || (fstspi.GetFlags() & SPI_FLAG_TXE) == 0 || fstspi.hFstSpi.State != HAL_SPI_STATE_READY);
	_WaitBusy();
	_wait_cs();

	_flash_CS_Enable();
	fstspi.FlashWriteReadByte(W25Q_READDATA_CMD);
	fstspi.FlashWriteReadByte((addr >> 16) & 0xFF);
	fstspi.FlashWriteReadByte((addr >> 8) & 0xFF);
	fstspi.FlashWriteReadByte(addr & 0xFF);

	fstspi.FlashReadBuffDMA(dlen, dbuff);

	_flash_CS_Disable();

	return;
}
//==============================================================================



// addr - address of a any byte in sector
void				W25Q_storage::EraseSector(uint32_t addr)
{
	addr &= ~(_info.sector_size - 1);

	while ((fstspi.GetFlags() & SPI_FLAG_BSY) || (fstspi.GetFlags() & SPI_FLAG_TXE) == 0 || fstspi.hFstSpi.State != HAL_SPI_STATE_READY);
	_WriteEnable();	
	_wait_cs();

	_flash_CS_Enable();
	fstspi.FlashWriteReadByte(W25Q_ERASESECTOR_CMD);
	fstspi.FlashWriteReadByte((addr >> 16) & 0xFF);
	fstspi.FlashWriteReadByte((addr >> 8) & 0xFF);
	fstspi.FlashWriteReadByte(addr & 0xFF);
	_flash_CS_Disable();
}
//==============================================================================




void				W25Q_storage::WriteBuff(uint32_t addr, uint32_t dlen, uint8_t *dbuff, bool erase)
{
	if (dlen == 0 || dbuff == 0)
		return;

	uint8_t		*buff = dbuff;

//	uint32_t	sector = addr & ~(_info.sector_size - 1);
//	uint32_t	oldsector = sector;
	// begin address in a sector
	uint32_t	addrinsect = addr & (_info.sector_size - 1);
	// begin address in a page
	uint32_t	addrinpage = addr & (_info.page_size - 1);

	// remain data length in a sector
	uint32_t	towritesect;
	// remain data in a page
	uint32_t	towritepage;

	while ((fstspi.GetFlags() & SPI_FLAG_BSY) || (fstspi.GetFlags() & SPI_FLAG_TXE) == 0 || fstspi.hFstSpi.State != HAL_SPI_STATE_READY);
	_WaitBusy();

	towritesect = dlen;
	if ((towritesect + addrinsect) > _info.sector_size)
		towritesect = _info.sector_size - addrinsect;
	// remain data in a page
	towritepage = towritesect;
	if ((towritepage + addrinpage) > _info.page_size)
		towritepage = _info.page_size - addrinpage;

	addrinpage = 0;
	addrinsect = 0;

	while (dlen)
	{
		// sector erase
		if (erase)
			EraseSector(addr);

		while (towritesect)
		{
			_WaitBusy();
			_wait_cs();

			_WriteEnable();	
			_wait_cs();

			_flash_CS_Enable();
			fstspi.FlashWriteReadByte(W25Q_PROGRAMPAGE_CMD);
			fstspi.FlashWriteReadByte((addr >> 16) & 0xFF);
			fstspi.FlashWriteReadByte((addr >> 8) & 0xFF);
			fstspi.FlashWriteReadByte(addr & 0xFF);
			for (uint32_t i = 0; i < towritepage; i++)
			{
				fstspi.FlashWriteReadByte(*buff);
				buff++;
			}
			_flash_CS_Disable();

			dlen -= towritepage;
			towritesect -= towritepage;
			addr += towritepage;
			// remain data in a page
			towritepage = towritesect;
			if (towritepage > _info.page_size)
				towritepage = _info.page_size;
		}
		towritesect = dlen;
		if (towritesect > _info.sector_size)
			towritesect = _info.sector_size;
		towritepage = towritesect;
		if (towritepage > _info.page_size)
			towritepage = _info.page_size;
	}

	return;
}
//==============================================================================



bool				W25Q_storage::InitFS()
{
  FRESULT res = FR_OK;

  if ((res = f_mount(&FS_flash, DISK_FLASH, 1)) != FR_OK)
  {
    if (res == FR_NO_FILESYSTEM)
    {
		  SERIAL_ECHOLNPGM("FlasFS: filesysytem nor found, creating new");
      if (f_mkfs(DISK_FLASH, NULL, (uint8_t*)fat_buff_copy + W25Q_SECTOR_SIZE_PHYS, W25Q_SECTOR_SIZE_PHYS) != FR_OK)
      {
			  SERIAL_ECHOLNPGM("FlasFS: error creating new filesystem");
        return false;
      }
		  SERIAL_ECHOLNPGM("FlasFS: new filesystem created");
      if (f_mount(&FS_flash, DISK_FLASH, 1) != FR_OK)
      {
			  SERIAL_ECHOLNPGM("FlasFS: error mounting filesystem");
        return false;
      }
    }
    else
    {
		  SERIAL_ECHOLNPGM("FlasFS: error access to FlashFS");
      return false;
    }
	  SERIAL_ECHOLNPGM("FlasFS: FlashFS filesystem mounted success");
  }

	return true;
}
//==============================================================================



