// Include Files
#include "blinky.h"
#include "sl_si91x_driver_gpio.h"
#include "sl_driver_gpio.h"
#include "sl_gpio_board.h"

static bool ledsConfigPattern(LedPattern_t pNewPattern);

static sl_si91x_gpio_pin_config_t sl_gpio_pin_config_led1 = {
  {
    SL_SI91X_ULP_GPIO_5_PORT,
    SL_SI91X_ULP_GPIO_5_PIN
  },
  GPIO_OUTPUT
};

// #define	LED_G_GPIO_PORT_PIN	 IO_PORTA_09
#define LED_G_ON        sl_gpio_driver_set_pin(&sl_gpio_pin_config_led1.port_pin)
#define LED_G_OFF       sl_gpio_driver_clear_pin(&sl_gpio_pin_config_led1.port_pin)

static int	led_cycle = 0;

/* This is the main LED_GREEN state - initialised when used. */
static LedState_t	LED_GREEN;

/*All of The LED_GREEN pins used*/
const int gLedPinsUsed = 0x0001; 

/*LEDS_OFF*/ 
const ledEntry_t pattern_GREEN_LEDS_OFF [ 1 ] = 
{	
	{ 0x0003 , LED_OFF , LED_CYCLE(1000) }
};

/*LEDS_ON*/ 
const ledEntry_t pattern_GREEN_LEDS_ON [ 1 ] = 
{	
	{ 0x0003 , LED_ON, LED_CYCLE(1000)  }
};

/*GREEN_SLOW_BLINK_PRT*/ 
const ledEntry_t pattern_GREEN_SLOW_BLINK_PRT [ 2 ] = 
{	
	{ 0x0001 , LED_ON, LED_CYCLE(1000)    },
	{ 0x0001 , LED_OFF, LED_CYCLE(1000)   }
};

/*GREEN_FAST_BLINK_RPT*/ 
const ledEntry_t pattern_GREEN_FAST_BLINK_RPT [ 2 ] = 
{	
	{ 0x0001 , LED_ON  , LED_CYCLE(250)  },
	{ 0x0001 , LED_OFF , LED_CYCLE(250)  }  
};

// /*ALTERNATING_TOGGLE_SLOW_BLINK_RPT*/ 
// const ledEntry_t pattern_ALTE_TOGGLE_SLOW_BLINK_RPT [ 4 ] = 
// {
//   { 0x0001 , LED_ON  , LED_CYCLE(1000) },
//   { 0x0001 , LED_OFF  , LED_CYCLE(0)   },
//   { 0x0002 , LED_ON  , LED_CYCLE(1000) },
//   { 0x0002 , LED_OFF , LED_CYCLE(0)    }
// };

// /*GREEN_RED_1s_1times_PRT*/ 
// const ledEntry_t pattern_GREEN_RED_fast_blink_PRT [ 4 ] = 
// {
// 	#if 1	
// 	//{ 0x0002 , LED_ON  , LED_CYCLE(1000)  }  , 	
// 	{ 0x0003 , LED_ON  , LED_CYCLE(1000)  }  ,	
// 	{ 0x0003 , LED_OFF , LED_CYCLE(1000)  }  
// 	#else	
// 	{ 0x0002 , LED_ON  , LED_CYCLE(999)  }  , 	
// 	{ 0x0002 , LED_OFF  , LED_CYCLE(0)  }  , 	
// 	{ 0x0001 , LED_ON  , LED_CYCLE(999)  }  ,	
// 	{ 0x0001 , LED_OFF , LED_CYCLE(0)}  
// 	#endif
// };

/*The LED_GREEN entries*/
const led_t green_gLeds [] = 
{	
	{  { 1,  0x00 , LED_RPT }, (ledEntry_t *) pattern_GREEN_LEDS_OFF } ,
	{  { 1,  0x00 , LED_RPT }, (ledEntry_t *) pattern_GREEN_LEDS_ON  } ,
	{  { 2,  0x00 , LED_RPT }, (ledEntry_t *) pattern_GREEN_SLOW_BLINK_PRT } ,
	{  { 2,  0x00 , LED_RPT }, (ledEntry_t *) pattern_GREEN_FAST_BLINK_RPT } ,
	// {  { 2,  0x00 , LED_RPT }, (ledEntry_t *) pattern_GREEN_SLOW_BLINK_RPT } ,
	// {  { 2,  0x00 , LED_RPT }, (ledEntry_t *) pattern_GREEN_FAST_BLINK_RPT } ,
	// {  { 4,  0x00 , LED_RPT }, (ledEntry_t *) pattern_ALTE_TOGGLE_SLOW_BLINK_RPT } ,    
	// {  { 4,  0x00 , LED_RPT }, (ledEntry_t *) pattern_ALTE_TOGGLE_FAST_BLINK_RPT } ,	
	// {  { 4,  0x00 , LED_RPT }, (ledEntry_t *) pattern_BLUE_RED_fast_blink_PRT } ,    
};
#define	LED_NUM_PATTERNS	sizeof(green_gLeds)/sizeof(green_gLeds[0])

static void green_led_set(bool pOnOrOff)
{
	pOnOrOff ? LED_G_ON : LED_G_OFF;
}

void green_led_init(void)
{
  	sl_status_t status;

	// Initialize the LED_GREEN pin
	status = sl_gpio_set_configuration(sl_gpio_pin_config_led1);
	if (status != SL_STATUS_OK) {
		return;
	}

    LED_GREEN.gCurrentPattern = LEDS_OFF;
	LED_GREEN.gCurrentPatternPosition = 0;	
	LED_GREEN.gRepeatingPattern = LEDS_OFF;	
	LED_GREEN.gRepeatingPatternPosition = 0;	
	green_led_set(LED_OFF);
	led_cycle = 0;
}

static bool ledsConfigPattern(LedPattern_t pNewPattern)
{
	bool bUpdate = true;		
	/* If current pattern is repeating */	
	if ( green_gLeds[LED_GREEN.gCurrentPattern].header.repeat)
  	{
		/*If new pattern is repeating */		
		if ( green_gLeds[pNewPattern].header.repeat )
    	{
			/* then interrupt the pattern with the new repeating pattern. */			
			LED_GREEN.gCurrentPatternPosition = 0;
			LED_GREEN.gCurrentPattern = pNewPattern;
			LED_GREEN.gRepeatingPattern = pNewPattern;
			LED_GREEN.gRepeatingPatternPosition = 0;
		}
		else
		{
			/* Interrupt the current pattern with a repeating pattern. */	
			/* Then store the current pattern to be resumed */			
			LED_GREEN.gRepeatingPattern = LED_GREEN.gCurrentPattern;			
			LED_GREEN.gRepeatingPatternPosition = LED_GREEN.gCurrentPatternPosition; 			
			/* and start the requested pattern. */			
			LED_GREEN.gCurrentPattern = pNewPattern;			
			LED_GREEN.gCurrentPatternPosition = 0;					
		}	
	}
  	else
	{
		/* Current pattern is non repeating. */
		/*if the new pattern is repeating */		
		if ( green_gLeds[pNewPattern].header.repeat )
    	{
			/* then store this to be resumed. */			
			LED_GREEN.gRepeatingPattern = pNewPattern;			
			LED_GREEN.gRepeatingPatternPosition = 0;		
		}
        else
        {
			/* The new pattern is also non-repeating and can't be currently * played. */	
			bUpdate = false;		
		}
	}
	return bUpdate;
}

bool green_led_play(LedPattern_t pNewPattern)
{
	bool bUpdate = true;	
	//printf("show led %d\r",pNewPattern);
	/* Ensure range is valid. */
	if (pNewPattern > (LED_NUM_PATTERNS-1) || pNewPattern == LED_GREEN.gCurrentPattern)
		return false;	
	/* Function which configures the requested pattern, if possible. */	
	bUpdate = ledsConfigPattern(pNewPattern);
	if(bUpdate)
  	{
		green_led_set(LED_OFF);
		led_cycle = 1;
	}
	return bUpdate;
}

void green_led_handler(void)
{
	if(led_cycle)
  	{
		if(--led_cycle == 0)
    	{
			/* The pattern has completed. */
			if (LED_GREEN.gCurrentPatternPosition >= green_gLeds[LED_GREEN.gCurrentPattern].header.num_entries)
      		{
				if (green_gLeds[LED_GREEN.gCurrentPattern].header.repeat)
        		{
					/* Reset the repeating pattern. */			
					LED_GREEN.gCurrentPatternPosition = 0;					
				}
        		else
				{
					/* One shot pattern is complete. */			
					/* Return to playing the repeating pattern. */			
					LED_GREEN.gCurrentPattern = LED_GREEN.gRepeatingPattern;			
					LED_GREEN.gCurrentPatternPosition = LED_GREEN.gRepeatingPatternPosition;			
					if (green_gLeds[LED_GREEN.gCurrentPattern].header.repeat)
          			{
						/* Reset the repeating pattern. */
						LED_GREEN.gCurrentPatternPosition = 0;
					}	
				}
			}
			green_led_set(green_gLeds[LED_GREEN.gCurrentPattern].entries[LED_GREEN.gCurrentPatternPosition].On );
			if ( ( green_gLeds[LED_GREEN.gCurrentPattern].header.num_entries) != 1 )
      		{
				if(green_gLeds[LED_GREEN.gCurrentPattern].entries[LED_GREEN.gCurrentPatternPosition].Time)
        		{
					led_cycle = green_gLeds[LED_GREEN.gCurrentPattern].entries[LED_GREEN.gCurrentPatternPosition].Time;
				}
				else
				{
					led_cycle = 1;
				}
			}
			LED_GREEN.gCurrentPatternPosition++;
		}
	}
}

