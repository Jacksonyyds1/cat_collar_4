/*
 * spa06_003.h
 *
 *  Created on: 2025年7月18日
 *      Author: YQ05165
 */
#include <stdbool.h>
#include <stdint.h>

#ifndef SPA06_003_H_
#define SPA06_003_H_

//#define ENABLE_FIFO

/** Device Identification (Who am I) **/
#define SPA06_ID        0x11

#define PRESSURE_SENSOR     0
#define TEMPERATURE_SENSOR  1

#define CONTINUOUS_PRESSURE     1
#define CONTINUOUS_TEMPERATURE  2
#define CONTINUOUS_P_AND_T      3

#define REG_PRS_B2      0x00    // Pressure Data2
#define REG_PRS_B1      0x01    // Pressure Data1
#define REG_PRS_B0      0x02    // Pressure Data0
#define REG_TMP_T2      0x03    // Temperature Data2
#define REG_TMP_T1      0x04    // Temperature Data1
#define REG_TMP_T0      0x05    // Temperature Data0
#define REG_PRS_CFG     0x06    // Pressure Configuration
#define REG_TMP_CFG     0x07    // Temperature Configuration
#define REG_MEAS_CFG    0X08    // Sensor Operating Mode and Status
#define REG_CFG_REG     0X09    // Interrupt and FIFO configuration
#define REG_INT_STS     0X0A    // Interrupt Status
#define REG_FIFO_STS    0X0B    // FIFO Status
#define REG_RESET       0X0C    // Soft Reset and FIFO flush
#define REG_ID          0X0D    // Product and Revision ID
#define REG_COEF_0      0X10    // Calibration Coefficients
#define REG_COEF_1      0X11    // Calibration Coefficients
#define REG_COEF_2      0X12    // Calibration Coefficients
#define REG_COEF_3      0X13    // Calibration Coefficients
#define REG_COEF_4      0X14    // Calibration Coefficients
#define REG_COEF_5      0X15    // Calibration Coefficients
#define REG_COEF_6      0X16    // Calibration Coefficients
#define REG_COEF_7      0X17    // Calibration Coefficients
#define REG_COEF_8      0X18    // Calibration Coefficients
#define REG_COEF_9      0X19    // Calibration Coefficients
#define REG_COEF_10     0X1A    // Calibration Coefficients
#define REG_COEF_11     0X1B    // Calibration Coefficients
#define REG_COEF_12     0X1C    // Calibration Coefficients
#define REG_COEF_13     0X1D    // Calibration Coefficients
#define REG_COEF_14     0X1E    // Calibration Coefficients
#define REG_COEF_15     0X1F    // Calibration Coefficients
#define REG_COEF_16     0X20    // Calibration Coefficients
#define REG_COEF_17     0X21    // Calibration Coefficients
#define REG_COEF_18     0X22    // Calibration Coefficients
#define REG_COEF_19     0X23    // Calibration Coefficients
#define REG_COEF_20     0X24    // Calibration Coefficients

#ifdef ENABLE_FIFO
typedef enum{
  FIFO_STATUS_UNKNOWN = 0,
  FIFO_STATUS_EMPTY = 1,
  FIFO_STATUS_FULL = 2,
  FIFO_STATUS_HAVE_DATA = 3,
}FIFO_STATUS;
#endif

struct spa06_calib_param_t {
    int16_t c0;
    int16_t c1;
    int32_t c00;
    int32_t c10;
    int16_t c01;
    int16_t c11;
    int16_t c20;
    int16_t c21;
    int16_t c30;
    int16_t c31;
    int16_t c40;
};

struct spa06_info_t {
    struct spa06_calib_param_t* pCalibParam;
    uint8_t chip_id;
    int32_t i32rawPressure;
    int32_t i32rawTemperature;
    int32_t i32kP;
    int32_t i32kT;
};

void spa06_init(void);        // 初始化
void spa06_soft_reset(void);  // 软件复位
int16_t spa06_get_temperature(void);
int32_t spa06_get_pressure(void);
int32_t spa06_get_elevation(double pressure);
#ifdef ENABLE_FIFO
uint8_t spa06_is_fifo_ready(void);
void spa06_reset_fifo_reday(void);
void spa06_get_fifo_data(void);
#endif
void spa06_test(void);

#endif /* SPA06_003_H_ */
