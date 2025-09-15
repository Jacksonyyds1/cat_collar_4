#ifndef SY6105_BSP_H
#define SY6105_BSP_H

#include "i2c.h"

#define false   0
#define true    1 

#define SY6105_DEBUG	1

#define SY_ENABLE       1
#define SY_DISABLE      0

/***********return status************/
typedef enum{
	RET_SUCCESS = 0x00U, 
#ifdef STM32L031xx
  RET_ERROR = HAL_ERROR,
  RET_BUSY = HAL_BUSY,
  RET_TIMEOUT = HAL_TIMEOUT,
#else
  RET_ERROR = 0x01U,
  RET_BUSY = 0x02U,
  RET_TIMEOUT = 0x03U,
#endif
  RET_PAR_ERR = 0xF0U
} sy_drv_status_t;

/*Register define*/
/*DPM_SET*/
#define REG_DPM_SET 0x00

typedef enum{
    VINDPM_SET_DIS = 0x00U,
    VINDPM_SET_4V2 = 0x01U,
    VINDPM_SET_4V6 = 0x02U,
    VINDPM_SET_4V7 = 0x03U
}vindpm_set_t;

typedef enum{
    IINDPM_SET_DIS     = 0x00U,
    IINDPM_SET_300MA   = 0x01U,
    IINDPM_SET_500MA   = 0x02U,
    IINDPM_SET_1200MA  = 0x03U
}iindpm_set_t;

/*SYS_CTRL0*/
#define REG_SYS_CTRL0       0x01U
typedef enum{
    RST_LEVEL_8S       = 0x00U,
    RST_LEVEL_12S      = 0x01U,
    RST_LEVEL_16S      = 0x02U, 
    RST_LEVEL_20S      = 0x03U 
}rst_level_t;

typedef enum{
    TRST_DUR_2S  =  0x00U,
    TRST_DUR_4S  =  0x01U
}trst_dur_t;

typedef enum{
    VBAT_UVLO_2V42 = 0x00U,
    VBAT_UVLO_2V64 = 0x01U,
    VBAT_UVLO_2V82 = 0x02U,
    VBAT_UVLO_2V90 = 0x03U
}vbat_uvlo_t;


/*SYS_CTRL1*/
#define REG_SYS_CTRL1       0x02U

/*IDSG_ITERM_SET*/
#define REG_IDSG_ITERM_SET      0x03U
typedef enum{
    IBATFET_OCP_DIS     = 0x00U,
    IBATFET_OCP_1000mA  = 0x01U,
    IBATFET_OCP_2000mA  = 0x02U,
    IBATFET_OCP_3200mA  = 0x03U
}ibatfet_ocp_t;

/*BAT_VOL_SET*/
#define REG_BAT_VOL_SET         0x04
typedef enum{
    BAT_LOW_3V3 = 0x00U,
    BAT_LOW_3V5 = 0x01U
}bat_low_t;

typedef enum{
    VBAT_PRE_2V8 = 0x00U,
    VBAT_RRE_3V0 = 0x01U
}vbat_pre_t;

typedef enum{
    VRE_CHG_100mV = 0x00U,
    VRE_CHG_200mV = 0x01U
}v_rechg_t;

/*SYS_CTRL2*/
#define REG_SYS_CTRL2       0x05U
typedef enum{
    WDT_TIMER_DIS   = 0x00U,
    WDT_TIMER_40S   = 0x01U,
    WDT_TIMER_80S   = 0x02U,
    WDT_TIMER_160S  = 0x03U
}wdt_timer_t;

typedef enum{
    CHG_TIMER_5H   =  0x00U,
    CHG_TIMER_8H   =  0x01U
}chg_timer_t;

/*SYS_CTRL3*/
#define REG_SYS_CTRL3       0x06
#define PG_INT_CONTROL          (1<<4)
#define CHG_DONE_INT_EN         (1<<3)
#define CHG_STAT_INT_EN         (1<<2)
#define NTC_TRANSITION_EN       (1<<1)
#define BATOVP_INT_EN           (1<<0)


/*SYS_CTRL4*/
#define REG_SYS_CTRL4       0x07
typedef enum{
    NTC_HOT_TH_45D  =    0x00U,
    NTC_HOT_TH_50D  =    0x01U,
    NTC_HOT_TH_55D  =    0x02U,
    NTC_HOT_TH_60D  =    0x03U
}ntc_hot_th_t;

typedef enum{
    THERMAL_REG_120D =   0x00U,
    THERMAL_REG_DIS  =   0x01U
}thermal_reg_t;

typedef enum{
    KEY_WK_SET_LONG  =   0x00U,       //2~3S
    KEY_WK_SET_SHORT =   0x01U      //125~127ms
}key_wk_time_set_t;

typedef enum{
    VSYS_REG_SET_4V45 = 0x00U,
    VSYS_REG_SET_4V55 = 0x01U,
    VSYS_REG_SET_4V65 = 0x02U,
    VSYS_REG_SET_4V85 = 0x03U
}vsys_reg_set_t;

/*SYS_CTRL5*/
#define REG_SYS_CTRL5       0x1B
typedef enum{
    VBUS_OVP_SEL_6V0  =  0x00U,
    VBUS_OVP_SEL_6V5  =  0x01U
}vbus_ovp_sel_t;


/*CHIP_STAT0*/
#define REG_CHIP_STAT0      0x08

enum{
    NOT_CHG,
    PRE_CHG,
    CC_CHG,
    CV_CHG,
    CHG_DONE
};

/*CHIP_STAT1*/
#define REG_CHIP_STAT1      0x09

enum{
    SHIP_DGL_1S,
    SHIP_DGL_2S,
    SHIP_DGL_4S,
    SHIP_DGL_DIS
};


#define REG_CHIP_STAT2     0x0B

//default little endian
typedef struct{
  // STATE0
  unsigned char THERM_STAT : 1;
  unsigned char VBUS_PG_STAT : 1;
  unsigned char VINDPM_STAT : 1;
  unsigned char CHG_STAT : 3;
  unsigned char IINDPM_STAT : 1;
  unsigned char WDT_FAULT : 1;
  // STATE1
  unsigned char NTC_COLD : 1;
  unsigned char NTC_HOT : 1;
  unsigned char SAFETY_TIMER : 1;
  unsigned char VBAT_OVP : 1;
  unsigned char TSHUT : 1;
  unsigned char VIN_FAULT : 1;
  unsigned char : 2;
  // STATE2
  unsigned char BAT_LOWV : 1;
  unsigned char WORK_MODE : 2;
  unsigned char : 2;
  unsigned char HICCUP_LATCH_STATE : 1;
  unsigned char ULTRA_POWER_MODE : 1;
  unsigned char VBUS_OVP : 1;
} sy6105_state_t;

#define REG_CHIP_ID_VER     0x0C

#define REG_INT_ENABLE_0    0x50
#define WDT_INT_EN          (1<<7)
#define BAT_LOW_INT_EN      (1<<6)

#define REG_INT_STAT0       0x58
typedef struct{
  unsigned char : 6;
  unsigned char BAT_LOW_INT : 1;
  unsigned char WATCH_DOG_INT : 1;
}int_state0_t;

sy_drv_status_t sy6105_get_id_version(unsigned char *id, unsigned char *version);   
sy_drv_status_t sy6105_reg_reset(void);                      
sy_drv_status_t sy6105_sys_soft_reset(void);                             
sy_drv_status_t sy6105_set_vsys_off_time(trst_dur_t trst_dur);           
sy_drv_status_t sy6105_ultra_power_en(unsigned char enable);     
sy_drv_status_t sy6105_enter_shipmode(unsigned char enable);       
sy_drv_status_t sy6105_set_hiz(unsigned char enable);    

sy_drv_status_t sy6105_set_ntc_en(unsigned char enable);            
sy_drv_status_t sy6105_set_ntc_hot(ntc_hot_th_t ntc_hot);           
sy_drv_status_t sy6105_set_thermal_loop_en(unsigned char enable);            
sy_drv_status_t sy6105_wdt_reset(void);    

sy_drv_status_t sy6105_set_wdt_dischg_en(unsigned char enable);          
sy_drv_status_t sy6105_set_wdt_timer(wdt_timer_t wdt_timer);             
sy_drv_status_t sy6105_set_wdt_rst_sys(unsigned char enable);                
sy_drv_status_t sy6105_set_chg_timer_en(unsigned char enable);               
sy_drv_status_t sy6105_set_chg_timer(chg_timer_t chg_timer);                 
sy_drv_status_t sy6105_set_chg_limit_timer_double(unsigned char enable);  
     
sy_drv_status_t sy6105_set_chg_en(unsigned char enable);                      
sy_drv_status_t sy6105_set_ipre(unsigned char ipre, unsigned char ipre_quadruple_en);    
sy_drv_status_t sy6105_set_icc(int icc, unsigned char icc_offset_en);            
sy_drv_status_t sy6105_set_term_en(unsigned char enable);            
sy_drv_status_t sy6105_set_iterm(unsigned char iterm, unsigned char iterm_double);           
sy_drv_status_t sy6105_set_vbat_target(int vbat);            
sy_drv_status_t sy6105_set_vbat_low(bat_low_t vbat_low);         
sy_drv_status_t sy6105_set_vbat_pre(vbat_pre_t vbat_pre);        
sy_drv_status_t sy6105_set_v_rechg(v_rechg_t v_rechg);     

sy_drv_status_t sy6105_set_vbat_uvlo(vbat_uvlo_t vbat_uvlo);             
sy_drv_status_t sy6105_set_batfet_ocp(ibatfet_ocp_t bfet_ocp);           
sy_drv_status_t sy6105_set_vsys_reg(vsys_reg_set_t vsys_reg);          
sy_drv_status_t sy6105_set_vbus_ovp(vbus_ovp_sel_t vbus_ovp_sel);      
sy_drv_status_t sy6105_set_vbus_reset_en(unsigned char enable);          
sy_drv_status_t sy6105_set_input_vol_sleep_det_en(unsigned char enable);   
sy_drv_status_t sy6105_set_vindpm(vindpm_set_t vindpm);              
sy_drv_status_t sy6105_set_iindpm(iindpm_set_t iindpm);   

sy_drv_status_t sy6105_pwrkey_wk_en(unsigned char enable);          
sy_drv_status_t sy6105_pwrkey_wk_time_set(key_wk_time_set_t key_wk_time);          
sy_drv_status_t sy6105_rst_level_set(rst_level_t rst_level);          
sy_drv_status_t sy6105_set_int_en(unsigned char int_en);          
sy_drv_status_t sy6105_int_callback(sy6105_state_t *state,int_state0_t *int_stat);      


sy_drv_status_t sy6105_init(void);

#endif
