#ifndef __touch_H
#define __touch_H


#include "../../../src/MarlinCore.h"
#include "fst_spi.h"


#define		TOUCH_READS			(uint8_t)16


typedef enum
{
	TS_FREE = 0,
	TS_WORKED = 1,
	TS_PREPRESSED = 2,
	TS_SPRESSED,
	TS_LPRESSED,
	TS_SRELEASED,
	TS_LRELEASED,
} touch_state_t;

typedef struct
{
	touch_state_t	state;
	touch_state_t	prevstate;
	uint8_t			time;
	uint16_t		xc;
	uint16_t		yc;
	uint16_t		xc_raw;
	uint16_t		yc_raw;
	uint16_t		x_min;
	uint16_t		x_max;
	uint16_t		y_min;
	uint16_t		y_max;

} touch_info_t;

typedef struct
{
	uint16_t		xc;
	uint16_t		yc;
} touch_point_t;

extern uint8_t				_touch_buff[TOUCH_BUFF_SIZE];

class SpiTouch
{
	private:
		touch_info_t		touch_info;
		touch_point_t		touch_point;
		uint32_t				last_read_ms = 0;

		FORCE_INLINE bool TOUCH_PRESSED()	{ return (touch_point.xc || touch_point.yc); };
		void 			_RefreshState();

	public:
		void				_ReadCoords();

		void				Init(void);
		void				Enable();
		void				Disable();

		touch_state_t	GetState();
		void				SetWorked(touch_state_t state);
		void				GetCurrentCoords(touch_point_t *pt);
		void				GetCurrentCoordsRaw(touch_point_t *pt);
		void				GetLastCoords(touch_point_t *pt);
		void				GetLastCoordsRaw(touch_point_t *pt);
		uint8_t		IsPressed();


		bool				getRawPoint(int16_t *x, int16_t *y);
};

extern SpiTouch touchspi;

#endif /*__touch_H */
