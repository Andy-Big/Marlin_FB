#include "fst_spi.h"



uint8_t					_touch_buff[TOUCH_BUFF_SIZE];

FastSpi   			fstspi;

bool						FastSpi::inited = false;
fst_mode_t			FastSpi::current_mode = FST_MODE_UNKNOW;



// SPI2 interrupt.
extern "C" void SPI2_IRQHandler(void)
{
	HAL_SPI_IRQHandler(&fstspi.hFstSpi);
}

// SPI2 RX DMA interrupt.
#ifdef STM32F1
	extern "C" void DMA1_Channel4_IRQHandler(void)
	{
		HAL_DMA_IRQHandler(&fstspi.hFstSpiDmaRx);
	}
#endif
#ifdef STM32F4
	extern "C" void DMA1_Stream3_IRQHandler(void)
	{
		HAL_DMA_IRQHandler(&fstspi.hFstSpiDmaRx);
	}
#endif

// SPI2 TX DMA interrupt.
#ifdef STM32F1
	extern "C" void DMA1_Channel5_IRQHandler(void)
	{
		HAL_DMA_IRQHandler(&fstspi.hFstSpiDmaTx);
	}
#endif
#ifdef STM32F4
	extern "C" void DMA1_Stream4_IRQHandler(void)
	{
		HAL_DMA_IRQHandler(&fstspi.hFstSpiDmaTx);
	}
#endif

void 		HAL_SPI_TxRxCpltCallback(SPI_HandleTypeDef *hspi)
{
	while ((fstspi.GetFlags() & SPI_FLAG_BSY) || (fstspi.GetFlags() & SPI_FLAG_TXE) == 0 || fstspi.hFstSpi.State != HAL_SPI_STATE_READY);
	while (fstspi.IsDMAReady() == 0);
	_touch_CS_Disable();
	_flash_CS_Disable();
}















uint16_t	FastSpi::GetFlags()
{
	return hFstSpi.Instance->SR;
}
//==============================================================================




void		FastSpi::Init(void)
{
	if (inited)
		return;

	GPIO_InitTypeDef GPIO_InitStruct = {0};
	FST_SPI_CLK_ENABLE();

	// TOUCH_SPI GPIO Configuration    
	#ifdef STM32F1
		GPIO_InitStruct.Pin = FST_SPI_MISO_Pin;
		GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
		GPIO_InitStruct.Pull = GPIO_NOPULL;
		GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
//		GPIO_InitStruct.Alternate = FST_SPI_GPIO_ALTERNATE;
		HAL_GPIO_Init(FST_SPI_MISO_GPIO, &GPIO_InitStruct);
	#endif
	#ifdef STM32F4
		GPIO_InitStruct.Pin = FST_SPI_MISO_Pin;
		GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
		GPIO_InitStruct.Pull = GPIO_NOPULL;
		GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
		GPIO_InitStruct.Alternate = FST_SPI_GPIO_ALTERNATE;
		HAL_GPIO_Init(FST_SPI_MISO_GPIO, &GPIO_InitStruct);
	#endif

	GPIO_InitStruct.Pin = FST_SPI_MOSI_Pin;
	HAL_GPIO_Init(FST_SPI_MOSI_GPIO, &GPIO_InitStruct);

	GPIO_InitStruct.Pin = FST_SPI_SCK_Pin;
	HAL_GPIO_Init(FST_SPI_SCK_GPIO, &GPIO_InitStruct);

	// Touch CS
	_touch_CS_Disable();
	GPIO_InitStruct.Pin = FST_SPI_CS_TOUCH_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(FST_SPI_CS_TOUCH_GPIO, &GPIO_InitStruct);


	// FLASH CS
	GPIO_InitStruct.Pin = FST_SPI_CS_FLASH_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(FST_SPI_CS_FLASH_GPIO, &GPIO_InitStruct);


	// Prepare SPI for TOUCH operations
	hFstSpi.Instance = FST_SPI;
	hFstSpi.Init.Mode = SPI_MODE_MASTER;
	hFstSpi.Init.Direction = SPI_DIRECTION_2LINES;
	hFstSpi.Init.DataSize = SPI_DATASIZE_8BIT;
	hFstSpi.Init.CLKPolarity = SPI_POLARITY_LOW;
	hFstSpi.Init.CLKPhase = SPI_PHASE_1EDGE;
	hFstSpi.Init.NSS = SPI_NSS_SOFT;
	hFstSpi.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_32;
	hFstSpi.Init.FirstBit = SPI_FIRSTBIT_MSB;
	hFstSpi.Init.TIMode = SPI_TIMODE_DISABLE;
	hFstSpi.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
	hFstSpi.Init.CRCPolynomial = 10;
	FST_SPI_CLK_ENABLE();
	HAL_SPI_Init(&hFstSpi);

	inited = true;
	current_mode = FST_MODE_UNKNOW;
}
//==============================================================================




HAL_SPI_StateTypeDef	FastSpi::GetState()
{
	return hFstSpi.State;
}
//==============================================================================




// prescaler - SPI_BAUDRATEPRESCALER_2/4/8/16/32/64/128/256
// APB2 clock is 84Mhz
void		FastSpi::SetSpeed(uint16_t prescaler)
{
	// Clear baudrate bits
	hFstSpi.Instance->CR1 &= ~(SPI_CR1_BR_2 | SPI_CR1_BR_1 | SPI_CR1_BR_0);
	// Set new baudrate bits
	hFstSpi.Instance->CR1 |= prescaler;	// Set SPI speed
}
//==============================================================================




// return baudrate bits value - SPI_BAUDRATEPRESCALER_2/4/8/16/32/64/128/256
uint16_t		FastSpi::GetSpeed()
{
	return hFstSpi.Instance->CR1 & (SPI_CR1_BR_2 | SPI_CR1_BR_1 | SPI_CR1_BR_0);
}
//==============================================================================












void		FastSpi::TouchEnable()
{
	if (!inited)
		Init();

	if (current_mode == FST_MODE_TOUCH)
		return;

	FlashDisable();

	while ((GetFlags() & SPI_FLAG_BSY) || (GetFlags() & SPI_FLAG_TXE) == 0 || hFstSpi.State != HAL_SPI_STATE_READY);
	while ( IsDMAReady() == 0);

	// DMA
	FST_SPI_DMA_CLK_ENABLE();

	// Prepare SPI for Touch
	hFstSpi.Instance = FST_SPI;
	hFstSpi.Init.Mode = SPI_MODE_MASTER;
	hFstSpi.Init.Direction = SPI_DIRECTION_2LINES;
	hFstSpi.Init.DataSize = SPI_DATASIZE_8BIT;
	hFstSpi.Init.CLKPolarity = SPI_POLARITY_LOW;
	hFstSpi.Init.CLKPhase = SPI_PHASE_1EDGE;
	hFstSpi.Init.NSS = SPI_NSS_SOFT;
	hFstSpi.Init.BaudRatePrescaler = FST_SPI_LOW_CLOCKSPEED;
	hFstSpi.Init.FirstBit = SPI_FIRSTBIT_MSB;
	hFstSpi.Init.TIMode = SPI_TIMODE_DISABLE;
	hFstSpi.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
	hFstSpi.Init.CRCPolynomial = 10;
	FST_SPI_CLK_ENABLE();
	HAL_SPI_Init(&hFstSpi);

	#ifdef STM32F1
		hFstSpiDmaRx.Instance = FST_SPI_DMA_CHANNEL_RX;
		hFstSpiDmaRx.Init.Direction = DMA_PERIPH_TO_MEMORY;
		hFstSpiDmaRx.Init.PeriphInc = DMA_PINC_DISABLE;
		hFstSpiDmaRx.Init.MemInc = DMA_MINC_ENABLE;
		hFstSpiDmaRx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
		hFstSpiDmaRx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
		hFstSpiDmaRx.Init.Mode = DMA_NORMAL;
		hFstSpiDmaRx.Init.Priority = DMA_PRIORITY_LOW;
	#endif
	#ifdef STM32F4
		hFstSpiDmaRx.Instance = FST_SPI_DMA_STREAM_RX;
		hFstSpiDmaRx.Init.Channel = FST_SPI_DMA_CHANNEL_RX;
		hFstSpiDmaRx.Init.Direction = DMA_PERIPH_TO_MEMORY;
		hFstSpiDmaRx.Init.PeriphInc = DMA_PINC_DISABLE;
		hFstSpiDmaRx.Init.MemInc = DMA_MINC_ENABLE;
		hFstSpiDmaRx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
		hFstSpiDmaRx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
		hFstSpiDmaRx.Init.Mode = DMA_NORMAL;
		hFstSpiDmaRx.Init.Priority = DMA_PRIORITY_LOW;
		hFstSpiDmaRx.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
	#endif
		HAL_DMA_Init(&hFstSpiDmaRx);   
		__HAL_LINKDMA(&hFstSpi, hdmarx, hFstSpiDmaRx);

	#ifdef STM32F1
		hFstSpiDmaTx.Instance = FST_SPI_DMA_CHANNEL_TX;
		hFstSpiDmaTx.Init.Direction = DMA_MEMORY_TO_PERIPH;
		hFstSpiDmaTx.Init.PeriphInc = DMA_PINC_DISABLE;
		hFstSpiDmaTx.Init.MemInc = DMA_MINC_ENABLE;
		hFstSpiDmaTx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
		hFstSpiDmaTx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
		hFstSpiDmaTx.Init.Mode = DMA_NORMAL;
		hFstSpiDmaTx.Init.Priority = DMA_PRIORITY_LOW;
	#endif
	#ifdef STM32F4
		hFstSpiDmaTx.Instance = FST_SPI_DMA_STREAM_TX;
		hFstSpiDmaTx.Init.Channel = FST_SPI_DMA_CHANNEL_TX;
		hFstSpiDmaTx.Init.Direction = DMA_MEMORY_TO_PERIPH;
		hFstSpiDmaTx.Init.PeriphInc = DMA_PINC_DISABLE;
		hFstSpiDmaTx.Init.MemInc = DMA_MINC_ENABLE;
		hFstSpiDmaTx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
		hFstSpiDmaTx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
		hFstSpiDmaTx.Init.Mode = DMA_NORMAL;
		hFstSpiDmaTx.Init.Priority = DMA_PRIORITY_LOW;
		hFstSpiDmaTx.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
	#endif
	HAL_DMA_Init(&hFstSpiDmaTx);
	__HAL_LINKDMA(&hFstSpi, hdmatx, hFstSpiDmaTx);

	// DMA interrupt init
	// DMA1_Stream3_IRQn interrupt configuration
	HAL_NVIC_SetPriority(FST_SPI_DMA_STREAM_RX_IRQ, 0, 0);
	HAL_NVIC_EnableIRQ(FST_SPI_DMA_STREAM_RX_IRQ);

	// DMA1_Stream4_IRQn interrupt configuration
	HAL_NVIC_SetPriority(FST_SPI_DMA_STREAM_TX_IRQ, 0, 0);
	HAL_NVIC_EnableIRQ(FST_SPI_DMA_STREAM_TX_IRQ);

	hFstSpi.Instance->CR1 |= SPI_CR1_SPE;

	current_mode = FST_MODE_TOUCH;
}
//==============================================================================




void		FastSpi::TouchDisable()
{
	if (current_mode != FST_MODE_TOUCH)
		return;

	while ((GetFlags() & SPI_FLAG_BSY) || (GetFlags() & SPI_FLAG_TXE) == 0 || hFstSpi.State != HAL_SPI_STATE_READY);
	while ( IsDMAReady() == 0);
	_touch_CS_Disable();

	HAL_DMA_DeInit(hFstSpi.hdmarx);
	HAL_DMA_DeInit(hFstSpi.hdmatx);
	
	current_mode = FST_MODE_UNKNOW;
}
//==============================================================================




bool		FastSpi::TouchStartRead()
{
	if (((GetFlags() & SPI_FLAG_BSY) || (GetFlags() & SPI_FLAG_TXE) == 0 || hFstSpi.State != HAL_SPI_STATE_READY) ||  IsDMAReady() == 0)
		return false;
	if (current_mode != FST_MODE_TOUCH)
	{
		 TouchEnable();
	}
	_touch_CS_Enable();
	memset(_touch_buff, 0, TOUCH_BUFF_SIZE);

	for (uint8_t i = 0; i < TOUCH_BUFF_SIZE; i+=6)
		_touch_buff[i] = 0x90;
	for (uint8_t i = 3; i < TOUCH_BUFF_SIZE; i+=6)
		_touch_buff[i] = 0xD0;

	HAL_SPI_TransmitReceive_DMA(&hFstSpi, _touch_buff, _touch_buff, sizeof(_touch_buff));
	return true;
}
//==============================================================================

















void		FastSpi::FlashEnable()
{
	if (!inited)
		Init();
	
	if (current_mode == FST_MODE_FLASH)
		return;

	TouchDisable();

	while ((GetFlags() & SPI_FLAG_BSY) || (GetFlags() & SPI_FLAG_TXE) == 0 || hFstSpi.State != HAL_SPI_STATE_READY);
	while ( IsDMAReady() == 0);
	// Prepare SPI for Flash
	hFstSpi.Instance = FST_SPI;
	hFstSpi.Init.Mode = SPI_MODE_MASTER;
	hFstSpi.Init.Direction = SPI_DIRECTION_2LINES;
	hFstSpi.Init.DataSize = SPI_DATASIZE_8BIT;
	hFstSpi.Init.CLKPolarity = SPI_POLARITY_HIGH;
	hFstSpi.Init.CLKPhase = SPI_PHASE_2EDGE;
	hFstSpi.Init.NSS = SPI_NSS_SOFT;
	hFstSpi.Init.BaudRatePrescaler = FST_SPI_HIGH_CLOCKSPEED;
	hFstSpi.Init.FirstBit = SPI_FIRSTBIT_MSB;
	hFstSpi.Init.TIMode = SPI_TIMODE_DISABLE;
	hFstSpi.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
	hFstSpi.Init.CRCPolynomial = 10;
	FST_SPI_CLK_ENABLE();
	HAL_SPI_Init(&hFstSpi);

	// DMA
	FST_SPI_DMA_CLK_ENABLE();

	// SPI1_RX Init
	#ifdef STM32F1
		hFstSpiDmaRx.Instance = FST_SPI_DMA_CHANNEL_RX;
		hFstSpiDmaRx.Init.Direction = DMA_PERIPH_TO_MEMORY;
		hFstSpiDmaRx.Init.PeriphInc = DMA_PINC_DISABLE;
		hFstSpiDmaRx.Init.MemInc = DMA_MINC_ENABLE;
		hFstSpiDmaRx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
		hFstSpiDmaRx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
		hFstSpiDmaRx.Init.Mode = DMA_NORMAL;
		hFstSpiDmaRx.Init.Priority = DMA_PRIORITY_LOW;
	#endif
	#ifdef STM32F4
		hFstSpiDmaRx.Instance = FST_SPI_DMA_STREAM_RX;
		hFstSpiDmaRx.Init.Channel = FST_SPI_DMA_CHANNEL_RX;
		hFstSpiDmaRx.Init.Direction = DMA_PERIPH_TO_MEMORY;
		hFstSpiDmaRx.Init.PeriphInc = DMA_PINC_DISABLE;
		hFstSpiDmaRx.Init.MemInc = DMA_MINC_ENABLE;
		hFstSpiDmaRx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
		hFstSpiDmaRx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
		hFstSpiDmaRx.Init.Mode = DMA_NORMAL;
		hFstSpiDmaRx.Init.Priority = DMA_PRIORITY_LOW;
		hFstSpiDmaRx.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
	#endif
	HAL_DMA_Init(&hFstSpiDmaRx);
	__HAL_LINKDMA(&hFstSpi, hdmarx, hFstSpiDmaRx);

	// SPI1_TX Init
	#ifdef STM32F1
		hFstSpiDmaTx.Instance = FST_SPI_DMA_CHANNEL_TX;
		hFstSpiDmaTx.Init.Direction = DMA_MEMORY_TO_PERIPH;
		hFstSpiDmaTx.Init.PeriphInc = DMA_PINC_DISABLE;
		hFstSpiDmaTx.Init.MemInc = DMA_MINC_ENABLE;
		hFstSpiDmaTx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
		hFstSpiDmaTx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
		hFstSpiDmaTx.Init.Mode = DMA_NORMAL;
		hFstSpiDmaTx.Init.Priority = DMA_PRIORITY_LOW;
	#endif
	#ifdef STM32F4
		hFstSpiDmaTx.Instance = FST_SPI_DMA_STREAM_TX;
		hFstSpiDmaTx.Init.Channel = FST_SPI_DMA_CHANNEL_TX;
		hFstSpiDmaTx.Init.Direction = DMA_MEMORY_TO_PERIPH;
		hFstSpiDmaTx.Init.PeriphInc = DMA_PINC_DISABLE;
		hFstSpiDmaTx.Init.MemInc = DMA_MINC_ENABLE;
		hFstSpiDmaTx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
		hFstSpiDmaTx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
		hFstSpiDmaTx.Init.Mode = DMA_NORMAL;
		hFstSpiDmaTx.Init.Priority = DMA_PRIORITY_LOW;
		hFstSpiDmaTx.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
	#endif
	HAL_DMA_Init(&hFstSpiDmaTx);
	__HAL_LINKDMA(&hFstSpi, hdmatx, hFstSpiDmaTx);

	// DMA interrupt init
	// DMA2_Stream2_IRQn interrupt configuration
	HAL_NVIC_SetPriority(FST_SPI_DMA_STREAM_RX_IRQ, 0, 0);
	HAL_NVIC_EnableIRQ(FST_SPI_DMA_STREAM_RX_IRQ);
	// DMA2_Stream5_IRQn interrupt configuration
	HAL_NVIC_SetPriority(FST_SPI_DMA_STREAM_TX_IRQ, 0, 0);
	HAL_NVIC_EnableIRQ(FST_SPI_DMA_STREAM_TX_IRQ);

	hFstSpi.Instance->CR1 |= SPI_CR1_SPE;

	current_mode = FST_MODE_FLASH;
}
//==============================================================================




void		FastSpi::FlashDisable()
{
	if (current_mode != FST_MODE_FLASH)
		return;
	
	while ((GetFlags() & SPI_FLAG_BSY) || (GetFlags() & SPI_FLAG_TXE) == 0 || hFstSpi.State != HAL_SPI_STATE_READY);
	while ( IsDMAReady() == 0);

	/* SPI1 DMA DeInit */
	HAL_DMA_DeInit(hFstSpi.hdmarx);
	HAL_DMA_DeInit(hFstSpi.hdmatx);

	current_mode = FST_MODE_UNKNOW;
}
//==============================================================================




uint8_t		FastSpi::FlashWriteReadByte(uint8_t txval)
{
	if (current_mode != FST_MODE_FLASH)
	{
		 FlashEnable();
	}

	uint16_t res = 0;

	while ((GetFlags() & SPI_FLAG_BSY) || (GetFlags() & SPI_FLAG_TXE) == 0);
	hFstSpi.Instance->DR = txval;
	while ((GetFlags() & SPI_FLAG_RXNE) ==  0);
	res = hFstSpi.Instance->DR;
/*
	while ((_flashGetFlags() & SPI_FLAG_BSY) || !(_flashGetFlags() & SPI_FLAG_TXE));
	hFstSpi.Instance->DR = txval;
	while ((_flashGetFlags() & SPI_FLAG_RXNE) ==  0);
	res = hFstSpi.Instance->DR;
*/
	return (uint8_t)res;
}
//==============================================================================




void		FastSpi::FlashReadBuff(uint32_t dlen, uint8_t *dbuff)
{
	if (current_mode != FST_MODE_FLASH)
	{
		 FlashEnable();
	}

	uint32_t	rlen = 0;
	uint8_t		*rbuff = dbuff;

	while ((GetFlags() & SPI_FLAG_BSY) || !(GetFlags() & SPI_FLAG_TXE));
	*rbuff = hFstSpi.Instance->DR & 0x00FF;
	while (rlen < dlen)
	{
		hFstSpi.Instance->DR = 0;
		while (!(GetFlags() & SPI_FLAG_RXNE));
		*rbuff = hFstSpi.Instance->DR & 0x00FF;
		rbuff++;
		rlen++;
	}

	return;
}
//==============================================================================




void		FastSpi::FlashWriteBuff(uint32_t dlen, uint8_t *dbuff)
{
	if (current_mode != FST_MODE_FLASH)
	{
		 FlashEnable();
	}

	uint32_t	rlen = 0;
	uint8_t		*rbuff = dbuff;

	while ((GetFlags() & SPI_FLAG_BSY) || !(GetFlags() & SPI_FLAG_TXE));
	*rbuff = hFstSpi.Instance->DR & 0x00FF;
	while (rlen < dlen)
	{
		hFstSpi.Instance->DR = *rbuff;
		while (!(GetFlags() & SPI_FLAG_TXE));
		rbuff++;
		rlen++;
	}

	return;
}
//==============================================================================




void		FastSpi::FlashReadBuffDMA(uint32_t dlen, uint8_t *dbuff)
{
	if (current_mode != FST_MODE_FLASH)
	{
		 FlashEnable();
	}

	while ((GetFlags() & SPI_FLAG_BSY) || (GetFlags() & SPI_FLAG_TXE) == 0 || hFstSpi.State != HAL_SPI_STATE_READY);
	HAL_SPI_Receive_DMA(&hFstSpi, dbuff, dlen);
	while (hFstSpi.State != HAL_SPI_STATE_READY);
	return;
}
//==============================================================================




void		FastSpi::FlashWriteBuffDMA(uint32_t dlen, uint8_t *dbuff)
{
	if (current_mode != FST_MODE_FLASH)
	{
		 FlashEnable();
	}

	while ((GetFlags() & SPI_FLAG_BSY) || (GetFlags() & SPI_FLAG_TXE) == 0 || hFstSpi.State != HAL_SPI_STATE_READY);
	HAL_SPI_Transmit_DMA(&hFstSpi, dbuff, dlen);
//	while (hFstSpi.State != HAL_SPI_STATE_READY);
	return;
}
//==============================================================================




