/*
 * lsm6ds3tr.c
 *
 *  Created on: 2025年7月18日
 *      Author: YQ05165
 */
#include "cmsis_os2.h"
#include "sl_utility.h"
#include <string.h>

#include "lsm6dsv.h"
#include "lsm6dsv_reg.h"
// #include "gpiointerrupt.h"

#include <rsi_common_apis.h>

#include "app_log.h"
#include "common_i2c.h"
#include "common.h"

// #define FOLLOWER_I2C_ADDR        0x6A  // I2C follower address  (1101 0100)D4->0xF0   (1100 0100)A4->F0   (0100 1000)48->90
// #define FOLLOWER_I2C_ADDR        LSM6DSV_ADDR>>1  // I2C follower address  D5 右移动一位 1101 0100 ->6A
#define I2C_INSTANCE_USED   INSTANCE_ONE
#define LSM6DSV_ADDR        ((LSM6DSV_I2C_ADD_L&0xFE)>>1) //8bit地址，即7bit地址最低位补0

/* Private macro -------------------------------------------------------------*/
#define BOOT_TIME           10 //ms

// #define INTR1_GPIOPORT    gpioPortD
// #define INTR1_GPIOPIN     2
// #define INTR2_GPIOPORT    gpioPortD
// #define INTR2_GPIOPIN     3

#define LSM6DSV_XL_SCALE         LSM6DSV_2g
#define LSM6DSV_RAWDATA_TO_MG    lsm6dsv_from_fs2_to_mg

#define LSM6DSV_GY_SCALE         LSM6DSV_2000dps
#define LSM6DSV_RAWDATA_TO_MDPS  lsm6dsv_from_fs2000_to_mdps

static stmdev_ctx_t dev_ctx = { 0 };
static int16_t acceleration_mg[3] = { 0 };
static int32_t angular_rate_mdps[3] = { 0 };

static int32_t lsm6dsv_i2c_write(void *handle, uint8_t reg, const uint8_t *buf, uint16_t len)
{
    int ret = 0;
    ret = platform_write(I2C_INSTANCE_USED, LSM6DSV_ADDR, handle, reg, buf, len);
    if (ret) {
      app_log_error("sl_i2c_driver_receive_data_blocking : Invalid Parameters, Error Code : %u \r\n", I2C_INSTANCE_USED);
      return INTRE_ERROR;
    }
    return INTRE_SUCCESS;
}

static int32_t lsm6dsv_i2c_read(void *handle, uint8_t regadr, uint8_t *buf, uint16_t len)
{
    int ret = INTRE_SUCCESS;
    ret = platform_read(I2C_INSTANCE_USED, LSM6DSV_ADDR, handle, regadr, buf, len);
    if (ret) {
      app_log_error("sl_i2c_driver_receive_data_blocking : Invalid Parameters, Error Code : %u \r\n", I2C_INSTANCE_USED);
      return INTRE_ERROR;
    }
    return INTRE_SUCCESS;
}

static void lsm6dsv_delay(uint32_t ms)
{
    platform_delay(ms);
}

//static void intr1_callback(uint8_t intNo, void *ctx)
//{
//  app_log_debug("intr1:intNo=%d\r\n", intNo);
//}
//
//static void intr2_callback(uint8_t intNo, void *ctx)
//{
//  app_log_debug("intr2:intNo=%d\r\n", intNo);
//}

// static void lsm6ds3tr_gpio_intr_init()
// {
//     int32_t interrupt1;
//     int32_t interrupt2;

//     CMU_ClockEnable(cmuClock_GPIO, true);
//     GPIO_PinModeSet(INTR1_GPIOPORT, INTR1_GPIOPIN, gpioModeInputPull,1);
//     interrupt1 = GPIOINT_CallbackRegisterExt(INTR1_GPIOPIN,
//              (GPIOINT_IrqCallbackPtrExt_t)intr1_callback,
//               NULL);
//     if(interrupt1==INTERRUPT_UNAVAILABLE){
//         app_log_debug("lsm6ds intr1 invalid\r\n");
//         while(1);
//     }
//     GPIO_ExtIntConfig(  INTR1_GPIOPORT,
//                         INTR1_GPIOPIN,
//                         interrupt1,
//                         true,
//                         true,
//                         true);

//     GPIO_PinModeSet(INTR2_GPIOPORT, INTR2_GPIOPIN, gpioModeInputPull,1);
//     interrupt2 = GPIOINT_CallbackRegisterExt(INTR2_GPIOPIN,
//                  (GPIOINT_IrqCallbackPtrExt_t)intr2_callback,
//                   NULL);
//     if(interrupt2==INTERRUPT_UNAVAILABLE){
//         app_log_debug("lsm6ds intr2 invalid\r\n");
//         while(1);
//     }
//     GPIO_ExtIntConfig ( INTR2_GPIOPORT,
//                       INTR2_GPIOPIN,
//                       interrupt2,
//                       true,
//                       true,
//                       true);
// }

void lsm6dsv_readdata(int16_t* xl_mg, int32_t* gy_mdps)
{
    // lsm6dsv_reg_t reg;
    lsm6dsv_data_ready_t data_ready;
    int16_t data_raw[3];
    if(lsm6dsv_flag_data_ready_get(&dev_ctx, &data_ready) == 0)
      setSystemStatus(SST_IMU, SYSSTATUS_OK);
    else{
      setSystemStatus(SST_IMU, SYSSTATUS_IMU_ERROR);
      xl_mg[0] = 0;
      xl_mg[1] = 0;
      xl_mg[2] = 0;
      gy_mdps[0] = 0;
      gy_mdps[1] = 0;
      gy_mdps[2] = 0;
      return;
    }
    // 读加速度计数据
    if (data_ready.drdy_xl) {
        memset(data_raw, 0x00, 3 * sizeof(int16_t));
        lsm6dsv_acceleration_raw_get(&dev_ctx, data_raw);
        //app_log_debug("Acceleration raw:%d\t%d\t%d\r\n",
        //              data_raw[0], data_raw[1], data_raw[2]);

        acceleration_mg[0] = (roundf(LSM6DSV_RAWDATA_TO_MG(data_raw[0])));
        acceleration_mg[1] = (roundf(LSM6DSV_RAWDATA_TO_MG(data_raw[1])));
        acceleration_mg[2] = (roundf(LSM6DSV_RAWDATA_TO_MG(data_raw[2])));
        // app_log_debug("Acceleration [mg]:%d\t%d\t%d\r\n",
        //            acceleration_mg[0], acceleration_mg[1], acceleration_mg[2]);
    }

    // 陀螺仪数据
    if (data_ready.drdy_gy) {
        /* Read magnetic field data */
        memset (data_raw, 0x00, 3 * sizeof(int16_t));
        lsm6dsv_angular_rate_raw_get (&dev_ctx, data_raw);
        angular_rate_mdps[0] = (int32_t)(roundf(LSM6DSV_RAWDATA_TO_MDPS(data_raw[0])));
        angular_rate_mdps[1] = (int32_t)(roundf(LSM6DSV_RAWDATA_TO_MDPS(data_raw[1])));
        angular_rate_mdps[2] = (int32_t)(roundf(LSM6DSV_RAWDATA_TO_MDPS(data_raw[2])));
      //  app_log_debug("Angular rate [mdps]:%d\t%d\t%d\r\n",
      //             angular_rate_mdps[0], angular_rate_mdps[1], angular_rate_mdps[2]);
    }

    // // 读取温度
    // if (data_ready.drdy_temp)
    // {
    //   /* Read temperature data */
    //   memset (&data_raw, 0x00, sizeof(int16_t));
    //   lsm6dsv_temperature_raw_get (&dev_ctx, &data_raw[0]);
    //   float temperature_degC = lsm6dsv_from_lsb_to_celsius (data_raw[0]);
    //   app_log_debug("Temperature [degC]:%d\r\n", (int)(temperature_degC*100));
    // }

    if(xl_mg) {
        xl_mg[0] = acceleration_mg[0];
        xl_mg[1] = acceleration_mg[1];
        xl_mg[2] = acceleration_mg[2];
    }
    if(gy_mdps) {
        gy_mdps[0] = angular_rate_mdps[0];
        gy_mdps[1] = angular_rate_mdps[1];
        gy_mdps[2] = angular_rate_mdps[2];
    }
}

void lsm6dsv_init(void)
{
    uint8_t whoamI;
    uint8_t ret;
    static lsm6dsv_filt_settling_mask_t filt_settling_mask;

    dev_ctx.write_reg = lsm6dsv_i2c_write;
    dev_ctx.read_reg = lsm6dsv_i2c_read;
    dev_ctx.mdelay = lsm6dsv_delay;
    dev_ctx.handle = NULL;

    /* Wait sensor boot time */
    platform_delay(BOOT_TIME);

    /* Check device ID */
    lsm6dsv_device_id_get(&dev_ctx, &whoamI);

    app_log_info("lsm6ds->CHIP ID->0x%02x\r\n", whoamI);
    if (whoamI != LSM6DSV_ID){
      app_log_error("imu: chip id error\r\n");
      return;
    }
    else setSystemStatus(SST_IMU, SYSSTATUS_OK);

    // app_log_debug("lsm6ds reset\r\n");
    /* Restore default configuration */
    lsm6dsv_reset_set(&dev_ctx, LSM6DSV_RESTORE_CTRL_REGS);
    do {
        lsm6dsv_reset_get(&dev_ctx, &ret);
    } while (ret != LSM6DSV_READY);

    /* Enable Block Data Update */
    lsm6dsv_block_data_update_set(&dev_ctx, PROPERTY_ENABLE);
    /* Set Output Data Rate.
    * Selected data rate have to be equal or greater with respect
    * with MLC data rate.
    */
    lsm6dsv_xl_data_rate_set(&dev_ctx, LSM6DSV_ODR_AT_15Hz);
    lsm6dsv_gy_data_rate_set(&dev_ctx, LSM6DSV_ODR_AT_15Hz);
    /* Set full scale */
    lsm6dsv_xl_full_scale_set(&dev_ctx, LSM6DSV_XL_SCALE);
    lsm6dsv_gy_full_scale_set(&dev_ctx, LSM6DSV_GY_SCALE);
    /* Configure filtering chain */
    filt_settling_mask.drdy = PROPERTY_ENABLE;
    filt_settling_mask.irq_xl = PROPERTY_ENABLE;
    filt_settling_mask.irq_g = PROPERTY_ENABLE;
    lsm6dsv_filt_settling_mask_set(&dev_ctx, filt_settling_mask);
    lsm6dsv_filt_gy_lp1_set(&dev_ctx, PROPERTY_ENABLE);
    lsm6dsv_filt_gy_lp1_bandwidth_set(&dev_ctx, LSM6DSV_GY_ULTRA_LIGHT);
    lsm6dsv_filt_xl_lp2_set(&dev_ctx, PROPERTY_ENABLE);
    lsm6dsv_filt_xl_lp2_bandwidth_set(&dev_ctx, LSM6DSV_XL_STRONG);

    // /* Disable I3C interface */
    // lsm6dso_i3c_disable_set(&dev_ctx, LSM6DSO_I3C_DISABLE);

    // // 唤醒信号配置
    // lsm6dso_xl_hp_path_internal_set(&dev_ctx, LSM6DSO_USE_HPF);
    // lsm6dso_wkup_threshold_set(&dev_ctx, 1);
    // lsm6dso_pin_polarity_set(&dev_ctx, LSM6DSO_ACTIVE_LOW);

    // lsm6dso_pin_int2_route_t int2_route;
    // lsm6dso_pin_int2_route_get(&dev_ctx, NULL, &int2_route);
    // int2_route.wake_up = PROPERTY_ENABLE;
    // lsm6dso_pin_int2_route_set(&dev_ctx, NULL, int2_route); 
}

void lsm6dsv_test(void)
{
    lsm6dsv_readdata(NULL, NULL);
#if 0
  lsm6dso_all_sources_t all_source;
  /* Check if Wake-Up events */
  lsm6dso_all_sources_get (&dev_ctx, &all_source);

  if (all_source.wake_up)
    {
      app_log_debug("Wake-Up event on:");

      if (all_source.wake_up_x)
        {
          app_log_debug("X");
        }

      if (all_source.wake_up_y)
        {
          app_log_debug("Y");
        }

      if (all_source.wake_up_z)
        {
          app_log_debug("Z");
        }
      app_log_debug("\r\n");
    }
#endif
}
