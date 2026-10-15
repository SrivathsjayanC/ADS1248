/*
 * ADS1248.c
 *
 *  Created on: Sep 30, 2026
 *      Author: SRIVATHSJAYAN
 */
#include "ADS1248.h"
#include "delay_us.h"

#if !USE_CONFIG_ADS1248_SET_CALIB_VALUES_AUTOMATICALLY
static HAL_StatusTypeDef ADS1248_WriteReg_3(ADS1248_Handle_t *hads,ADS1248_Reg_t Ads1248_Reg,uint32_t *pData32);
#endif
HAL_StatusTypeDef ADS1248_WriteReg_Once(ADS1248_Handle_t *hads,ADS1248_Reg_t Ads1248_Reg,uint8_t Data);
static uint32_t __ADS1248_Calc_Calib_Delay(ADS1248_Handle_t *hads);

HAL_StatusTypeDef ADS1248_WriteReg_3(ADS1248_Handle_t *hads,ADS1248_Reg_t Ads1248_Reg,uint32_t *pData32)
{
	uint8_t tx[5];
	HAL_StatusTypeDef status;

	tx[0] = __ADS1248_CMD_WREG(Ads1248_Reg);
	tx[1] = __ADS1248_CMD_W_R_REG_COUNT(3U);
	tx[2] = (uint8_t)(*pData32>>16);
	tx[3] = (uint8_t)(*pData32>>8);
	tx[4] = (uint8_t)(*pData32);

	hads->CS_Port->BSRR = hads->CS_Pin << 16U;
	delay_us(1);
	status = HAL_SPI_Transmit(hads->hspi, tx, 5U, ADS1248_SPI_DELAY);
	delay_us(3);
	hads->CS_Port->BSRR = hads->CS_Pin;
	if(status != HAL_OK)
	{
		return status;
	}

	return status;
}
// setting values for voltage calculation ADS1248_Get_Data
void ADS1248_Init(ADS1248_Handle_t *hads)
{
	hads->Init.__pga_value = 1U;
	hads->Init.__vref_voltage = 2.048f;
	hads->Init.__voltage_lsb = (double) ((hads->Init.__vref_voltage/(hads->Init.__pga_value) / 0x7FFFFFU));
}
// used to caolulate the LSB Value for converting the raw 24bit data into voltage value
HAL_StatusTypeDef ADS1248_Set_Vltg_Calc_LSB(ADS1248_Handle_t *hads,float ads1248_vref_vltg)
{
	if(hads == NULL || ads1248_vref_vltg < 0.5f || ads1248_vref_vltg > 4.25f)
	{
		return HAL_ERROR;
	}
	HAL_StatusTypeDef status;
	uint8_t reg;
	status = ADS1248_ReadReg_Once(hads, ADS1248_REG_SYS0, &reg);
	if(status != HAL_OK)
	{
		return status;
	}
	switch((reg & (0x7<<__ADS1248_PGA_POS))>>__ADS1248_PGA_POS)
	{
		case ADS1248_PGA_1:
			hads->Init.__pga_value = 1U;
			break;
		case ADS1248_PGA_2:
			hads->Init.__pga_value = 2U;
			break;
		case ADS1248_PGA_4:
			hads->Init.__pga_value = 4U;
			break;
		case ADS1248_PGA_8:
			hads->Init.__pga_value = 8U;
			break;
		case ADS1248_PGA_16:
			hads->Init.__pga_value = 16U;
			break;
		case ADS1248_PGA_32:
			hads->Init.__pga_value = 32U;
			break;
		case ADS1248_PGA_64:
			hads->Init.__pga_value = 64U;
			break;
		case ADS1248_PGA_128:
			hads->Init.__pga_value = 128U;
			break;
		default:
			return HAL_ERROR;
	}

	hads->Init.__vref_voltage = ads1248_vref_vltg;

	hads->Init.__voltage_lsb = (double) ((hads->Init.__vref_voltage/(hads->Init.__pga_value) / 0x7FFFFF));

	return status;
}
HAL_StatusTypeDef ADS1248_Reset_Hard(ADS1248_Handle_t *hads)
{
	if(hads->RESET_Port == NULL)
	{
		return HAL_ERROR;
	}
	hads->RESET_Port->BSRR = hads->RESET_Pin << 16U;
	HAL_Delay(1U);
	hads->RESET_Port->BSRR = hads->RESET_Pin;
	HAL_Delay(1U);
	return HAL_OK;
}
// Delay of 0.7us is required between start pulses so a 1us is used
// because fCLK of ADS1248 is 4.096MHz
//note:  Do not combine using the START pin and using commands to control conversions
HAL_StatusTypeDef ADS1248_Trigger_Start_Pin(ADS1248_Handle_t *hads)
{
	if(hads->START_Port == NULL)
	{
		return HAL_ERROR;
	}
	hads->START_Port->BSRR = hads->START_Pin;
	delay_us(1U);
	hads->START_Port->BSRR = hads->START_Pin<<16;

	return HAL_OK;
}

HAL_StatusTypeDef ADS1248_WriteReg_Once(ADS1248_Handle_t *hads,ADS1248_Reg_t Ads1248_Reg,uint8_t Data)
{
	uint8_t tx[3];
	HAL_StatusTypeDef status;

	tx[0] = __ADS1248_CMD_WREG(Ads1248_Reg);
	tx[1] = __ADS1248_CMD_W_R_REG_COUNT(1U);
	tx[2] = Data;

	hads->CS_Port->BSRR = hads->CS_Pin << 16U;
	delay_us(1);
	status = HAL_SPI_Transmit(hads->hspi, tx, 3U, ADS1248_SPI_DELAY);
	delay_us(3);
	hads->CS_Port->BSRR = hads->CS_Pin;
	if(status != HAL_OK)
	{
		return status;
	}

	return status;
}
HAL_StatusTypeDef ADS1248_ReadReg_Once(ADS1248_Handle_t *hads,ADS1248_Reg_t Ads1248_Reg,uint8_t *pData)
{
	uint8_t tx[3];
	uint8_t rx[3];
	HAL_StatusTypeDef status;

	tx[0] = __ADS1248_CMD_RREG(Ads1248_Reg);
	tx[1] = __ADS1248_CMD_W_R_REG_COUNT(1U);
	tx[2] = ADS1248_CMD_NOP;

	hads->CS_Port->BSRR = hads->CS_Pin << 16U;
	delay_us(1);
	status = HAL_SPI_TransmitReceive(hads->hspi, tx, rx, 3U, ADS1248_SPI_DELAY);
	delay_us(3);
	hads->CS_Port->BSRR = hads->CS_Pin;
	if(status != HAL_OK)
	{
		return status;
	}

	*pData = rx[2];

	return status;
}
//note: doesnt allow warpback of addresses
HAL_StatusTypeDef ADS1248_ReadReg_n(ADS1248_Handle_t *hads,ADS1248_Reg_t Ads1248_Reg,uint8_t n, uint8_t *pData)
{
	if(hads == NULL || pData == NULL)
	{
		return HAL_ERROR;
	}
	if(n == 0 || n > 16 || Ads1248_Reg > ADS1248_REG_GPIODAT || n > (15 - Ads1248_Reg))
	{
		return HAL_ERROR;
	}
	uint8_t tx[n+2];
	uint8_t rx[n+2];
	HAL_StatusTypeDef status;

	tx[0] = __ADS1248_CMD_RREG(Ads1248_Reg);
	tx[1] = __ADS1248_CMD_W_R_REG_COUNT(n);

	hads->CS_Port->BSRR = hads->CS_Pin << 16U;
	delay_us(1);

	for(uint8_t i = 0U; i<(n+2) ; i++)
	{
		tx[i+2] = ADS1248_CMD_NOP;
	}
	status = HAL_SPI_TransmitReceive(hads->hspi, tx, rx, (n+2), ADS1248_SPI_DELAY);
	delay_us(3);
	hads->CS_Port->BSRR = hads->CS_Pin;
	if(status != HAL_OK)
	{
		return status;
	}
	for(uint8_t i = 0U; i<n ; i++)
	{
		*(pData+i) = rx[i+2];
	}
	return status;
}
//Note: this function doesnt send SELFOCAL, SYSOCAL, SYSGCAL commands
HAL_StatusTypeDef ADS1248_Command_Tx(ADS1248_Handle_t *hads, uint8_t Ads1248_Cmd)
{
	if(Ads1248_Cmd > __ADS1248_CMD_SELFOCAL)
	{
		return HAL_ERROR;
	}
	uint8_t tx;
	HAL_StatusTypeDef status;

	tx = Ads1248_Cmd;

	hads->CS_Port->BSRR = hads->CS_Pin << 16U;
	delay_us(1);
	status = HAL_SPI_Transmit(hads->hspi, &tx, 1, ADS1248_SPI_DELAY);
	delay_us(3);
	hads->CS_Port->BSRR = hads->CS_Pin;
	if(status != HAL_OK)
	{
		return status;
	}

	return status;
}
//note: setting Burnout current source will add noise to the ADC data
HAL_StatusTypeDef ADS1248_Set_Burnout_Current(ADS1248_Handle_t *hads,ADS1248_Burnout_Current Ads1248_Bcs)
{
	uint8_t reg;
	HAL_StatusTypeDef status;
	status = ADS1248_ReadReg_Once(hads, ADS1248_REG_MUX0, &reg);
	if(status != HAL_OK)
	{
		return status;
	}
	reg &= __ADS1248_CLEAR_BSC;
	reg |= Ads1248_Bcs <<__ADS1248_BSC_POS;

	return ADS1248_WriteReg_Once(hads, ADS1248_REG_MUX0, reg);
}
//note: dont assign same AIN pins as the input
HAL_StatusTypeDef ADS1248_Set_Input_Channel(ADS1248_Handle_t *hads,Ads1248_Mux_PosInput_t Ads1248_Mux_Sp,Ads1248_Mux_NegInput_t Ads1248_Mux_Sn)
{
	if(Ads1248_Mux_Sp > ADS1248_MUX_SP_AIN7 || Ads1248_Mux_Sn > ADS1248_MUX_SN_AIN7)
	{
		return HAL_ERROR;
	}
	uint8_t reg;
	HAL_StatusTypeDef status;
	status = ADS1248_ReadReg_Once(hads, ADS1248_REG_MUX0,&reg);
	if(status != HAL_OK)
	{
		return status;
	}
	reg &= __ADS1248_CLEAR_MUX_SP | __ADS1248_CLEAR_MUX_NP;
	reg |= (Ads1248_Mux_Sp << __ADS1248_MUX_SP_POS) | (Ads1248_Mux_Sn << __ADS1248_MUX_SN_POS);
	return ADS1248_WriteReg_Once(hads, ADS1248_REG_MUX0,reg);
}
HAL_StatusTypeDef ADS1248_Set_VBIAS(ADS1248_Handle_t *hads,uint8_t Ads1248_Vbias,uint8_t En_Di)
{
	if(En_Di > ENABLE)
	{
		return HAL_ERROR;
	}
	uint8_t reg;
	HAL_StatusTypeDef status;
	status = ADS1248_ReadReg_Once(hads, ADS1248_REG_VBIAS,&reg);
	if(status!=HAL_OK)
	{
		return status;
	}
	if(En_Di == ENABLE)
	{
		reg |= Ads1248_Vbias;
	}
	else if(En_Di == DISABLE)
	{
		reg &= ~(Ads1248_Vbias);
	}
	return ADS1248_WriteReg_Once(hads, ADS1248_REG_VBIAS, reg);
}
HAL_StatusTypeDef ADS1248_Set_Vref_State(ADS1248_Handle_t *hads,ADS1248_Vref_State Ads1248_Vrefcon)
{
	uint8_t reg;
	HAL_StatusTypeDef status;
	status = ADS1248_ReadReg_Once(hads, ADS1248_REG_MUX1, &reg);
	if(status != HAL_OK)
	{
		return status;
	}

	reg &= __ADS1248_CLEAR_VREFCON;
	reg |= Ads1248_Vrefcon <<__ADS1248_VREFCON_POS;

	return ADS1248_WriteReg_Once(hads, ADS1248_REG_MUX1, reg);
}
HAL_StatusTypeDef ADS1248_Set_Vref_Source(ADS1248_Handle_t *hads,Ads1248_Reference_Src_t Ads1248_Refsel)
{
	uint8_t reg;
	HAL_StatusTypeDef status;
	status = ADS1248_ReadReg_Once(hads, ADS1248_REG_MUX1, &reg);
	if(status != HAL_OK)
	{
		return status;
	}
	reg &= __ADS1248_CLEAR_REFSELT;
	reg |= Ads1248_Refsel << __ADS1248_REFSELT_POS;
	return ADS1248_WriteReg_Once(hads, ADS1248_REG_MUX1, reg);
}
HAL_StatusTypeDef ADS1248_Set_System_Monitor(ADS1248_Handle_t *hads,ADS1248_MuxCal_t Ads1248_MuxCal)
{
	uint8_t reg;
	HAL_StatusTypeDef status;
	status = ADS1248_ReadReg_Once(hads, ADS1248_REG_MUX1, &reg);
	if(status != HAL_OK)
	{
		return status;
	}
	reg &= __ADS1248_CLEAR_MUXCAL;
	reg |= Ads1248_MuxCal << __ADS1248_MUXCAL_POS;
	return ADS1248_WriteReg_Once(hads, ADS1248_REG_MUX1, reg);
}
/**
  * @brief  Sets the Programmable Gain Amplifier (PGA) gain for the ADS1248.
  * @note   This function modifies the SYS0 register. According to the datasheet,
  *         changing the PGA gain also automatically loads the corresponding
  *         factory-trimmed full-scale calibration (FSC) reset value.
  * @param  hads           Pointer to an ADS1248_Handle_t structure that contains
  *                        the configuration information for the specified ADS1248.
  * @param  Ads1248_Pga    PGA gain setting to apply. Valid settings from the
  *                        PGA Full-Scale Range table (when using Internal Vref of 2.048V):
  *                          - 1   (FSR: ±2.048 V)
  *                          - 2   (FSR: ±1.024 V)
  *                          - 4   (FSR: ±0.512 V)
  *                          - 8   (FSR: ±0.256 V)
  *                          - 16  (FSR: ±0.128 V)
  *                          - 32  (FSR: ±0.064 V)
  *                          - 64  (FSR: ±0.032 V)
  *                          - 128 (FSR: ±0.016 V)
  * @retval HAL_StatusTypeDef HAL status code (HAL_OK on success, or error code on failure).
  */
// note: SYS0 setting such as PGA and DR setting must be change before MUX0 setting sehrn togging betweem the Input Channels to avoid Overloadding
HAL_StatusTypeDef ADS1248_Set_PGA(ADS1248_Handle_t *hads,ADS1248_GainPga_t Ads1248_Pga)
{
	uint8_t reg;
	HAL_StatusTypeDef status;
	status = ADS1248_ReadReg_Once(hads, ADS1248_REG_SYS0, &reg);
	if(status != HAL_OK)
	{
		return status;
	}
	reg &= __ADS1248_CLEAR_PGA;
	reg |= Ads1248_Pga << __ADS1248_PGA_POS;
	return ADS1248_WriteReg_Once(hads, ADS1248_REG_SYS0, reg);
}
HAL_StatusTypeDef ADS1248_Set_Data_Rate(ADS1248_Handle_t *hads,ADS1248_DataRate_t Ads1248_Datarate)
{
	uint8_t reg;
	HAL_StatusTypeDef status;
	status = ADS1248_ReadReg_Once(hads, ADS1248_REG_SYS0, &reg);
	if(status != HAL_OK)
	{
		return status;
	}
	reg &= __ADS1248_CLEAR_DR;
	reg |= Ads1248_Datarate << __ADS1248_DR_POS;
	return ADS1248_WriteReg_Once(hads, ADS1248_REG_SYS0, reg);
}
HAL_StatusTypeDef ADS1248_Set_Drdy_Mode(ADS1248_Handle_t *hads, Ads1248_Drdy Ads1248_Drdy_Mode)
{
	uint8_t reg;
	HAL_StatusTypeDef status;
	status = ADS1248_ReadReg_Once(hads, ADS1248_REG_IDAC0, &reg);
	if(status != HAL_OK)
	{
		return status;
	}
	reg &= __ADS1248_CLEAR_DRDY;
	reg |= Ads1248_Drdy_Mode <<__ADS1248_DRDY_MODE_POS;

	return ADS1248_WriteReg_Once(hads, ADS1248_REG_IDAC0, reg);
}
// when setting magnitude, also need to set the current output for IADC
//note: Int ref will be tunred on and will be set to always on mode when setting the current magnitude
// if u are going to turnoff the exci current, it keeps the internal reference to always on mode, also need to set the internal refrence state after this turning off the exci current
HAL_StatusTypeDef ADS1248_Set_Exci_Current_Mag(ADS1248_Handle_t *hads,ADS1248_IdacMagnitude_t Ads1248_IMag)
{
	if(Ads1248_IMag > ADS1248_IMAG_1500_UA || hads == NULL)
	{
		return HAL_ERROR;
	}
	uint8_t reg[2];
	HAL_StatusTypeDef status;
	status = ADS1248_ReadReg_Once(hads, ADS1248_REG_MUX1, &reg[0]);
	if(status)
	{
		return status;
	}
	reg[0] &= __ADS1248_CLEAR_VREFCON;
	status = ADS1248_ReadReg_Once(hads, ADS1248_REG_IDAC0, &reg[1]);
	if(status)
	{
		return status;
	}
	reg[1] &= __ADS1248_CLEAR_IMAG;
	if(Ads1248_IMag != ADS1248_IMAG_OFF)
	{
		reg[0] |= ADS1248_VREFCON_INT_ALWAYS_ON << __ADS1248_VREFCON_POS;
		reg[1] |= Ads1248_IMag << __ADS1248_IMAG_POS;
	}
	status = ADS1248_WriteReg_Once(hads, ADS1248_REG_MUX1, reg[0]);
	if(status)
	{
		return status;
	}
	return ADS1248_WriteReg_Once(hads, ADS1248_REG_IDAC0, reg[1]);
}
// when selecting excitation current output pin, which IDAC u are going to use must also be set
HAL_StatusTypeDef ADS1248_Set_Exci_Current_Pin(ADS1248_Handle_t *hads,uint8_t Ads1248_idac,ADS1248_Exci_Pin_t Ads1248_Ex_Cur)
{
	if(hads == NULL || Ads1248_idac > ADS1248_IDAC_2 || Ads1248_Ex_Cur > ADS1248_EX_CUR_OUT_DISCON)
	{
		return HAL_ERROR;
	}
	uint8_t reg;
	HAL_StatusTypeDef status;
	status = ADS1248_ReadReg_Once(hads, ADS1248_REG_IDAC1,&reg);
	if(status)
	{
		return status;
	}
	if(Ads1248_idac == ADS1248_IDAC_1)
	{
		reg &= __ADS1248_CLEAR_I1DIR;
		reg |= (Ads1248_Ex_Cur << __ADS1248_I1DIR_POS);
	}
	else if(Ads1248_idac == ADS1248_IDAC_2)
	{
		reg &= __ADS1248_CLEAR_I2DIR;
		reg |= (Ads1248_Ex_Cur << __ADS1248_I2DIR_POS);
	}
	return ADS1248_WriteReg_Once(hads, ADS1248_REG_IDAC1, reg);
}
HAL_StatusTypeDef ADS1248_Set_GPIO_Mode(ADS1248_Handle_t *hads,uint8_t Ads1248_Pin,uint8_t Ads1248_Ain_Mode)
{
	HAL_StatusTypeDef status;
	uint8_t reg;
	if(Ads1248_Pin > ADS1248_PIN_AIN_7 || Ads1248_Ain_Mode > ADS1248_AIN_MODE_DISABLE || hads == NULL || !__IS_ADS1248_GPIO_PIN(Ads1248_Pin))
	{
		return HAL_ERROR;
	}
	status = ADS1248_ReadReg_Once(hads, ADS1248_REG_GPIOCFG, &reg);
	if(status != HAL_OK)
	{
		return status;
	}
	if(Ads1248_Ain_Mode == ADS1248_AIN_MODE_OUTPUT || Ads1248_Ain_Mode == ADS1248_AIN_MODE_INPUT)
	{
		reg |= Ads1248_Pin;
	}
	else
	{
		reg &= ~(Ads1248_Pin);
	}
	status = ADS1248_WriteReg_Once(hads, ADS1248_REG_GPIOCFG, reg);
	if(status != HAL_OK)
	{
		return status;
	}
	//also clears the GPIODAT reg for that pin
	if(Ads1248_Ain_Mode == ADS1248_AIN_MODE_DISABLE)
	{
		status = ADS1248_ReadReg_Once(hads, ADS1248_REG_GPIODAT, &reg);
		if(status != HAL_OK)
		{
			return status;
		}
		reg &= ~(Ads1248_Pin);
		status = ADS1248_WriteReg_Once(hads, ADS1248_REG_GPIODAT, reg);
		if(status != HAL_OK)
		{
			return status;
		}
	}
	status = ADS1248_ReadReg_Once(hads, ADS1248_REG_GPIODIR, &reg);
	if(status != HAL_OK)
	{
		return status;
	}
	if(Ads1248_Ain_Mode == ADS1248_AIN_MODE_OUTPUT)
	{
		reg &= ~(Ads1248_Pin);
	}
	else if(Ads1248_Ain_Mode == ADS1248_AIN_MODE_INPUT)
	{
		reg |= Ads1248_Pin;
	}
	status = ADS1248_WriteReg_Once(hads, ADS1248_REG_GPIODIR, reg);
	if(status != HAL_OK)
	{
		return status;
	}
	return status;
}
HAL_StatusTypeDef ADS1248_GPIO_Write(ADS1248_Handle_t *hads,uint8_t Ads1248_Pin,ADS1248_Gpio_Pin_State_t Ads1248_Gpio)
{
	if(Ads1248_Gpio > ADS1248_GPIO_SET || !__IS_ADS1248_GPIO_PIN(Ads1248_Pin))
	{
		return HAL_ERROR;
	}
	uint8_t reg;
	HAL_StatusTypeDef status;
	status = ADS1248_ReadReg_Once(hads, ADS1248_REG_GPIODAT, &reg);
	if(status != HAL_OK)
	{
		return status;
	}
	reg &= ~Ads1248_Pin;
	if(Ads1248_Gpio == ADS1248_GPIO_SET)
	{
		reg |= Ads1248_Pin;
	}
	return ADS1248_WriteReg_Once(hads, ADS1248_REG_GPIODAT, reg);
}
uint8_t ADS1248_GPIO_Read(ADS1248_Handle_t *hads,uint8_t Ads1248_Pin)
{
	if(hads == NULL || !__IS_ADS1248_GPIO_PIN(Ads1248_Pin))
	{
		return 0xFF;
	}
	uint8_t reg;
	HAL_StatusTypeDef status;
	ADS1248_Gpio_Pin_State_t pin_status;
	status = ADS1248_ReadReg_Once(hads, ADS1248_REG_GPIODAT, &reg);
	if(status != HAL_OK)
	{
		return 0x99;
	}
	if((reg & Ads1248_Pin) == ADS1248_GPIO_SET)
	{
		pin_status = ADS1248_GPIO_SET;
	}
	else
	{
		pin_status = ADS1248_GPIO_RESET;
	}
	return pin_status;
}
// used to calc dealy for completion of calibration
static uint32_t __ADS1248_Calc_Calib_Delay(ADS1248_Handle_t *hads)
{
	uint8_t reg;
	HAL_StatusTypeDef status;
	status = ADS1248_ReadReg_Once(hads, ADS1248_REG_SYS0, &reg);
	if(status != HAL_OK)
	{
		return 0X9999;
	}
	switch((reg & 0x7))
	{
		case ADS1248_DATARATE_5SPS:
			return 0xC82;
		case ADS1248_DATARATE_10SPS:
			return 0x642;
		case ADS1248_DATARATE_20SPS:
			return 0x322;
		case ADS1248_DATARATE_40SPS:
			return 0x191;
		case ADS1248_DATARATE_80SPS:
			return 0xC9;
		case ADS1248_DATARATE_160SPS:
			return 0x65;
		case ADS1248_DATARATE_320SPS:
			return 0x33;
		case ADS1248_DATARATE_640SPS:
			return 0x1A;
		case ADS1248_DATARATE_1000SPS:
			return 0x11;
		case ADS1248_DATARATE_2000SPS:
			return 0x9;
		default:
			return 0x00;
	}
}
#if USE_CONFIG_ADS1248_ADD_OFFSET_VALUES_MANUALLY
// use this function if u manually need to add offset to the 24Bit ADC Values
HAL_StatusTypeDef ADS1248_Get_Voltage_V_Man_Offset(ADS1248_Handle_t *hads,int32_t *pData32,double *pDataD)
{
	uint8_t tx[3] = {ADS1248_CMD_NOP,ADS1248_CMD_NOP,ADS1248_CMD_NOP};
	uint8_t rx[3];
	int32_t raw_data;
	HAL_StatusTypeDef status;
	hads->CS_Port->BSRR = hads->CS_Pin<<16;
	delay_us(1);
	status = HAL_SPI_TransmitReceive(hads->hspi, tx, rx, 3, ADS1248_SPI_DELAY);
	delay_us(3);
	hads->CS_Port->BSRR = hads->CS_Pin;
	if(status != HAL_OK)
	{
		return status;
	}
	raw_data = (int32_t)(((uint32_t)(rx[0]<<16)) | ((uint32_t)(rx[1]<<8)) | ((uint32_t)(rx[2]<<0)));
	if(raw_data & 0x800000)
	{
		raw_data |= 0xFF000000;
	}
	raw_data -= (*pData32);

	*pDataD = (double)(raw_data * hads->Init.__voltage_lsb);

	return status;
}
#else
HAL_StatusTypeDef ADS1248_Get_Voltage_V(ADS1248_Handle_t *hads,double *pDataD)
{
	uint8_t tx[3] = {ADS1248_CMD_NOP,ADS1248_CMD_NOP,ADS1248_CMD_NOP};
	uint8_t rx[3];
	int32_t raw_data;
	HAL_StatusTypeDef status;
	hads->CS_Port->BSRR = hads->CS_Pin<<16;
	delay_us(1);
	status = HAL_SPI_TransmitReceive(hads->hspi, tx, rx, 3, ADS1248_SPI_DELAY);
	delay_us(3);
	hads->CS_Port->BSRR = hads->CS_Pin;
	if(status != HAL_OK)
	{
		return status;
	}
	raw_data = (int32_t)(((uint32_t)(rx[0]<<16)) | ((uint32_t)(rx[1]<<8)) | ((uint32_t)(rx[2]<<0)));
	if(raw_data & 0x800000)
	{
		raw_data |= 0xFF000000;
	}

	*pDataD = (double)(raw_data * hads->Init.__voltage_lsb);

	return status;
}
#endif
HAL_StatusTypeDef ADS1248_Set_Self_Calib(ADS1248_Handle_t *hads)
{
	HAL_StatusTypeDef status;
	uint32_t calib_delay;
	status = ADS1248_Command_Tx(hads, __ADS1248_CMD_SELFOCAL);
	if(status != HAL_OK)
	{
		return status;
	}
	calib_delay = __ADS1248_Calc_Calib_Delay(hads);
	HAL_Delay(calib_delay);

	return status;
}
#if USE_CONFIG_ADS1248_SET_CALIB_VALUES_AUTOMATICALLY
HAL_StatusTypeDef ADS1248_Set_System_Offset_Calib(ADS1248_Handle_t *hads)
{
	uint8_t reg;
	HAL_StatusTypeDef status;
	uint32_t calib_delay;
	calib_delay = __ADS1248_Calc_Calib_Delay(hads);
	status = ADS1248_ReadReg_Once(hads, ADS1248_REG_MUX0, &reg);
	if(status != HAL_OK)
	{
		return status;
	}
	status = ADS1248_Set_VBIAS(hads, (1U<<(reg & (0x07U))), ENABLE);
	if(status != HAL_OK)
	{
		return status;
	}
	status = ADS1248_Command_Tx(hads, __ADS1248_CMD_SYSOCAL);
	if(status)
	{
		return status;
	}
	HAL_Delay(calib_delay);
	reg &= __ADS1248_CLEAR_MUXCAL;
	reg |= ADS1248_MUXCAL_NORMAL<< __ADS1248_MUXCAL_POS;

	status =  ADS1248_WriteReg_Once(hads, ADS1248_REG_MUX1, reg);

	return ADS1248_Set_VBIAS(hads, (1U<<(reg & (0x07U))), ENABLE);
}
// warning ADS1248_Set_System_Offset_Calib() must be called and OFC must be set before calling this function
HAL_StatusTypeDef ADS1248_Set_Gain_Calib(ADS1248_Handle_t *hads)
{
	uint8_t reg;
	HAL_StatusTypeDef status;
	uint32_t calib_delay;


	status = ADS1248_ReadReg_Once(hads, ADS1248_REG_MUX1, &reg);
	if(status != HAL_OK)
	{
		return status;
	}

	reg &= __ADS1248_CLEAR_MUXCAL;
	reg |= ADS1248_MUXCAL_GAIN_CALIB << __ADS1248_MUXCAL_POS;
	status = ADS1248_WriteReg_Once(hads, ADS1248_REG_MUX1, reg);
	if(status != HAL_OK)
	{
		return status;
	}

	calib_delay = __ADS1248_Calc_Calib_Delay(hads);

	status = ADS1248_Command_Tx(hads, __ADS1248_CMD_SYSGCAL);
	if(status)
	{
		return status;
	}
	HAL_Delay(calib_delay);
	reg &= __ADS1248_CLEAR_MUXCAL;
	reg |= ADS1248_MUXCAL_NORMAL<< __ADS1248_MUXCAL_POS;

	return ADS1248_WriteReg_Once(hads, ADS1248_REG_MUX1, reg);
}
#else
// used to set the value of OFC manually
HAL_StatusTypeDef ADS1248_Set_Offset_Calib(ADS1248_Handle_t *hads, uint32_t *pData32)
{
	if(*pData32 > 0xFFFFFF)
	{
		return HAL_ERROR;
	}
	return ADS1248_WriteReg_3(hads, ADS1248_REG_OFC0,pData32);
}
// used to set the value of FSC manually
HAL_StatusTypeDef ADS1248_Set_FullScale_Calib(ADS1248_Handle_t *hads, uint32_t *pData32)
{
	if(*pData32 > 0xFFFFFF)
	{
		return HAL_ERROR;
	}
	return ADS1248_WriteReg_3(hads, ADS1248_REG_FSC0,pData32);
}
static HAL_StatusTypeDef ADS1248_WriteReg_3(ADS1248_Handle_t *hads,ADS1248_Reg_t Ads1248_Reg,uint32_t *pData32)
{
	uint8_t tx[5];
	HAL_StatusTypeDef status;

	tx[0] = __ADS1248_CMD_WREG(Ads1248_Reg);
	tx[1] = __ADS1248_CMD_W_R_REG_COUNT(3U);
	tx[2] = (uint8_t)(*pData32>>16);
	tx[3] = (uint8_t)(*pData32>>8);
	tx[4] = (uint8_t)(*pData32);

	hads->CS_Port->BSRR = hads->CS_Pin << 16U;
	delay_us(1);
	status = HAL_SPI_Transmit(hads->hspi, tx, 5U, ADS1248_SPI_DELAY);
	delay_us(3);
	hads->CS_Port->BSRR = hads->CS_Pin;
	if(status != HAL_OK)
	{
		return status;
	}

	return status;
}
#endif
uint8_t ADS1248_Get_Clock_Status(ADS1248_Handle_t *hads)
{
	uint8_t reg;
	HAL_StatusTypeDef status;
	status = ADS1248_ReadReg_Once(hads, ADS1248_REG_MUX1,&reg);
	if(status != HAL_OK)
	{
		return 0x99;
	}
	return (reg & (__ADS1248_CLCKSTAT_MASK));
}
HAL_StatusTypeDef ADS1248_Get_Temp_C(ADS1248_Handle_t *hads,double *pDataD)
{
	uint8_t tx[3] = {ADS1248_CMD_NOP,ADS1248_CMD_NOP,ADS1248_CMD_NOP};
	uint8_t rx[3];
	int32_t raw_data;
	HAL_StatusTypeDef status;
	hads->CS_Port->BSRR = hads->CS_Pin<<16;
	delay_us(1);
	status = HAL_SPI_TransmitReceive(hads->hspi, tx, rx, 3, ADS1248_SPI_DELAY);
	delay_us(3);
	hads->CS_Port->BSRR = hads->CS_Pin;
	if(status != HAL_OK)
	{
		return status;
	}
	raw_data = (int32_t)(((uint32_t)(rx[0]<<16)) | ((uint32_t)(rx[1]<<8)) | ((uint32_t)(rx[2]<<0)));
	if(raw_data & 0x800000)
	{
		raw_data |= 0xFF000000;
	}

	*pDataD = (double)__ADS1248_TEMP_LSB(raw_data);

	return status;
}
