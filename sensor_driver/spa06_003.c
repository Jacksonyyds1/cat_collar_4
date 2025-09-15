/*
 * spa06_003.c
 *
 *  Created on: 2025年7月19日
 *      Author: YQ05165
 */

#include "spa06_003.h"
#include "common.h"
#include "cmsis_os2.h"

#include "sl_utility.h"
#include "app_log.h"
#include "common_i2c.h"
#include "math.h"

// #include "em_gpio.h"
// #include "gpiointerrupt.h"

// #define SPA06_INTR_GPIOPORT   gpioPortC
// #define SPA06_INTR_GPIOPIN    0

#define SPA06_ADDR  (0x76)    // I2C7位地址，SDO下拉时为0x76，否则为0x77 111011
#define I2C_INSTANCE_USED INSTANCE_ONE

// #define SPA06_ADDR  (0xEC)          // I2C7位地址，SDO下拉时为0x76 左移1位->0xEC
// #define SPA06_OBJ   SL_I2CSPM_SENSOR_PERIPHERAL

static struct spa06_info_t spa06_info = { 0 };
static struct spa06_calib_param_t sCalibParam = { 0 };

// #define FOLLOWER_I2C_ADDR        0x6A  // I2C follower address  (1101 0100)D4->0xF0   (1100 0100)A4->F0   (0100 1000)48->90
// #define FOLLOWER_I2C_ADDR        SPA06_ADDR

static intre_err spa06_reg_readMultiByte (uint8_t regadr, uint8_t* buf, uint16_t len)
{
    int ret = INTRE_SUCCESS;

    ret = platform_read(I2C_INSTANCE_USED, SPA06_ADDR, NULL, regadr, buf, len);

    if (ret) {
      app_log_error("sl_i2c_driver_receive_data_blocking : Invalid Parameters, Error Code : %u \r\n", I2C_INSTANCE_USED);
      return INTRE_ERROR;
    }
    return INTRE_SUCCESS;
}

static uint8_t spa06_reg_readbyte (uint8_t regadr)
{
    uint8_t rdata = 0;
    spa06_reg_readMultiByte(regadr, &rdata, 1);
    return rdata;
}

static intre_err spa06_reg_writeMultiByte(uint8_t regadr, uint8_t* buf, uint16_t len)
{
    int ret = INTRE_SUCCESS;
    ret = platform_write(I2C_INSTANCE_USED, SPA06_ADDR, NULL, regadr, buf, len);
    if (ret) {
      app_log_error("sl_i2c_driver_receive_data_blocking : Invalid Parameters, Error Code : %u \r\n", I2C_INSTANCE_USED);
      return INTRE_ERROR;
    }
    return INTRE_SUCCESS;
}

static intre_err spa06_reg_writebyte(uint8_t regadr, uint8_t val)
{
   return spa06_reg_writeMultiByte(regadr, &val, 1);
}

static uint8_t spa06_chipID(void)
{
    uint8_t id =  spa06_reg_readbyte(REG_ID);
    return id;
}

static uint8_t is_spa06_003(void)
{
 if(spa06_info.chip_id == SPA06_ID)  return 1;
 else  return 0;
}

static void spa06_get_calib_param(void)
{
    uint8_t h;
    uint8_t m;
    uint8_t l;
    h =  spa06_reg_readbyte(0x10);
    l =  spa06_reg_readbyte(0x11);
    sCalibParam.c0 = (int16_t)h<<4 | l>>4;
    sCalibParam.c0 = (sCalibParam.c0&0x0800)?(0xF000|sCalibParam.c0):sCalibParam.c0;

    h =  spa06_reg_readbyte(0x11);
    l =  spa06_reg_readbyte(0x12);
    sCalibParam.c1 = (int16_t)(h&0x0F)<<8 | l;
    sCalibParam.c1 = (sCalibParam.c1&0x0800)?(0xF000|sCalibParam.c1):sCalibParam.c1;

    h =  spa06_reg_readbyte(0x13);
    m =  spa06_reg_readbyte(0x14);
    l =  spa06_reg_readbyte(0x15);
    sCalibParam.c00 = (int32_t)h<<12 | (int32_t)m<<4 | (int32_t)l>>4;
    sCalibParam.c00 = (sCalibParam.c00&0x080000)?(int32_t)(0xFFF00000|sCalibParam.c00):sCalibParam.c00;

    h =  spa06_reg_readbyte(0x15);
    m =  spa06_reg_readbyte(0x16);
    l =  spa06_reg_readbyte(0x17);
    sCalibParam.c10 = (int32_t)(h&0x0F)<<16 | (int32_t)m<<8 | l;
    sCalibParam.c10 = (sCalibParam.c10&0x080000)?((int32_t)0xFFF00000|sCalibParam.c10):sCalibParam.c10;

    h =  spa06_reg_readbyte(0x18);
    l =  spa06_reg_readbyte(0x19);
    sCalibParam.c01 = (int16_t)h<<8 | l;

    h =  spa06_reg_readbyte(0x1A);
    l =  spa06_reg_readbyte(0x1B);
    sCalibParam.c11 = (int16_t)h<<8 | l;

    h =  spa06_reg_readbyte(0x1C);
    l =  spa06_reg_readbyte(0x1D);
    sCalibParam.c20 = (int16_t)h<<8 | l;

    h =  spa06_reg_readbyte(0x1E);
    l =  spa06_reg_readbyte(0x1F);
    sCalibParam.c21 = (int16_t)h<<8 | l;

    h =  spa06_reg_readbyte(0x20);
    l =  spa06_reg_readbyte(0x21);
    sCalibParam.c30 = (int16_t)h<<8 | l;

    if(is_spa06_003())
    {
        // app_log_debug("spa06_get_calib_param ex\r\n");
        h = spa06_reg_readbyte(0x22);
        l = spa06_reg_readbyte(0x23);
        sCalibParam.c31 = (int16_t)h << 4 | l >> 4;
        sCalibParam.c31 = (sCalibParam.c31 & 0x0800) ? (0xF000 | sCalibParam.c31) : sCalibParam.c31;

        h = spa06_reg_readbyte(0x23);
        l = spa06_reg_readbyte(0x24);
        sCalibParam.c40 = (int16_t)(h & 0x0F) << 8 | l;
        sCalibParam.c40 = (sCalibParam.c40 & 0x0800) ? (0xF000 | sCalibParam.c40) : sCalibParam.c40;
    }
}

static float spa06_Calibration_temperature(int32_t rawTemperature)
{
    float fTCompensate;
    float fTsc;
    fTsc = rawTemperature / (float)spa06_info.i32kT;
    fTCompensate =  spa06_info.pCalibParam->c0 * 0.5 + spa06_info.pCalibParam->c1 * fTsc;
    return fTCompensate;
}

static float spa06_Calibration_pressure(int32_t rawPressure)
{
    float fTsc, fPsc;
    float qua2, qua3;
    float fPCompensate;

    fTsc = spa06_info.i32rawTemperature / (float)spa06_info.i32kT;
    fPsc = rawPressure / (float)spa06_info.i32kP;
    qua2 = spa06_info.pCalibParam->c10 + fPsc * (spa06_info.pCalibParam->c20 + fPsc* (spa06_info.pCalibParam->c30+ fPsc * spa06_info.pCalibParam->c40));
    if(is_spa06_003())
      qua3 = fTsc * fPsc * (spa06_info.pCalibParam->c11 + fPsc * (spa06_info.pCalibParam->c21+ fPsc * spa06_info.pCalibParam->c31));
    else
      qua3 = fTsc * fPsc * (spa06_info.pCalibParam->c11 + fPsc * spa06_info.pCalibParam->c21);
    fPCompensate = spa06_info.pCalibParam->c00 + fPsc * qua2 + fTsc * spa06_info.pCalibParam->c01 + qua3;

    spa06_info.i32rawPressure = rawPressure;
    return fPCompensate;
}

static void spa06_rateset(uint8_t iSensor, uint8_t u8SmplRate, uint8_t u8OverSmpl)
{
    uint8_t reg = 0;
    int32_t i32kPkT = 0;
    switch (u8SmplRate)
    {
    case 2:
      reg |= (1 << 4);
      break;
    case 4:
      reg |= (2 << 4);
      break;
    case 8:
      reg |= (3 << 4);
      break;
    case 16:
      reg |= (4 << 4);
      break;
    case 32:
      reg |= (5 << 4);
      break;
    case 64:
      reg |= (6 << 4);
      break;
    case 128:
      reg |= (7 << 4);
      break;
    case 1:
    default:
      break;
    }

    switch (u8OverSmpl)
    {
    case 2:
      reg |= 1;
      i32kPkT = 1572864;
      break;
    case 4:
      reg |= 2;
      i32kPkT = 3670016;
      break;
    case 8:
      reg |= 3;
      i32kPkT = 7864320;
      break;
    case 16:
      i32kPkT = 253952;
      reg |= 4;
      break;
    case 32:
      i32kPkT = 516096;
      reg |= 5;
      break;
    case 64:
      i32kPkT = 1040384;
      reg |= 6;
      break;
    case 128:
      i32kPkT = 2088960;
      reg |= 7;
      break;
    case 1:
    default:
      i32kPkT = 524288;
      break;
    }

    if (iSensor == PRESSURE_SENSOR)
    {
        spa06_info.i32kP = i32kPkT;
        spa06_reg_writebyte(REG_PRS_CFG, reg);
        if (u8OverSmpl > 8)
        {
          reg = spa06_reg_readbyte (REG_CFG_REG);
          spa06_reg_writebyte(REG_CFG_REG, reg | 0x04);
        }
        else
        {
          reg = spa06_reg_readbyte (REG_CFG_REG);
          spa06_reg_writebyte(REG_CFG_REG, reg & (~0x04));
        }
    }
    if (iSensor == TEMPERATURE_SENSOR)
    {
        spa06_info.i32kT = i32kPkT;
        if (is_spa06_003())
          spa06_reg_writebyte (REG_TMP_CFG, reg);
        else
          spa06_reg_writebyte (REG_TMP_CFG, reg | 0x80);  //Using mems temperature
        if (u8OverSmpl > 8)
        {
            reg = spa06_reg_readbyte (REG_CFG_REG);
            spa06_reg_writebyte (REG_CFG_REG, reg | 0x08);
        }
        else
        {
            reg = spa06_reg_readbyte(REG_CFG_REG);
            spa06_reg_writebyte(REG_CFG_REG, reg & (~0x08));
        }
    }
}

/*
mode:
#define CONTINUOUS_PRESSURE     1
#define CONTINUOUS_TEMPERATURE  2
#define CONTINUOUS_P_AND_T      3
*/
static void spa06_start_continuous(uint8_t mode)
{
    spa06_reg_writebyte(REG_MEAS_CFG, mode+4);
}


// static volatile uint8_t sFifoReady = 0;
// static void intr_callback(uint8_t intNo, void *ctx)
// {
//   intNo = intNo;
//   ctx = ctx;
//   app_log_debug("...\r\n");
//   sFifoReady = 1;
// }

// static void spa06_gpio_intr_init(void)
// {
//   int32_t interrupt;

//   GPIO_PinModeSet(SPA06_INTR_GPIOPORT, SPA06_INTR_GPIOPIN, gpioModeInputPull,0);
//   // GPIO_PinModeSet(SPA06_INTR_GPIOPORT, SPA06_INTR_GPIOPIN, gpioModePushPull, 1);
//   // GPIO_PinOutClear(SPA06_INTR_GPIOPORT, SPA06_INTR_GPIOPIN);
//   interrupt = GPIOINT_CallbackRegisterExt(SPA06_INTR_GPIOPIN,
//            (GPIOINT_IrqCallbackPtrExt_t)intr_callback,
//             NULL);
//   if(interrupt==INTERRUPT_UNAVAILABLE){
//       app_log_debug("interrupt invalid\r\n");
//       while(1);
//   }

//   GPIO_ExtIntConfig(  SPA06_INTR_GPIOPORT,
//                       SPA06_INTR_GPIOPIN,
//                       interrupt,
//                       true,
//                       false,
//                       true);
// }

#ifdef ENABLE_FIFO
static void spa06_enable_fifo(void)
{
  uint8_t val;
  val = spa06_reg_readbyte (REG_CFG_REG);

  val |= (0x01<<7);  // Interrupt (on SDO pin) active level: Active high
  val |= (0x01<<6);  // Generate interrupt when the FIFO is full
  val |= (0x01<<1);  // Enable the FIFO

  spa06_reg_writebyte(REG_CFG_REG, val);
}

static void spa06_clear_fifo(void)
{
    uint8_t val = 0x80;
    spa06_reg_writebyte(REG_RESET, val);
}

static uint8_t spa06_clear_intr_status(void)
{
    // Interrupt Status (INT_STS)
    // Interrupt status register. The register is cleared on read.
    uint8_t intr_status = spa06_reg_readbyte(REG_INT_STS);
    app_log_debug("intr_status=0x%02x\r\n",intr_status);
    return intr_status;
}


static FIFO_STATUS spa06_get_fifo_status(void)
{
    FIFO_STATUS fifo_status = FIFO_STATUS_HAVE_DATA;
    uint8_t st = spa06_reg_readbyte(REG_FIFO_STS);

    if(st&0x01){
        fifo_status = FIFO_STATUS_FULL;
        app_log_debug("FIFO_FULL\r\n");
    }
    else if(st&0x02){
        fifo_status = FIFO_STATUS_EMPTY;
        app_log_debug("FIFO_EMPTY\r\n");
    }

    return fifo_status;
}

// 读取fifo数据就绪标记
uint8_t spa06_is_fifo_ready(void)
{
    return sFifoReady;
}
// 清除fifo数据就绪标记
void spa06_reset_fifo_reday(void)
{
    sFifoReady = 0;
}

void spa06_get_fifo_data(void)
{
  uint8_t buf[3] = { 0 };
  uint8_t dataType = 0;
  int32_t i32data = 0;
  float fdata = 0;
  uint8_t fifo_depth = 32;  // fifo 最多存储32个数据
  uint8_t i = 0;

  for(i=0;i<fifo_depth;i++)
  {
      // 查询FIFO队列是否为空
      if (spa06_get_fifo_status () == FIFO_STATUS_EMPTY)
      {
          LOG_PRINT("spa06_get_fifo_data empty1\r\n");
          break;
      }
      // 开始读取数据
      // fifo模式下，不论温度还是气压值，都是存储在pressure data寄存器，最后一位表示数据类型：0--温度 1--气压
      spa06_reg_readMultiByte (REG_PRS_B2, buf, 3);

      if(buf[0]==0x80 && buf[1]==0x00 && buf[2]==0x00){
          LOG_PRINT("spa06_get_fifo_data empty2\r\n");
          break;
      }


      i32data = (int32_t)buf[0]<<15 | (int32_t)buf[1]<<7 | (int32_t)buf[2]>>1;
      i32data = (i32data&0x800000) ? (int32_t) (0xFF000000|i32data) : i32data;

      dataType = buf[2] & 0x01;
      // 气压值
      if (dataType)
      {
          spa06_info.i32rawPressure = i32data;
          fdata = spa06_Calibration_pressure (i32data);
          app_log_debug("pressure:%f\n", fdata);
      }
      // 温度值
      else
      {
          spa06_info.i32rawTemperature = i32data;
          fdata = spa06_Calibration_temperature (i32data);
          app_log_debug("temperature:%f\n", fdata);
      }
  }

  // 清空FIFO队列
  spa06_clear_fifo();
  // 清除中断
  spa06_clear_intr_status();
}

#endif

void spa06_init(void)
{
    spa06_info.pCalibParam = &sCalibParam;
    spa06_info.i32rawPressure = 0;
    spa06_info.i32rawTemperature = 0;

    // spa06_gpio_intr_init();

    spa06_info.chip_id = spa06_chipID();
    app_log_info("spa06->CHIP ID->0x%02x\r\n", spa06_info.chip_id);
    if(is_spa06_003()==0){
        app_log_error("is_spa06_003==0\r\n");
        setSystemStatus(SST_AIRPRESS, SYSSTATUS_AIRPRESS_ERROR);
        return;
    }
    setSystemStatus(SST_AIRPRESS, SYSSTATUS_OK);

    spa06_get_calib_param();

    // app_log_debug("c0=0x%04x\r\n", sCalibParam.c0);
    // app_log_debug("c1=0x%04x\r\n", sCalibParam.c1);
    // app_log_debug("c00=0x%04x\r\n", sCalibParam.c00);
    // app_log_debug("c10=0x%04x\r\n", sCalibParam.c10);
    // app_log_debug("c01=0x%04x\r\n", sCalibParam.c01);
    // app_log_debug("c11=0x%04x\r\n", sCalibParam.c11);
    // app_log_debug("c20=0x%04x\r\n", sCalibParam.c20);
    // app_log_debug("c21=0x%04x\r\n", sCalibParam.c21);
    // app_log_debug("c30=0x%04x\r\n", sCalibParam.c30);
    // app_log_debug("c31=0x%04x\r\n", sCalibParam.c31);
    // app_log_debug("c40=0x%04x\r\n", sCalibParam.c40);

#ifdef ENABLE_FIFO
    spa06_enable_fifo();
#endif

    spa06_rateset(PRESSURE_SENSOR, 32, 8);
    spa06_rateset(TEMPERATURE_SENSOR, 32, 8);
    spa06_start_continuous(CONTINUOUS_PRESSURE);
}


// 获取温度，单位0.1℃
int16_t spa06_get_temperature(void)
{
    uint8_t buf[3] = { 0 };
    intre_err ret = INTRE_ERROR;
    int32_t rawTemperature = 0;
    float fTCompensate;
    int16_t iTCompensate = 0;

    ret = spa06_reg_readMultiByte(REG_TMP_T2, buf, 3);
    if(ret == INTRE_SUCCESS)
      setSystemStatus(SST_AIRPRESS, SYSSTATUS_OK);
    else {
      setSystemStatus(SST_AIRPRESS, SYSSTATUS_AIRPRESS_ERROR);
      return 0;
    }

    // 采样值
    rawTemperature = (int32_t)buf[0]<<16 | (int32_t)buf[1]<<8 | (int32_t)buf[2];
    rawTemperature = (rawTemperature&0x800000) ? (int32_t)(0xFF000000|rawTemperature) : rawTemperature;

    // 校正值
    fTCompensate = spa06_Calibration_temperature(rawTemperature);
    spa06_info.i32rawTemperature = rawTemperature;
    iTCompensate = (int16_t)(roundf(fTCompensate*10)); // 扩大十倍后四舍五入

    // app_log_debug("rawTemperature=%d,fTCompensate=%d\r\n", rawTemperature, (int)(fTCompensate));
    return iTCompensate;
}

// 获取气压值，单位 0.1pa（帕）
int32_t spa06_get_pressure(void)
{
    uint8_t buf[3] = { 0 };
    intre_err ret = INTRE_ERROR;
    int32_t rawPressure = 0;
    float fPCompensate;
    int32_t iPCompensate;

    ret = spa06_reg_readMultiByte(REG_PRS_B2, buf, 3);
    //app_log_debug("spa06_get_pressure ret=%d\r\n", ret);
    if(ret == INTRE_SUCCESS)
      setSystemStatus(SST_AIRPRESS, SYSSTATUS_OK);
    else  {
        return 0;
        setSystemStatus(SST_AIRPRESS, SYSSTATUS_AIRPRESS_ERROR);
    }

    // 采样值
    rawPressure = (int32_t)buf[0]<<16 | (int32_t)buf[1]<<8 | (int32_t)buf[2];
    rawPressure = (rawPressure&0x800000) ? (int32_t)(0xFF000000|rawPressure) : rawPressure;

    //校正值
    fPCompensate = spa06_Calibration_pressure(rawPressure);
    spa06_info.i32rawPressure = rawPressure;
    iPCompensate = (int32_t)(roundf(fPCompensate*10));  // 扩大十倍后四舍五入

    return iPCompensate;
}

// 海拔高度，单位 0.1m
#define P0 101325
int32_t spa06_get_elevation(double pressure)
{

    int32_t iElevation = 0;
    double fElevation = 0;
#if 0 // 标准计算，消耗较大
    // 根据大气压计算海拔高度
    fElevation = 44330*(1 - pow(pressure/P0, 0.1902949572) );

#else // 简单计算，误差较大
    fElevation = (P0 - pressure)*0.0843;
#endif
    iElevation = round(fElevation*10);
    return iElevation;
}

void spa06_soft_reset(void)
{
    uint8_t val = 0x09;
    spa06_reg_writebyte(REG_RESET, val);
}

void spa06_test(void)
{
#ifdef ENABLE_FIFO
    if(spa06_is_fifo_ready()){
        spa06_reset_fifo_reday();
        spa06_get_fifo_data();
    }
    //uint8_t x = GPIO_PinInGet(SPA06_INTR_GPIOPORT, SPA06_INTR_GPIOPIN);
    //app_log_debug("x=%d\r\n",x);
#else
    int16_t temperature = spa06_get_temperature();
    int32_t pressure = spa06_get_pressure();
    int32_t elevation = spa06_get_elevation(pressure/10.0);
    app_log_debug("temperature [degC]%d\r\n", temperature);
    app_log_debug("pressure [pa]:%ld\r\n", pressure);
    app_log_debug("elevation [cm]%ld\r\n", elevation);

#endif
}

