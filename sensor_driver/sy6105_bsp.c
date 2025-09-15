#include "sy6105_bsp.h"
#include "sy6105_cfg.h"

#include "common_i2c.h"
#include "app_log.h"

#define SY6105_ADDR         0x07
#define I2C_INSTANCE_USED   INSTANCE_ZERO

unsigned char SY6105_ID = (0x07<<1);

#ifdef STM32L031xx
	extern I2C_HandleTypeDef hi2c1;
	#define sy_rx_nbyte(reg_addr, data_p, data_size)  	(sy_drv_status_t)HAL_I2C_Mem_Read(&hi2c1, SY6105_ID, reg_addr, 1, data_p, data_size, 0x1000);
	#define sy_tx_nbyte(reg_addr, data_p, data_size)		(sy_drv_status_t)HAL_I2C_Mem_Write(&hi2c1, SY6105_ID, reg_addr, 1, data_p, data_size, 0x1000);
#else
    #define sy_rx_nbyte(reg_addr, data_p, data_size)  	\
            (sy_drv_status_t)platform_read(I2C_INSTANCE_USED, SY6105_ADDR, NULL, reg_addr, (uint8_t *)data_p, data_size);
	#define sy_tx_nbyte(reg_addr, data_p, data_size)	\
            (sy_drv_status_t)platform_write(I2C_INSTANCE_USED, SY6105_ADDR, NULL, reg_addr, (const uint8_t *)data_p, data_size);
#endif

#if SY6105_DEBUG
#define sy6105_err(str) 	app_log_error("[%s:%d] err:%s\r\n",__FILE__,__LINE__,str)
#define sy6105_print(str) 	app_log_debug("%s\r\n",str)
#else
	#define sy6105_print(str) 	NULL
#endif

static sy_drv_status_t set_reg_bit(unsigned char reg_addr, unsigned char bit_start, unsigned char bit_num, unsigned char val)
{
    sy_drv_status_t ret = RET_SUCCESS;
    unsigned char reg_val = 0;
    unsigned char clear_zero = 0;

		ret = sy_rx_nbyte(reg_addr, &reg_val, 1);
    if(ret){
        return ret;
    }
    
		for(; bit_num > 0; bit_num--){
			clear_zero <<= 1;
      clear_zero = clear_zero | 0x01;
    }
		
		clear_zero <<= bit_start;
		
		
//    val = val << (bit_start - (bit_num - 1));
//    for(; bit_num > 0; bit_num--){
//        clear_zero = clear_zero | (1 << bit_start);
//        bit_start--;
//    }
    clear_zero = ~clear_zero;
    reg_val = reg_val & clear_zero;
		
    reg_val = reg_val | (val<<bit_start);
    
		ret = sy_tx_nbyte(reg_addr, &reg_val, 1);
    if(ret){
        return ret;
    }    

    return RET_SUCCESS;
}

sy_drv_status_t sy6105_reg_reset(void)                      
{
    sy_drv_status_t ret;
    ret = set_reg_bit(REG_SYS_CTRL1, 7, 1, SY_ENABLE);
    return ret;
}

sy_drv_status_t sy6105_sys_soft_reset(void)                             
{
    return set_reg_bit(REG_SYS_CTRL4, 7, 1, SY_ENABLE);
}

sy_drv_status_t sy6105_set_vsys_off_time(trst_dur_t trst_dur)           
{
    return set_reg_bit(REG_SYS_CTRL0, 5, 1, trst_dur);
}

sy_drv_status_t sy6105_ultra_power_en(unsigned char enable)     
{
    return set_reg_bit(REG_SYS_CTRL2, 1, 1, enable?SY_ENABLE:SY_DISABLE);
}

sy_drv_status_t sy6105_enter_shipmode(unsigned char enable)                     
{
    return set_reg_bit(REG_SYS_CTRL3, 5, 1, enable?SY_ENABLE:SY_DISABLE);
}

sy_drv_status_t sy6105_set_ntc_en(unsigned char enable)            
{
    return set_reg_bit(REG_SYS_CTRL3, 7, 1, enable?SY_ENABLE:SY_DISABLE);
}

sy_drv_status_t sy6105_set_ntc_hot(ntc_hot_th_t ntc_hot)           
{
    return set_reg_bit(REG_SYS_CTRL4, 5, 2, ntc_hot);
}

sy_drv_status_t sy6105_set_thermal_loop_en(unsigned char enable)            
{
    return set_reg_bit(REG_SYS_CTRL4, 4, 1, enable?SY_ENABLE:SY_DISABLE);
}

sy_drv_status_t sy6105_wdt_reset(void)                          
{
    return set_reg_bit(REG_SYS_CTRL1, 6, 1, 1);
}

sy_drv_status_t sy6105_set_wdt_dischg_en(unsigned char enable)          
{
    return set_reg_bit(REG_SYS_CTRL2, 7, 1, enable?SY_ENABLE:SY_DISABLE);
}

sy_drv_status_t sy6105_set_wdt_timer(wdt_timer_t wdt_timer)             
{
    return set_reg_bit(REG_SYS_CTRL2, 5, 2, wdt_timer);
}

sy_drv_status_t sy6105_set_wdt_rst_sys(unsigned char enable)                
{
    return set_reg_bit(REG_SYS_CTRL4, 3, 1, enable?SY_ENABLE:SY_DISABLE);
}

sy_drv_status_t sy6105_set_chg_timer_en(unsigned char enable)               
{
    return set_reg_bit(REG_SYS_CTRL2, 3, 1, enable?SY_ENABLE:SY_DISABLE);
}

sy_drv_status_t sy6105_set_chg_timer(chg_timer_t chg_timer)                 
{
    return set_reg_bit(REG_SYS_CTRL2, 2, 1, chg_timer);
}

sy_drv_status_t sy6105_set_chg_limit_timer_double(unsigned char enable)         
{
    return set_reg_bit(REG_SYS_CTRL3, 6, 1, enable?SY_ENABLE:SY_DISABLE);
}

sy_drv_status_t sy6105_get_id_version(unsigned char *id, unsigned char *version)        
{
    unsigned char reg_val;
    sy_drv_status_t ret;
    ret = sy_rx_nbyte(REG_CHIP_ID_VER, &reg_val, 1);
    if(ret){
        app_log_error("sy6105:read reg err\r\n");
        return ret;
    }
    app_log_info("sy6105->CHIP ID->0x%02x\r\n", reg_val);
   *id = reg_val & 0x07;
   *version = reg_val >> 5; 

   return RET_SUCCESS;
}

sy_drv_status_t sy6105_set_hiz(unsigned char enable)                        
{
    return set_reg_bit(REG_SYS_CTRL0, 4, 1, enable?SY_ENABLE:SY_DISABLE);
}

sy_drv_status_t sy6105_set_chg_en(unsigned char enable)                      
{
    return set_reg_bit(REG_SYS_CTRL0, 3, 1 ,enable?SY_DISABLE:SY_ENABLE);
}

sy_drv_status_t sy6105_set_ipre(unsigned char ipre, unsigned char ipre_quadruple_en)    
{
    unsigned char reg_val;
    sy_drv_status_t ret;
    if(ipre > 33){
        return RET_PAR_ERR;
    }

    ret = set_reg_bit(REG_IDSG_ITERM_SET, 5, 1 , ipre_quadruple_en?SY_ENABLE:SY_DISABLE);

    if(ret){
        return ret;
    }

    reg_val = (ipre - 1)/2;

    return set_reg_bit(REG_DPM_SET, 2, 4, reg_val);
}


sy_drv_status_t sy6105_set_icc(int icc, unsigned char icc_offset_en)            
{
    uint8_t reg_val;
    sy_drv_status_t ret;
    if((icc > 792 && icc_offset_en == SY_DISABLE) || (icc > (792+248) && icc_offset_en == SY_ENABLE) || (icc < 256 && icc_offset_en == SY_ENABLE)){
        return RET_PAR_ERR;
    }
    
    ret = set_reg_bit(REG_SYS_CTRL0, 2, 1 , icc_offset_en?SY_ENABLE:SY_DISABLE);

    if(ret){
        return ret;
    }

    if(icc_offset_en){
        if(icc - 248 <= 456){
            reg_val = (icc - 248 - 8) / 8;
        }
        else{
            reg_val = (icc - 248 -456) / 48 + 56;
        }
    }
    else{
        if(icc < 456){
            reg_val = (icc - 8) / 8; 
        }
        else{
            reg_val = (icc - 456) / 48 + 56;
        }
    }

    return set_reg_bit(REG_SYS_CTRL1, 0, 5, reg_val);
}

sy_drv_status_t sy6105_set_term_en(unsigned char enable)            
{
    return set_reg_bit(REG_SYS_CTRL2, 4, 1, enable?SY_ENABLE:SY_DISABLE);
}

sy_drv_status_t sy6105_set_iterm(unsigned char iterm, unsigned char iterm_double)           
{
    unsigned char reg_val;
    sy_drv_status_t ret;
    if(iterm > 33){
        return RET_PAR_ERR;
    }

    ret = set_reg_bit(REG_IDSG_ITERM_SET, 4, 1 , iterm_double?SY_ENABLE:SY_DISABLE);

    if(ret){
        return ret;
    }

    reg_val = (iterm - 1)/2;

    return set_reg_bit(REG_IDSG_ITERM_SET, 0, 4, reg_val);
}

sy_drv_status_t sy6105_set_vbat_target(int vbat)              
{
    unsigned char reg_val;

    if(vbat < 4000 || vbat > 4590){
        return RET_PAR_ERR;
    }

    if(vbat < 4350){
        reg_val = (vbat - 4000) / 50;
    }
    else{
        reg_val = (vbat - 4350) / 10 + 7;
    }

    return set_reg_bit(REG_BAT_VOL_SET, 3, 5, reg_val);
}

sy_drv_status_t sy6105_set_vbat_low(bat_low_t vbat_low)         
{
    return set_reg_bit(REG_BAT_VOL_SET, 2, 1, vbat_low);
}

sy_drv_status_t sy6105_set_vbat_pre(vbat_pre_t vbat_pre)        
{
    return set_reg_bit(REG_BAT_VOL_SET, 1, 1, vbat_pre);
}

sy_drv_status_t sy6105_set_v_rechg(v_rechg_t v_rechg)           
{
    return set_reg_bit(REG_BAT_VOL_SET, 0, 1, v_rechg);
}

sy_drv_status_t sy6105_set_vbat_uvlo(vbat_uvlo_t vbat_uvlo)             
{
    return set_reg_bit(REG_SYS_CTRL0, 0, 2, vbat_uvlo);
}

sy_drv_status_t sy6105_set_batfet_ocp(ibatfet_ocp_t bfet_ocp)           
{
    return set_reg_bit(REG_IDSG_ITERM_SET, 6, 2, bfet_ocp);
}

sy_drv_status_t sy6105_set_vsys_reg(vsys_reg_set_t vsys_reg)          
{
    return set_reg_bit(REG_SYS_CTRL4, 0, 2, vsys_reg);
}

sy_drv_status_t sy6105_set_vbus_ovp(vbus_ovp_sel_t vbus_ovp_sel)      
{
    return set_reg_bit(REG_SYS_CTRL5, 1, 1, vbus_ovp_sel);
}

sy_drv_status_t sy6105_set_vbus_reset_en(unsigned char enable)          
{
    return set_reg_bit(REG_SYS_CTRL2, 0, 1, enable?SY_ENABLE:SY_DISABLE);
}

sy_drv_status_t sy6105_set_input_vol_sleep_det_en(unsigned char enable)   
{
    return set_reg_bit(REG_SYS_CTRL5, 2, 1, enable?SY_ENABLE:SY_DISABLE);
}

sy_drv_status_t sy6105_set_vindpm(vindpm_set_t vindpm)              
{
    return set_reg_bit(REG_DPM_SET, 6, 2, vindpm);
}

sy_drv_status_t sy6105_set_iindpm(iindpm_set_t iindpm)          
{
    return set_reg_bit(REG_DPM_SET, 0, 2, iindpm);
}

sy_drv_status_t sy6105_pwrkey_wk_en(unsigned char enable)          
{
    return set_reg_bit(REG_SYS_CTRL5, 0, 1, enable?SY_ENABLE:SY_DISABLE);
}

sy_drv_status_t sy6105_pwrkey_wk_time_set(key_wk_time_set_t key_wk_time)          
{
    return set_reg_bit(REG_SYS_CTRL4, 2, 1, key_wk_time);
}

sy_drv_status_t sy6105_rst_level_set(rst_level_t rst_level)          
{
    return set_reg_bit(REG_SYS_CTRL0, 6, 2, rst_level);
}

sy_drv_status_t sy6105_set_int_en(unsigned char int_en)          
{
    sy_drv_status_t ret;

    ret = set_reg_bit(REG_SYS_CTRL3, 0, 5, int_en & 0x1F);
    if(ret){
        return ret;
    }

    return set_reg_bit(REG_INT_ENABLE_0, 6, 2, int_en >> 6);
}

sy_drv_status_t sy6105_int_callback(sy6105_state_t *state,int_state0_t *int_stat)           
{
    sy_drv_status_t ret;
    ret = sy_rx_nbyte(REG_CHIP_STAT0 ,(uint8_t*)state, 2);
    if(ret){
        return ret;
    }
    ret = sy_rx_nbyte(REG_CHIP_STAT2 ,(uint8_t*)state + 2, 1);
    if(ret){
        return ret;
    }

    return sy_rx_nbyte(REG_INT_STAT0 ,(uint8_t*)int_stat, 1);
}

static void delay_ms(int ms)
{
    platform_delay(ms);
}

sy_drv_status_t sy6105_init(void)
{
    unsigned char id,version;
	sy_drv_status_t ret;

    ret = sy6105_reg_reset();
    if(ret){
        return ret;
    }
    delay_ms(2);
    ret = sy6105_get_id_version(&id, &version);
    if(ret){
        return ret;
    }

    ret = sy6105_set_ipre(IPRE_CFG, IPRE_QUAD);
    if(ret){
        return ret;
    }

    ret = sy6105_set_icc(ICC, ICC_OFFSET);
    if(ret){
        return ret;
    }

    ret = sy6105_set_iterm(ITERM, ITERM_DOUBLE);
    if(ret){
        return ret;
    }

    ret = sy6105_set_term_en(TERM_EN);
    if(ret){
        return ret;
    }

    ret = sy6105_set_vbat_target(VBAT_TARGET);
    if(ret){
        return ret;
    }

    ret = sy6105_set_vbat_low(VBAT_LOW);
    if(ret){
        return ret;
    }

    ret = sy6105_set_vbat_pre(VBAT_PRE);
    if(ret){
        return ret;
    }

    ret = sy6105_set_v_rechg(VBAR_RECHG);
    if(ret){
        return ret;
    }

    ret = sy6105_set_vbat_uvlo(VBAT_UVLO);
    if(ret){
        return ret;
    }

    ret = sy6105_set_batfet_ocp(BFET_OCP);
    if(ret){
        return ret;
    }

    ret = sy6105_set_vsys_reg(VSYS_VOL);
    if(ret){
        return ret;
    }

    ret = sy6105_set_vbus_ovp(VBUS_OVP);
    if(ret){
        return ret;
    }


    ret = sy6105_set_vindpm(VINDPM);
    if(ret){
        return ret;
    }

    ret = sy6105_set_iindpm(IINDPM);
    if(ret){
        return ret;
    }

    ret = sy6105_set_vbus_reset_en(VBUS_RST);
    if(ret){
        return ret;
    }

    ret = sy6105_set_int_en(INT_EN);
    if(ret){
        return ret;
    }

    ret = sy6105_set_chg_en(true);
    if(ret){
        return ret;
    }

	return ret;
}

