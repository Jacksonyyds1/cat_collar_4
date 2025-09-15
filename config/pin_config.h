#ifndef PIN_CONFIG_H
#define PIN_CONFIG_H

// $[USART0]
// USART0 TX on GPIO_15
#ifndef USART0_TX_PORT                          
#define USART0_TX_PORT                           HP
#endif
#ifndef USART0_TX_PIN                           
#define USART0_TX_PIN                            15
#endif
#ifndef USART0_TX_LOC                           
#define USART0_TX_LOC                            4
#endif

// USART0 RX on GPIO_10
#ifndef USART0_RX_PORT                          
#define USART0_RX_PORT                           HP
#endif
#ifndef USART0_RX_PIN                           
#define USART0_RX_PIN                            10
#endif
#ifndef USART0_RX_LOC                           
#define USART0_RX_LOC                            9
#endif

// [USART0]$

// $[UART1]
// [UART1]$

// $[ULP_UART]
// [ULP_UART]$

// $[I2C0]
// I2C0 SCL on GPIO_7
#ifndef I2C0_SCL_PORT                           
#define I2C0_SCL_PORT                            HP
#endif
#ifndef I2C0_SCL_PIN                            
#define I2C0_SCL_PIN                             7
#endif
#ifndef I2C0_SCL_LOC                            
#define I2C0_SCL_LOC                             0
#endif

// I2C0 SDA on GPIO_6
#ifndef I2C0_SDA_PORT                           
#define I2C0_SDA_PORT                            HP
#endif
#ifndef I2C0_SDA_PIN                            
#define I2C0_SDA_PIN                             6
#endif
#ifndef I2C0_SDA_LOC                            
#define I2C0_SDA_LOC                             3
#endif

// [I2C0]$

// $[I2C1]
// I2C1 SCL on GPIO_29
#ifndef I2C1_SCL_PORT                           
#define I2C1_SCL_PORT                            HP
#endif
#ifndef I2C1_SCL_PIN                            
#define I2C1_SCL_PIN                             29
#endif
#ifndef I2C1_SCL_LOC                            
#define I2C1_SCL_LOC                             1
#endif

// I2C1 SDA on GPIO_30
#ifndef I2C1_SDA_PORT                           
#define I2C1_SDA_PORT                            HP
#endif
#ifndef I2C1_SDA_PIN                            
#define I2C1_SDA_PIN                             30
#endif
#ifndef I2C1_SDA_LOC                            
#define I2C1_SDA_LOC                             7
#endif

// [I2C1]$

// $[ULP_I2C]
// ULP_I2C SCL on ULP_GPIO_7/GPIO_71
#ifndef ULP_I2C_SCL_PORT                        
#define ULP_I2C_SCL_PORT                         ULP
#endif
#ifndef ULP_I2C_SCL_PIN                         
#define ULP_I2C_SCL_PIN                          7
#endif
#ifndef ULP_I2C_SCL_LOC                         
#define ULP_I2C_SCL_LOC                          2
#endif

// ULP_I2C SDA on ULP_GPIO_6/GPIO_70
#ifndef ULP_I2C_SDA_PORT                        
#define ULP_I2C_SDA_PORT                         ULP
#endif
#ifndef ULP_I2C_SDA_PIN                         
#define ULP_I2C_SDA_PIN                          6
#endif
#ifndef ULP_I2C_SDA_LOC                         
#define ULP_I2C_SDA_LOC                          6
#endif

// [ULP_I2C]$

// $[SSI_MASTER]
// [SSI_MASTER]$

// $[SSI_SLAVE]
// [SSI_SLAVE]$

// $[ULP_SSI]
// [ULP_SSI]$

// $[GSPI_MASTER]
// GSPI_MASTER SCK_ on GPIO_52
#ifndef GSPI_MASTER_SCK__PORT                   
#define GSPI_MASTER_SCK__PORT                    HP
#endif
#ifndef GSPI_MASTER_SCK__PIN                    
#define GSPI_MASTER_SCK__PIN                     52
#endif
#ifndef GSPI_MASTER_SCK_LOC                     
#define GSPI_MASTER_SCK_LOC                      3
#endif

// GSPI_MASTER CS0_ on GPIO_53
#ifndef GSPI_MASTER_CS0__PORT                   
#define GSPI_MASTER_CS0__PORT                    HP
#endif
#ifndef GSPI_MASTER_CS0__PIN                    
#define GSPI_MASTER_CS0__PIN                     53
#endif
#ifndef GSPI_MASTER_CS0_LOC                     
#define GSPI_MASTER_CS0_LOC                      7
#endif

// GSPI_MASTER MOSI_ on GPIO_57
#ifndef GSPI_MASTER_MOSI__PORT                  
#define GSPI_MASTER_MOSI__PORT                   HP
#endif
#ifndef GSPI_MASTER_MOSI__PIN                   
#define GSPI_MASTER_MOSI__PIN                    57
#endif
#ifndef GSPI_MASTER_MOSI_LOC                    
#define GSPI_MASTER_MOSI_LOC                     19
#endif

// GSPI_MASTER MISO_ on GPIO_56
#ifndef GSPI_MASTER_MISO__PORT                  
#define GSPI_MASTER_MISO__PORT                   HP
#endif
#ifndef GSPI_MASTER_MISO__PIN                   
#define GSPI_MASTER_MISO__PIN                    56
#endif
#ifndef GSPI_MASTER_MISO_LOC                    
#define GSPI_MASTER_MISO_LOC                     24
#endif

// [GSPI_MASTER]$

// $[I2S0]
// [I2S0]$

// $[ULP_I2S]
// [ULP_I2S]$

// $[SCT]
// [SCT]$

// $[SIO]
// [SIO]$

// $[PWM]
// [PWM]$

// $[PWM_CH0]
// [PWM_CH0]$

// $[PWM_CH1]
// [PWM_CH1]$

// $[PWM_CH2]
// [PWM_CH2]$

// $[PWM_CH3]
// [PWM_CH3]$

// $[ADC_CH1]
// [ADC_CH1]$

// $[ADC_CH2]
// [ADC_CH2]$

// $[ADC_CH3]
// [ADC_CH3]$

// $[ADC_CH4]
// [ADC_CH4]$

// $[ADC_CH5]
// [ADC_CH5]$

// $[ADC_CH6]
// [ADC_CH6]$

// $[ADC_CH7]
// [ADC_CH7]$

// $[ADC_CH8]
// [ADC_CH8]$

// $[ADC_CH9]
// [ADC_CH9]$

// $[ADC_CH10]
// [ADC_CH10]$

// $[ADC_CH11]
// [ADC_CH11]$

// $[ADC_CH12]
// [ADC_CH12]$

// $[ADC_CH13]
// [ADC_CH13]$

// $[ADC_CH14]
// [ADC_CH14]$

// $[ADC_CH15]
// [ADC_CH15]$

// $[ADC_CH16]
// [ADC_CH16]$

// $[ADC_CH17]
// [ADC_CH17]$

// $[ADC_CH18]
// [ADC_CH18]$

// $[ADC_CH19]
// [ADC_CH19]$

// $[COMP1]
// [COMP1]$

// $[COMP2]
// [COMP2]$

// $[DAC0]
// [DAC0]$

// $[DAC1]
// [DAC1]$

// $[SYSRTC]
// [SYSRTC]$

// $[UULP_VBAT_GPIO]
// [UULP_VBAT_GPIO]$

// $[GPIO]
// [GPIO]$

// $[QEI]
// [QEI]$

// $[HSPI_SECONDARY]
// [HSPI_SECONDARY]$

// $[CUSTOM_PIN_NAME]
#ifndef _PORT                                   
#define _PORT                                    HP
#endif
#ifndef _PIN                                    
#define _PIN                                     6
#endif

// [CUSTOM_PIN_NAME]$

#endif // PIN_CONFIG_H
