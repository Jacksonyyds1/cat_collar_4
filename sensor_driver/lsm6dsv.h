/*
 * lsm6ds3tr.h
 *
 *  Created on: 2025年7月19日
 *      Author: YQ05165
 */

#ifndef LSM6DSV_H_
#define LSM6DSV_H_

#include "stdint.h"

void lsm6dsv_init(void);
void lsm6dsv_readdata(int16_t* xl_mg, int32_t* gy_mdps);
// void lsm6dsv_readdata();
void lsm6dsv_test(void);

#endif /* LSM6DSV_H_ */
