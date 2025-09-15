#ifndef _SY6105_CFG_H
#define _SY6105_CFG_H

#include "sy6105_bsp.h"

#define IPRE_CFG        11
#define IPRE_QUAD       false
#define ICC             128
#define ICC_OFFSET      false
#define ITERM           5
#define ITERM_DOUBLE    false
#define TERM_EN         false
#define VBAT_TARGET     4210
#define VBAT_LOW        BAT_LOW_3V3
#define VBAT_PRE        VBAT_RRE_3V0
#define VBAR_RECHG      VRE_CHG_100mV

#define VBAT_UVLO       VBAT_UVLO_2V82
#define BFET_OCP        IBATFET_OCP_2000mA
#define VSYS_VOL        VSYS_REG_SET_4V55

#define VBUS_OVP        VBUS_OVP_SEL_6V0
#define VINDPM          VINDPM_SET_4V6
#define IINDPM          IINDPM_SET_500MA
#define VBUS_RST        false

#define INT_EN          (CHG_DONE_INT_EN | PG_INT_CONTROL | BAT_LOW_INT_EN)

#endif
