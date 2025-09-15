/*******************************************************************************
 * @file  blinky.c
 * @brief
 *******************************************************************************
 * # License
 * <b>Copyright 2023 Silicon Laboratories Inc. www.silabs.com</b>
 *******************************************************************************
 *
 * The licensor of this software is Silicon Laboratories Inc. Your use of this
 * software is governed by the terms of Silicon Labs Master Software License
 * Agreement (MSLA) available at
 * www.silabs.com/about-us/legal/master-software-license-agreement. This
 * software is distributed to you in Source Code format and is governed by the
 * sections of the MSLA applicable to Source Code.
 *
 ******************************************************************************/
/**============================================================================
 * @section Description
 * This files contains example code to demonstrate the GPIO/LED toggle functionality.
 ============================================================================**/

// Include Files
#include "rsi_ccp_user_config.h"
#include "sl_sleeptimer.h"
#include "sl_si91x_led_instances.h"
#include "sl_si91x_led.h"
#include "blinky.h"

// #include "sl_si91x_driver_gpio.h"
// #include "sl_driver_gpio.h"
// #include "sl_gpio_board.h"
#include "app_log.h"

/*******************************************************************************
 *******************************   DEFINES   ***********************************
 ******************************************************************************/

#ifndef LED_INSTANCE
#define LED_INSTANCE_LED0 led_led0
#define LED_INSTANCE_LED1 led_led1
#endif

#ifndef HANDLE_TOGGLE_DELAY_MS
#define HANDLE_TOGGLE_DELAY_MS 10
#endif

extern void green_led_init(void);
extern void green_led_handler(void);
extern bool green_led_play(LedPattern_t pNewPattern);

static bool ledsConfigPattern(LedPattern_t pNewPattern);
static void blue_led_handler(void);

/////===================================================================
sl_sleeptimer_timer_handle_t timer;
bool toggle_timeout = false;
static void on_timeout(sl_sleeptimer_timer_handle_t *handle, void *data);

/*******************************************************************************
 * Initialize blinky example.
 ******************************************************************************/
void blinky_init(void)
{
  // Create timer for waking up the system periodically.
  sl_sleeptimer_start_periodic_timer_ms(&timer,
                                        HANDLE_TOGGLE_DELAY_MS,
                                        on_timeout,
                                        NULL,
                                        1,
                                        SL_SLEEPTIMER_NO_HIGH_PRECISION_HF_CLOCKS_REQUIRED_FLAG);
}

/***************************************************************************/ /**
 * Sleeptimer timeout callback.
 ******************************************************************************/
static void on_timeout(sl_sleeptimer_timer_handle_t *handle, void *data)
{
  (void)&handle;
  (void)&data;
  // toggle_timeout = true;
  blue_led_handler();
  green_led_handler();
}
////============================================================================

/////////////////////////////////////////////////////////////////////////////
// #define	LED_R_GPIO_PORT_PIN	 IO_PORTA_10
#define LED_B_ON        sl_si91x_led_set(LED_INSTANCE_LED0.pin)
#define LED_B_OFF       sl_si91x_led_clear(LED_INSTANCE_LED0.pin)

static int	led_cycle = 0;

/* This is the main LED_BLUE state - initialised when used. */
static LedState_t	LED_BLUE;

/*BLUE_LED_ON_RPT*/ 
const ledEntry_t pattern_BLUE_ON_RPT [ 1 ] = 
{    
	{ 0x0001 , LED_ON  , LED_CYCLE(1000) }  
};

/*BLUE_LED_OFF_RPT*/
const ledEntry_t pattern_BLUE_OFF_RPT [ 1 ] =
{
  	{ 0x0001 , LED_ON  , LED_CYCLE(1000) }  
};

/*BLUE_SLOW_BLINK_PRT*/ 
const ledEntry_t pattern_BLUE_SLOW_BLINK_PRT [ 2 ] = 
{	
	{ 0x0001 , LED_ON, LED_CYCLE(1000)    },
	{ 0x0001 , LED_OFF, LED_CYCLE(1000)   }
};

/*BLUE_FAST_BLINK_RPT*/ 
const ledEntry_t pattern_BLUE_FAST_BLINK_RPT [ 2 ] = 
{	
	{ 0x0001 , LED_ON  , LED_CYCLE(250)  },
	{ 0x0001 , LED_OFF , LED_CYCLE(250)  }  
};

// /*GREEN_ON_RPT*/ 
// const ledEntry_t pattern_GREEN_ON_RPT [ 1 ] = 
// {
// 	{ 0x0004 , LED_ON  , LED_CYCLE(1000) }  
// }; 

// /*BLUE_RED_1s_1times_PRT*/ 
// const ledEntry_t pattern_BLUE_RED_fast_blink_PRT [ 4 ] = 
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

/*The LED_BLUE entries*/
const led_t blue_gLeds [] = 
{	
	{  { 1,  0x00 , LED_RPT }, (ledEntry_t *) pattern_BLUE_ON_RPT  } ,
    {  { 1,  0x00 , LED_RPT }, (ledEntry_t *) pattern_BLUE_OFF_RPT } ,
	{  { 2,  0x00 , LED_RPT }, (ledEntry_t *) pattern_BLUE_SLOW_BLINK_PRT } ,
	{  { 2,  0x00 , LED_RPT }, (ledEntry_t *) pattern_BLUE_FAST_BLINK_RPT } ,
	// {  { 2,  0x00 , LED_RPT }, (ledEntry_t *) pattern_GREEN_SLOW_BLINK_RPT } ,
	// {  { 2,  0x00 , LED_RPT }, (ledEntry_t *) pattern_GREEN_FAST_BLINK_RPT } ,
	// {  { 4,  0x00 , LED_RPT }, (ledEntry_t *) pattern_ALTE_TOGGLE_SLOW_BLINK_RPT } ,    
	// {  { 4,  0x00 , LED_RPT }, (ledEntry_t *) pattern_ALTE_TOGGLE_FAST_BLINK_RPT } ,	
	// {  { 2,  0x00 , LED_RPT }, (ledEntry_t *) pattern_GREEN_fast_blink_RPT } ,	
	// {  { 2,  0x00 , LED_RPT }, (ledEntry_t *) pattern_GREEN_slow_blink_RPT } ,	
	// {  { 1,  0x00 , LED_RPT }, (ledEntry_t *) pattern_GREEN_ON_RPT } ,    
	// {  { 4,  0x00 , LED_RPT }, (ledEntry_t *) pattern_BLUE_RED_fast_blink_PRT } ,    
};
#define	LED_NUM_PATTERNS	sizeof(blue_gLeds)/sizeof(blue_gLeds[0])

static void blue_led_set(bool pOnOrOff)
{
	pOnOrOff ? LED_B_ON : LED_B_OFF;
}

void blue_led_init(void)
{
    LED_BLUE.gCurrentPattern = LEDS_OFF;
	LED_BLUE.gCurrentPatternPosition = 0;	
	LED_BLUE.gRepeatingPattern = LEDS_OFF;	
	LED_BLUE.gRepeatingPatternPosition = 0;	
	blue_led_set(LED_OFF);
	led_cycle = 0;
}

void leds_init(void)
{
	green_led_init();
	blue_led_init();
  	blinky_init();
}

static bool ledsConfigPattern(LedPattern_t pNewPattern)
{
	bool bUpdate = true;		
	/* If current pattern is repeating */	
	if ( blue_gLeds[LED_BLUE.gCurrentPattern].header.repeat)
  	{
		/*If new pattern is repeating */		
		if ( blue_gLeds[pNewPattern].header.repeat )
    	{
			/* then interrupt the pattern with the new repeating pattern. */			
			LED_BLUE.gCurrentPatternPosition = 0;
			LED_BLUE.gCurrentPattern = pNewPattern;
			LED_BLUE.gRepeatingPattern = pNewPattern;
			LED_BLUE.gRepeatingPatternPosition = 0;
		}
		else
		{
			/* Interrupt the current pattern with a repeating pattern. */	
			/* Then store the current pattern to be resumed */			
			LED_BLUE.gRepeatingPattern = LED_BLUE.gCurrentPattern;			
			LED_BLUE.gRepeatingPatternPosition = LED_BLUE.gCurrentPatternPosition; 			
			/* and start the requested pattern. */			
			LED_BLUE.gCurrentPattern = pNewPattern;			
			LED_BLUE.gCurrentPatternPosition = 0;					
		}	
	}
  	else
	{
		/* Current pattern is non repeating. */
		/*if the new pattern is repeating */		
		if ( blue_gLeds[pNewPattern].header.repeat )
    	{
			/* then store this to be resumed. */			
			LED_BLUE.gRepeatingPattern = pNewPattern;			
			LED_BLUE.gRepeatingPatternPosition = 0;		
		}
        else
        {
			/* The new pattern is also non-repeating and can't be currently * played. */	
			bUpdate = false;		
		}
	}
	return bUpdate;
}

static bool blue_led_play(LedPattern_t pNewPattern)
{
	bool bUpdate = true;	
	/* Ensure range is valid. */
	if (pNewPattern > (LED_NUM_PATTERNS-1) || pNewPattern == LED_BLUE.gCurrentPattern)
		return false;	
	/* Function which configures the requested pattern, if possible. */	
	bUpdate = ledsConfigPattern(pNewPattern);
	if(bUpdate)
  	{
		blue_led_set(LED_OFF);
		led_cycle = 1;
	}
	return bUpdate;
}

static void blue_led_handler(void)
{
	if(led_cycle)
  	{
		if(--led_cycle == 0)
    	{
			/* The pattern has completed. */
			if (LED_BLUE.gCurrentPatternPosition >= blue_gLeds[LED_BLUE.gCurrentPattern].header.num_entries)
      		{
				if (blue_gLeds[LED_BLUE.gCurrentPattern].header.repeat)
        		{
					/* Reset the repeating pattern. */			
					LED_BLUE.gCurrentPatternPosition = 0;					
				}
        		else
				{
					/* One shot pattern is complete. */			
					/* Return to playing the repeating pattern. */			
					LED_BLUE.gCurrentPattern = LED_BLUE.gRepeatingPattern;			
					LED_BLUE.gCurrentPatternPosition = LED_BLUE.gRepeatingPatternPosition;			
					if (blue_gLeds[LED_BLUE.gCurrentPattern].header.repeat)
          			{
						/* Reset the repeating pattern. */
						LED_BLUE.gCurrentPatternPosition = 0;
					}	
				}
			}
			blue_led_set(blue_gLeds[LED_BLUE.gCurrentPattern].entries[LED_BLUE.gCurrentPatternPosition].On );
			if ( ( blue_gLeds[LED_BLUE.gCurrentPattern].header.num_entries) != 1 )
      		{
				if(blue_gLeds[LED_BLUE.gCurrentPattern].entries[LED_BLUE.gCurrentPatternPosition].Time)
        		{
					led_cycle = blue_gLeds[LED_BLUE.gCurrentPattern].entries[LED_BLUE.gCurrentPatternPosition].Time;
				}
				else
				{
					led_cycle = 1;
				}
			}
			LED_BLUE.gCurrentPatternPosition++;
		}
	}
}

bool leds_play(leds_type led, LedPattern_t pNewPattern)
{
	if (led == BLUE_LED) {
		return blue_led_play(pNewPattern);
	} else if (led == GREEN_LED) {
		return green_led_play(pNewPattern);
	} else {
		return false;
	}
}