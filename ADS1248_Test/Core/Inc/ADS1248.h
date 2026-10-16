/*
 * ADS1248.h
 *
 *  Created on: Sep 30, 2026
 *      Author: SRIVATHSJAYAN
 */

#ifndef INC_ADS1248_H_
#define INC_ADS1248_H_

#include "main.h"

#include <stdint.h>

#define ADS1248_SPI_DELAY HAL_MAX_DELAY

#define ADS1248_VREF_VLTG_INT 2.048f
//note: need to enter the reference value in ADS1248_VREF_VLTG_EXT if u are going to use a ext reference as source
#define ADS1248_VREF_VLTG_REF0 1.5f
#define ADS1248_VREF_VLTG_REF1 0.0f


#define USE_CONFIG_ADS1248_SET_CALIB_VALUES_AUTOMATICALLY 1
#define USE_CONFIG_ADS1248_ADD_OFFSET_VALUES_MANUALLY 0

/**
 * @brief ADS1248 Register Address Map
 */
typedef enum {
    ADS1248_REG_MUX0    = 0x00,  // Multiplexer Control 0
    ADS1248_REG_VBIAS   = 0x01,  // Bias Voltage Control
    ADS1248_REG_MUX1    = 0x02,  // Multiplexer Control 1
    ADS1248_REG_SYS0    = 0x03,  // System Control 0 (PGA and Data Rate)
    ADS1248_REG_OFC0    = 0x04,  // Offset Calibration Coefficient 0
    ADS1248_REG_OFC1    = 0x05,  // Offset Calibration Coefficient 1
    ADS1248_REG_OFC2    = 0x06,  // Offset Calibration Coefficient 2
    ADS1248_REG_FSC0    = 0x07,  // Full-Scale Calibration Coefficient 0
    ADS1248_REG_FSC1    = 0x08,  // Full-Scale Calibration Coefficient 1
    ADS1248_REG_FSC2    = 0x09,  // Full-Scale Calibration Coefficient 2
    ADS1248_REG_IDAC0   = 0x0A,  // IDAC Control 0 (Magnitude)
    ADS1248_REG_IDAC1   = 0x0B,  // IDAC Control 1 (Routing)
    ADS1248_REG_GPIOCFG = 0x0C,  // GPIO Configuration
    ADS1248_REG_GPIODIR = 0x0D,  // GPIO Direction
    ADS1248_REG_GPIODAT = 0x0E   // GPIO Data
} ADS1248_Reg_t;

typedef struct
{
	float __vref_voltage;
	uint8_t __pga_value;
	float __voltage_lsb;
} ADS1248_Init_t;
typedef struct
{
	SPI_HandleTypeDef *hspi;

	GPIO_TypeDef *CS_Port;
	uint16_t CS_Pin;

	GPIO_TypeDef *RESET_Port;
	uint16_t RESET_Pin;

	GPIO_TypeDef *DRDY_Port;
	uint16_t DRDY_Pin;

	GPIO_TypeDef *START_Port;
	uint16_t START_Pin;

	ADS1248_Init_t Init;

} ADS1248_Handle_t;
typedef enum
{
	ADS1248_BCS_0_UA = 0x00U,
	ADS1248_BCS_0_5_UA = 0x01U,
	ADS1248_BCS_2_UA = 0x02U,
	ADS1248_BCS_10_UA = 0x03U
} ADS1248_Burnout_Current;
typedef enum
{
	ADS1248_MUX_SP_AIN0 = 0x00,
	ADS1248_MUX_SP_AIN1 = 0x01,
	ADS1248_MUX_SP_AIN2 = 0x02,
	ADS1248_MUX_SP_AIN3 = 0x03,
	ADS1248_MUX_SP_AIN4 = 0x04,
	ADS1248_MUX_SP_AIN5 = 0x05,
	ADS1248_MUX_SP_AIN6 = 0x06,
	ADS1248_MUX_SP_AIN7 = 0x07
} Ads1248_Mux_PosInput_t;

typedef enum {
    ADS1248_MUX_SN_AIN0 = 0x00,
    ADS1248_MUX_SN_AIN1 = 0x01,
    ADS1248_MUX_SN_AIN2 = 0x02,
    ADS1248_MUX_SN_AIN3 = 0x03,
    ADS1248_MUX_SN_AIN4 = 0x04,
    ADS1248_MUX_SN_AIN5 = 0x05,
    ADS1248_MUX_SN_AIN6 = 0x06,
    ADS1248_MUX_SN_AIN7 = 0x07
} Ads1248_Mux_NegInput_t;

typedef enum {
    ADS1248_MUXCAL_NORMAL       = 0x00,
    ADS1248_MUXCAL_OFFSET_CALIB = 0x01,
    ADS1248_MUXCAL_GAIN_CALIB   = 0x02,
    ADS1248_MUXCAL_TEMP         = 0x03,
    ADS1248_MUXCAL_REF1         = 0x04,
    ADS1248_MUXCAL_REF0         = 0x05,
    ADS1248_MUXCAL_ANALOG_SUP   = 0x06,
    ADS1248_MUXCAL_DIGITAL_SUP  = 0x07
} ADS1248_MuxCal_t;

typedef enum {
    ADS1248_REFSEL_REFP0_REFN0     = 0x00,
    ADS1248_REFSEL_REFP1_REFN1     = 0x01,
    ADS1248_REFSEL_INT             = 0x02,
    ADS1248_REFSEL_INT_REFP0_REFN0 = 0x03
} Ads1248_Reference_Src_t;

/* PGA Gain Settings (SYS0 Register bits [6:4]) */
typedef enum {
    ADS1248_PGA_1   = 0x00,
    ADS1248_PGA_2   = 0x01,
    ADS1248_PGA_4   = 0x02,
    ADS1248_PGA_8   = 0x03,
    ADS1248_PGA_16  = 0x04,
    ADS1248_PGA_32  = 0x05,
    ADS1248_PGA_64  = 0x06,
    ADS1248_PGA_128 = 0x07
} ADS1248_GainPga_t;

/* Data Rate Settings (SYS0 Register bits [3:0]) */
typedef enum {
    ADS1248_DATARATE_5SPS    = 0x00,
    ADS1248_DATARATE_10SPS   = 0x01,
    ADS1248_DATARATE_20SPS   = 0x02,
    ADS1248_DATARATE_40SPS   = 0x03,
    ADS1248_DATARATE_80SPS   = 0x04,
    ADS1248_DATARATE_160SPS  = 0x05,
    ADS1248_DATARATE_320SPS  = 0x06,
    ADS1248_DATARATE_640SPS  = 0x07,
    ADS1248_DATARATE_1000SPS = 0x08,
    ADS1248_DATARATE_2000SPS = 0x09
} ADS1248_DataRate_t;

typedef enum {
    ADS1248_IMAG_OFF     = 0x00,
    ADS1248_IMAG_50_UA   = 0x01,
    ADS1248_IMAG_100_UA  = 0x02,
    ADS1248_IMAG_250_UA  = 0x03,
    ADS1248_IMAG_500_UA  = 0x04,
    ADS1248_IMAG_750_UA  = 0x05,
    ADS1248_IMAG_1000_UA = 0x06,
    ADS1248_IMAG_1500_UA = 0x07
} ADS1248_IdacMagnitude_t;

typedef enum
{
	ADS1248_VREFCON_INT_ALWAYS_OFF = 0x00,
	ADS1248_VREFCON_INT_ALWAYS_ON = 0x01,
	ADS1248_VREFCON_INT_CONV_ON = 0x02
} ADS1248_Vref_State;

typedef enum
{
	ADS1248_DRDY_MODE_DATA_OUT_ONLY = 0x00U,
	ADS1248_DRDY_MODE_DATA_RDY_OUT = 0x01U

} Ads1248_Drdy;
typedef enum
{
	ADS1248_EX_CUR_OUT_AIN0 = 0x00,
	ADS1248_EX_CUR_OUT_AIN1 = 0x01,
	ADS1248_EX_CUR_OUT_AIN2 = 0x02,
	ADS1248_EX_CUR_OUT_AIN3 = 0x03,
	ADS1248_EX_CUR_OUT_AIN4 = 0x04,
	ADS1248_EX_CUR_OUT_AIN5 = 0x05,
	ADS1248_EX_CUR_OUT_AIN6 = 0x06,
	ADS1248_EX_CUR_OUT_AIN7 = 0x07,
	ADS1248_EX_CUR_OUT_IEXC1 = 0x08,
	ADS1248_EX_CUR_OUT_IEXC2 = 0x09,
	ADS1248_EX_CUR_OUT_DISCON = 0x0F
} ADS1248_Exci_Pin_t;

typedef enum
{
	ADS1248_GPIO_RESET = 0U,
	ADS1248_GPIO_SET = 1U
} ADS1248_Gpio_Pin_State_t;

#define ADS1248_PIN_REFP0     (1U << 0U)
#define ADS1248_PIN_REFN0     (1U << 1U)
#define ADS1248_PIN_AIN_2     (1U << 2U)
#define ADS1248_PIN_AIN_3     (1U << 3U)
#define ADS1248_PIN_AIN_4     (1U << 4U)
#define ADS1248_PIN_AIN_5     (1U << 5U)
#define ADS1248_PIN_AIN_6     (1U << 6U)
#define ADS1248_PIN_AIN_7     (1U << 7U)

#define ADS1248_VBIAS_AIN0     (1U << 0U)
#define ADS1248_VBIAS_AIN1     (1U << 1U)
#define ADS1248_VBIAS_AIN2     (1U << 2U)
#define ADS1248_VBIAS_AIN3     (1U << 3U)
#define ADS1248_VBIAS_AIN4     (1U << 4U)
#define ADS1248_VBIAS_AIN5     (1U << 5U)
#define ADS1248_VBIAS_AIN6     (1U << 6U)
#define ADS1248_VBIAS_AIN7     (1U << 7U)

#define ADS1248_AIN_MODE_OUTPUT  0U
#define ADS1248_AIN_MODE_INPUT   1U
#define ADS1248_AIN_MODE_DISABLE 2U

#define ADS1248_IDAC_1           0U
#define ADS1248_IDAC_2           1U

#define __ADS1248_BSC_POS       6U
#define __ADS1248_MUX_SP_POS    3U
#define __ADS1248_MUX_SN_POS    0U
#define __ADS1248_MUXCAL_POS    0U
#define __ADS1248_REFSELT_POS   3U
#define __ADS1248_VREFCON_POS   5U
#define __ADS1248_CLKSTAT_POS   7U
#define __ADS1248_PGA_POS       4U
#define __ADS1248_DR_POS        0U
#define __ADS1248_IMAG_POS      0U
#define __ADS1248_DRDY_MODE_POS 3U
#define __ADS1248_I1DIR_POS     4U
#define __ADS1248_I2DIR_POS     0U

#define __ADS1248_CLEAR_MUX_SP ~0x38U
#define __ADS1248_CLEAR_MUX_NP ~0x07U
#define __ADS1248_CLEAR_MUXCAL ~0x07U
#define __ADS1248_CLEAR_REFSELT ~0x18U
#define __ADS1248_CLEAR_PGA ~0x70U
#define __ADS1248_CLEAR_DR ~0x0FU
#define __ADS1248_CLEAR_IMAG ~0x07U
#define __ADS1248_CLEAR_VREFCON ~0x60U
#define __ADS1248_CLEAR_BSC ~0xC0U
#define __ADS1248_CLEAR_DRDY ~0x08U
#define __ADS1248_CLEAR_I1DIR ~0xF0U
#define __ADS1248_CLEAR_I2DIR ~0x0FU

#define __ADS1248_CLCKSTAT_MASK (1<<__ADS1248_CLKSTAT_POS)
#define __ADS1248_TEMP_LSB(ADC_24)   (25U + ((((ADC_24*ADS1248_VREF_VLTG_INT)/0x7FFFFF) - 0.118f)/0.000405f))

/* System Commands */
#define ADS1248_CMD_WAKEUP      0x00U  /* Exit sleep mode                     */
#define ADS1248_CMD_SLEEP       0x02U  /* Enter low-power sleep mode          */
#define ADS1248_CMD_SYNC        0x04U  /* Synchronize ADC conversions         */
#define ADS1248_CMD_RESET       0x06U  /* Reset ADC to default power-up state */
#define ADS1248_CMD_NOP         0xFFU  /* No operation                        */

/* Data Read Commands */
#define ADS1248_CMD_RDATA       0x12U  /* Read data once by command           */
#define ADS1248_CMD_RDATAC      0x14U  /* Read data continuous mode           */
#define ADS1248_CMD_SDATAC      0x16U  /* Stop read data continuous mode      */

/* Calibration Commands */
#define __ADS1248_CMD_SYSOCAL     0x60U  /* System offset calibration           */
#define __ADS1248_CMD_SYSGCAL     0x61U  /* System gain calibration             */
#define __ADS1248_CMD_SELFOCAL    0x62U  /* Self offset calibration             */

/* Register Read / Write Command Base Masks */
#define __ADS1248_CMD_RREG_BASE   0x20U  /* Read register command base (0010 rrrr)  */
#define __ADS1248_CMD_WREG_BASE   0x40U  /* Write register command base (0100 rrrr) */

#define __ADS1248_CMD_RREG(r) (uint8_t)(__ADS1248_CMD_RREG_BASE | r)
#define __ADS1248_CMD_WREG(r) (uint8_t)(__ADS1248_CMD_WREG_BASE | r)
#define __ADS1248_CMD_W_R_REG_COUNT(n) (uint8_t)(n-1U)

#define __ADS1248_PIN_MASK 0xFFU

#define __IS_ADS1248_GPIO_PIN(PIN) ((PIN & __ADS1248_PIN_MASK) != 0x00U)

#if USE_CONFIG_ADS1248_SET_CALIB_VALUES_AUTOMATICALLY
HAL_StatusTypeDef ADS1248_Set_System_Offset_Calib(ADS1248_Handle_t *hads);
HAL_StatusTypeDef ADS1248_Set_Gain_Calib(ADS1248_Handle_t *hads);
#else
HAL_StatusTypeDef ADS1248_Set_Offset_Calib(ADS1248_Handle_t *hads, uint32_t *pData32);
HAL_StatusTypeDef ADS1248_Set_FullScale_Calib(ADS1248_Handle_t *hads, uint32_t *pData32);
#endif

#if USE_CONFIG_ADS1248_ADD_OFFSET_VALUES_MANUALLY
HAL_StatusTypeDef ADS1248_Get_Voltage_V_Man_Offset(ADS1248_Handle_t *hads,int32_t *pData32,float *pData);
#else
HAL_StatusTypeDef ADS1248_Get_Voltage_V(ADS1248_Handle_t *hads,float *pData);
#endif

void ADS1248_Init(ADS1248_Handle_t *hads);
HAL_StatusTypeDef ADS1248_Set_Vltg_Calc_LSB(ADS1248_Handle_t *hads,float vref_vltg);
HAL_StatusTypeDef ADS1248_Reset_Hard(ADS1248_Handle_t *hads);
HAL_StatusTypeDef ADS1248_Trigger_Start_Pin(ADS1248_Handle_t *hads);
HAL_StatusTypeDef ADS1248_ReadReg_Once(ADS1248_Handle_t *hads,ADS1248_Reg_t Ads1248_Reg,uint8_t *pData);
HAL_StatusTypeDef ADS1248_ReadReg_n(ADS1248_Handle_t *hads,ADS1248_Reg_t Ads1248_Reg,uint8_t n, uint8_t *pData);
HAL_StatusTypeDef ADS1248_Command_Tx(ADS1248_Handle_t *hads, uint8_t Ads1248_Cmd);
HAL_StatusTypeDef ADS1248_Set_Burnout_Current(ADS1248_Handle_t *hads,ADS1248_Burnout_Current Ads1248_Bsc);
HAL_StatusTypeDef ADS1248_Set_Input_Channel(ADS1248_Handle_t *hads,Ads1248_Mux_PosInput_t Ads1248_Mux_Sp,Ads1248_Mux_NegInput_t Ads1248_Mux_Sn);
HAL_StatusTypeDef ADS1248_Set_VBIAS(ADS1248_Handle_t *hads,uint8_t Ads1248_Vbias,uint8_t En_Di);
HAL_StatusTypeDef ADS1248_Set_Vref_State(ADS1248_Handle_t *hads,ADS1248_Vref_State Ads1248_Vrefcon);
HAL_StatusTypeDef ADS1248_Set_Vref_Source(ADS1248_Handle_t *hads,Ads1248_Reference_Src_t Ads1248_Refsel);
HAL_StatusTypeDef ADS1248_Set_System_Monitor(ADS1248_Handle_t *hads,ADS1248_MuxCal_t Ads1248_MuxCal);
HAL_StatusTypeDef ADS1248_Set_PGA(ADS1248_Handle_t *hads,ADS1248_GainPga_t Ads1248_Pga);
HAL_StatusTypeDef ADS1248_Set_Data_Rate(ADS1248_Handle_t *hads,ADS1248_DataRate_t Ads1248_Datarate);
HAL_StatusTypeDef ADS1248_Set_Drdy_Mode(ADS1248_Handle_t *hads, Ads1248_Drdy Ads1248_Drdy_Mode);
HAL_StatusTypeDef ADS1248_Set_Exci_Current_Mag(ADS1248_Handle_t *hads,ADS1248_IdacMagnitude_t Ads1248_Idac0);
HAL_StatusTypeDef ADS1248_Set_Exci_Current_Pin(ADS1248_Handle_t *hads,uint8_t Ads1248_idac,ADS1248_Exci_Pin_t Ads1248_Ex_Cur);
HAL_StatusTypeDef ADS1248_Set_GPIO_Mode(ADS1248_Handle_t *hads,uint8_t Ads1248_Pin,uint8_t Ads1248_Ain_Mode);
HAL_StatusTypeDef ADS1248_GPIO_Write(ADS1248_Handle_t *hads,uint8_t Ads1248_Pin,ADS1248_Gpio_Pin_State_t Ads1248_Gpio);
uint8_t ADS1248_GPIO_Read(ADS1248_Handle_t *hads,uint8_t Ads1248_Pin);
HAL_StatusTypeDef ADS1248_Set_Self_Calib(ADS1248_Handle_t *hads);
uint8_t ADS1248_Get_Clock_Status(ADS1248_Handle_t *hads);
HAL_StatusTypeDef ADS1248_Get_Temp_C(ADS1248_Handle_t *hads,float *pData);

#endif /* INC_ADS1248_H_ */
