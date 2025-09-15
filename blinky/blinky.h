/***************************************************************************/ /**
 * @file
 * @brief Simple button baremetal examples functions
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

#ifndef BLINKY_H
#define BLINKY_H

#include <stdint.h>
#include <stdbool.h>

#define LED_RPT 	1
#define	LED_CYCLE(ms) ms/10

typedef enum ledTypeTag
{
    BLUE_LED,
    GREEN_LED,
}leds_type;

typedef enum LedPatternTag
{
	LEDS_OFF,
	LEDS_ON,
    LEDS_SLOW_BLINK,
    LEDS_FAST_BLINK,
} LedPattern_t;

typedef struct ledEntryTag
{	
	unsigned short		PioMask;	
	unsigned On		:1;	
	unsigned Time	:15;
}ledEntry_t;

typedef struct ledHeaderTag
{
	unsigned     num_entries    :8;
	unsigned     reserved       :7;
	unsigned     repeat         :1;
}ledHeader_t; 

typedef struct ledbTag
{	
	ledHeader_t  header;	
	ledEntry_t * entries;
}led_t;

/* The LEDs task data. */
typedef struct{    
	/* Current pattern being played. */    
	LedPattern_t gCurrentPattern;       
	/* Position in the sequence of playing the current pattern. */       
	unsigned short gCurrentPatternPosition;    
	/* Current repeating pattern being played */    
	LedPattern_t gRepeatingPattern;    
	/* Position in the sequence through that repeating pattern */    
	unsigned short gRepeatingPatternPosition;    
} LedState_t;

/** State of the LED */
enum led_state 
{	
	/** LED is Off */	
	LED_OFF = 0,	
	/** LED is On */	
	LED_ON,
};

///////////////////////////////////////////////////////////////
void leds_init(void);

bool leds_play(leds_type led, LedPattern_t pNewPattern);

// ///test
// extern bool toggle_timeout;

#endif // BLINK_H
