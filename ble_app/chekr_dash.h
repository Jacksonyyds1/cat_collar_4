#ifndef __CHEKR_DASH_H__
#define __CHEKR_DASH_H__

#include <stdint.h>
#include <stdbool.h>

typedef struct {
	uint8_t battery_pct;
	float battery_v;
	float charging_a;
	float temperature;
} pmic_get_info_t;

int dashboard_init(void);
int dashboard_response(void);
int dashboard_ctrl(dashboard_ctrl_t ctrl, uint8_t interval);

#endif // __CHEKR_DASH_H__