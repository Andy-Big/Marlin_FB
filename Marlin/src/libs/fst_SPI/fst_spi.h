#ifndef __fst_spi_H

#define __fst_spi_H







#include "../../../src/MarlinCore.h"







#ifdef USE_HAL_SPI_REGISTER_CALLBACKS

  #undef USE_HAL_SPI_REGISTER_CALLBACKS

#endif

#define USE_HAL_SPI_REGISTER_CALLBACKS	1U




#define FST_SPI					          SPI2

#define FST_SPI_MISO_Pin			    GPIO_PIN_14

#define FST_SPI_MISO_GPIO			    GPIOB

#define FST_SPI_MOSI_Pin			    GPIO_PIN_15

#define FST_SPI_MOSI_GPIO			    GPIOB

#define FST_SPI_SCK_Pin			      GPIO_PIN_10

#define FST_SPI_SCK_GPIO			    GPIOB

#ifdef STM32F4

  #define FST_SPI_GPIO_ALTERNATE	  GPIO_AF5_SPI2

#endif




#define FST_SPI_CS_FLASH_Pin		  GPIO_PIN_12

#define FST_SPI_CS_FLASH_GPIO		  GPIOB




#define FST_SPI_CS_TOUCH_Pin      GPIO_PIN_7

#define FST_SPI_CS_TOUCH_GPIO     GPIOA




#ifdef STM32F1

  #define FST_SPI_DMA_CHANNEL_RX		  DMA1_Channel4

  #define FST_SPI_DMA_STREAM_RX_IRQ	  DMA1_Channel4_IRQn

  #define FST_SPI_DMA_CHANNEL_TX		  DMA1_Channel5

  #define FST_SPI_DMA_STREAM_TX_IRQ	  DMA1_Channel5_IRQn

#endif

#ifdef STM32F4

  #define FST_SPI_DMA_STREAM_RX		    DMA1_Stream3

  #define FST_SPI_DMA_CHANNEL_RX		  DMA_CHANNEL_0

  #define FST_SPI_DMA_STREAM_RX_IRQ	  DMA1_Stream3_IRQn

  #define FST_SPI_DMA_STREAM_TX		    DMA1_Stream4

  #define FST_SPI_DMA_CHANNEL_TX		  DMA_CHANNEL_0

  #define FST_SPI_DMA_STREAM_TX_IRQ	  DMA1_Stream4_IRQn

#endif




#define FST_SPI_CLK_ENABLE()			  __HAL_RCC_SPI2_CLK_ENABLE()

#define FST_SPI_CLK_DISABLE()			  __HAL_RCC_SPI2_CLK_DISABLE()

#define FST_SPI_DMA_CLK_ENABLE()		__HAL_RCC_DMA1_CLK_ENABLE()

#define FST_SPI_DMA_CLK_DISABLE()		__HAL_RCC_DMA1_CLK_DISABLE()




#define FST_SPI_LOW_CLOCKSPEED		  SPI_BAUDRATEPRESCALER_32

#define FST_SPI_HIGH_CLOCKSPEED		  SPI_BAUDRATEPRESCALER_2







#define		_touch_CS_Enable()	FST_SPI_CS_TOUCH_GPIO->BSRR = (uint32_t)FST_SPI_CS_TOUCH_Pin << 16U

#define 	_touch_CS_Disable()	FST_SPI_CS_TOUCH_GPIO->BSRR = FST_SPI_CS_TOUCH_Pin

#define		TOUCH_READS			    (uint8_t)16

#define		TOUCH_BUFF_SIZE		  (uint8_t)6*TOUCH_READS




#define		_flash_CS_Enable()	FST_SPI_CS_FLASH_GPIO->BSRR = (uint32_t)FST_SPI_CS_FLASH_Pin << 16U

#define 	_flash_CS_Disable()	FST_SPI_CS_FLASH_GPIO->BSRR = FST_SPI_CS_FLASH_Pin







typedef enum

{

  FST_MODE_TOUCH = 0,

  FST_MODE_FLASH,

  FST_MODE_UNKNOW,

} fst_mode_t;




void      HAL_SPI_TxRxCpltCallback(SPI_HandleTypeDef *hspi);







class FastSpi

{




  private:

    static fst_mode_t		current_mode;

    static bool         inited;










  public:

    DMA_HandleTypeDef	  hFstSpiDmaRx;

    DMA_HandleTypeDef	  hFstSpiDmaTx;

    SPI_HandleTypeDef		hFstSpi;

    uint16_t	    GetFlags();

    void		      Init(void);

    HAL_SPI_StateTypeDef	GetState();

    void		      SetSpeed(uint16_t prescaler);

    uint16_t	    GetSpeed();

    inline uint8_t		IsDMAReady() { return (hFstSpi.State == HAL_SPI_STATE_READY); }




    void		      TouchEnable();

    void		      TouchDisable();

    bool		      TouchStartRead();










    void		      FlashEnable();

    void		      FlashDisable();




    uint8_t		    FlashWriteReadByte(uint8_t txval);

    void		      FlashReadBuff(uint32_t dlen, uint8_t *dbuff);

    void		      FlashWriteBuff(uint32_t dlen, uint8_t *dbuff);

    void		      FlashReadBuffDMA(uint32_t dlen, uint8_t *dbuff);

    void		      FlashWriteBuffDMA(uint32_t dlen, uint8_t *dbuff);

};




extern FastSpi   fstspi;




#endif /*__fst_spi_H */


