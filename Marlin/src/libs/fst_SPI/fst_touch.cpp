#include "fst_touch.h"




SpiTouch touchspi;



void		SpiTouch::_ReadCoords()
{
	if ((millis() - last_read_ms) < 15)
		return;


	if (fstspi.GetState() != HAL_SPI_STATE_READY)
	{
		touch_point.xc = touch_point.yc = 0;
		return;
	}

	uint32_t	vavg_x = 0, vavg_y = 0;
	uint8_t		*buff = _touch_buff + 1;

	// Calculate X coord
	for (uint8_t i = 0; i < TOUCH_READS; i++)
	{
		vavg_x += (uint16_t)(*buff)<<8;
		buff++;
		vavg_x += *buff;
		buff += 5;
	}
	vavg_x = vavg_x / TOUCH_READS;

	// Calculate Y coord
	buff = _touch_buff + 4;
	for (uint8_t i = 0; i < TOUCH_READS; i++)
	{
		vavg_y += (uint16_t)(*buff)<<8;
		buff++;
		vavg_y += *buff;
		buff += 5;
	}
	vavg_y = vavg_y / TOUCH_READS;

	touch_info.xc_raw = vavg_x;
	touch_info.yc_raw = vavg_y;

	// is touched?
	if (vavg_y < touch_info.y_min || vavg_y > touch_info.y_max || vavg_x < touch_info.x_min || vavg_x > touch_info.x_max)
	{
		touch_point.xc = 0;
		touch_point.yc = 0;
	}
	else
	{
//		if (cfgConfig.display_rotate == 0)
		{
			touch_point.xc = TFT_WIDTH - (vavg_x - touch_info.x_min) * TFT_WIDTH / (touch_info.x_max - touch_info.x_min);
			touch_point.yc = (vavg_y - touch_info.y_min) * TFT_HEIGHT / (touch_info.y_max - touch_info.y_min);
		}
		// else
		// {
		// 	touch_point.xc = (vavg_x - touch_info.x_min) * TFT_WIDTH / (touch_info.x_max - touch_info.x_min);
		// 	touch_point.yc = TFT_HEIGHT - (vavg_y - touch_info.y_min) * TFT_HEIGHT / (touch_info.y_max - touch_info.y_min);
		// }
	}


	// Start new read
	if (fstspi.TouchStartRead())
			last_read_ms = millis();


	_RefreshState();

	return;
}
//==============================================================================




void		SpiTouch::_RefreshState()
{
	switch (touch_info.state)
	{
		case TS_WORKED:
			switch (touch_info.prevstate)
			{
				case TS_SRELEASED:
				case TS_LRELEASED:
					if (TOUCH_PRESSED())
					{
						touch_info.state = TS_PREPRESSED;
						touch_info.time = 0;
					}
					else
						touch_info.state = TS_FREE;
					break;
				case TS_SPRESSED:
					if (TOUCH_PRESSED())
					{
						if (touch_info.time > 100)
						{
							touch_info.state = TS_LPRESSED;
							touch_info.xc = touch_point.xc;
							touch_info.yc = touch_point.yc;
						}
						else
							touch_info.time++;
					}
					else
					{
						touch_info.state = TS_SRELEASED;
						touch_info.time = 0;
					}
					break;
				case TS_LPRESSED:
					if (!TOUCH_PRESSED())
					{
						touch_info.state = TS_LRELEASED;
						touch_info.time = 0;
					}
					break;
				default:
					touch_info.state = TS_FREE;
					break;
			}
			break;
		case TS_FREE:
		case TS_SRELEASED:
		case TS_LRELEASED:
			if (TOUCH_PRESSED())
			{
				touch_info.state = TS_PREPRESSED;
				touch_info.time = 0;
			}
			break;
		case TS_PREPRESSED:
			if (TOUCH_PRESSED())
			{
				if (touch_info.time > 1)
				{
					touch_info.state = TS_SPRESSED;
					touch_info.xc = touch_point.xc;
					touch_info.yc = touch_point.yc;
				}
				else
					touch_info.time++;
			}
			else
			{
				touch_info.state = TS_WORKED;
				touch_info.time = 0;
			}
			break;
		case TS_SPRESSED:
			if (TOUCH_PRESSED())
			{
				if (touch_info.time > 50)
				{
					touch_info.state = TS_LPRESSED;
					touch_info.xc = touch_point.xc;
					touch_info.yc = touch_point.yc;
				}
				else
					touch_info.time++;
			}
			else
			{
				touch_info.state = TS_SRELEASED;
				touch_info.time = 0;
			}
			break;
		case TS_LPRESSED:
			if (!TOUCH_PRESSED())
			{
				touch_info.state = TS_LRELEASED;
				touch_info.time = 0;
			}
			else
				touch_info.time++;
			break;
	}
}
//==============================================================================












void		SpiTouch::Init(void)
{
	fstspi.Init();

	if (TFT_WIDTH == 480)
	{
		touch_info.x_min = 1500;
		touch_info.x_max = 32000;
		touch_info.y_min = 1000;
		touch_info.y_max = 32000;
	}
	else
	{
		touch_info.x_min = 1000;
		touch_info.x_max = 30000;
		touch_info.y_min = 1700;
		touch_info.y_max = 29000;
	}
	touch_info.state = TS_FREE;
	touch_info.time = 0;
	touch_info.xc = 0;
	touch_info.yc = 0;

	touch_point.xc = 0;
	touch_point.yc = 0;
}
//==============================================================================




void		SpiTouch::Enable()
{
	fstspi.TouchEnable();
}
//==============================================================================




void		SpiTouch::Disable()
{
	fstspi.TouchDisable();
}
//==============================================================================




touch_state_t	SpiTouch::GetState()
{
	return touch_info.state;
}
//==============================================================================




void		SpiTouch::SetWorked(touch_state_t state)
{
	// check current state for protect again delayed change
	if (touch_info.state == state)
	{
		touch_info.prevstate = touch_info.state;
		touch_info.state = TS_WORKED;
	}
}
//==============================================================================



/*
void		SpiTouch::GetCurrentCoords(touch_point_t *pt)
{
	pt->xc = (uint16_t)(cfgConfig.touch_cal[0] * touch_point.xc + cfgConfig.touch_cal[1] * touch_point.yc + cfgConfig.touch_cal[2]);
	pt->yc = (uint16_t)(cfgConfig.touch_cal[3] * touch_point.xc + cfgConfig.touch_cal[4] * touch_point.yc + cfgConfig.touch_cal[5]);
}
//==============================================================================
void		SpiTouch::GetCurrentCoordsRaw(touch_point_t *pt)
{
	pt->xc = touch_point.xc;
	pt->yc = touch_point.yc;
}
//==============================================================================
void		SpiTouch::GetLastCoords(touch_point_t *pt)
{
	pt->xc = (uint16_t)(cfgConfig.touch_cal[0] * touch_info.xc + cfgConfig.touch_cal[1] * touch_info.yc + cfgConfig.touch_cal[2]);
	pt->yc = (uint16_t)(cfgConfig.touch_cal[3] * touch_info.xc + cfgConfig.touch_cal[4] * touch_info.yc + cfgConfig.touch_cal[5]);
}
//==============================================================================
void		SpiTouch::GetLastCoordsRaw(touch_point_t *pt)
{
	pt->xc = touch_info.xc;
	pt->yc = touch_info.yc;
}
//==============================================================================
uint8_t		SpiTouch::IsPressed()
{
	return touch_point.xc || touch_point.yc;
}
//==============================================================================
*/



bool				SpiTouch::getRawPoint(int16_t *x, int16_t *y)
{
	*x = touch_info.xc_raw;
	*y = touch_info.yc_raw;
	return touch_point.xc || touch_point.yc;
}
