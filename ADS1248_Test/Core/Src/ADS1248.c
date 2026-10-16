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
static HAL_StatusTypeDef ADS1248_WriteReg_Once(ADS1248_Handle_t *hads,ADS1248_Reg_t Ads1248_Reg,uint8_t Data);
static uint32_t __ADS1248_Calc_Calib_Delay(ADS1248_Handle_t *hads);

/**
 * @brief  Initializes the ADS1248 handle parameters for voltage calculation.
 *
 * @details This function sets the default reference values required for converting raw ADC
 *          readings into actual voltage measurements, specifically for use in the
 *          ADS1248_Get_Data function. It configures the internal PGA value to 1,
 *          sets the internal reference voltage to 2.048V, and pre-calculates the
 *          voltage LSB weight based on the maximum positive 24-bit range (0x7FFFFF).
 *
 * @param  hads Pointer to the ADS1248_Handle_t structure that contains the
 *              initialization and configuration parameters for the device.
 *
 * @retval None
 */
void ADS1248_Init(ADS1248_Handle_t *hads)
{
	hads->Init.__pga_value = 1U;
	hads->Init.__vref_voltage = 2.048f;
	hads->Init.__voltage_lsb = (float) ((hads->Init.__vref_voltage/(hads->Init.__pga_value) / 8388608.0f));
}
/**
 * @brief  Calculates and sets the voltage LSB weight for converting raw 24-bit data into an actual voltage value.
 *
 * @details This function validates the provided reference voltage against the allowed
 *          hardware limits (0.5V to 4.25V). It performs an SPI read of the SYS0 register
 *          to dynamically determine the currently active Programmable Gain Amplifier (PGA)
 *          setting. Using the user-provided reference voltage and the retrieved PGA gain,
 *          it computes the voltage LSB weight using the maximum positive 24-bit code
 *          range (0x7FFFFF). The reference voltage, active PGA, and calculated LSB
 *          are then cached in the handle for future data conversions.
 *
 * @param  hads               Pointer to the ADS1248_Handle_t structure that contains the
 *                            initialization parameters, SPI handle, and state for the device.
 * @param  ads1248_vref_vltg  The physical reference voltage applied to the ADC.
 *                            Must be a floating-point value between 0.5f and 4.25f.
 *
 * @retval HAL_StatusTypeDef HAL status:
 *         - HAL_OK:      The PGA register was successfully read and the LSB was updated.
 *         - HAL_ERROR:   A NULL handle was passed, the reference voltage was out of bounds,
 *                        an invalid PGA value was decoded, or an SPI communication error occurred.
 */
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

	hads->Init.__voltage_lsb = (float) ((hads->Init.__vref_voltage/(hads->Init.__pga_value) / 8388608.0f));

	return status;
}
/**
 * @brief  Performs a hardware reset on the ADS1248 using the physical RESET pin.
 *
 * @details This function toggles the hardware RESET pin to perform a hard reset
 *          of the ADS1248. It validates the RESET port pointer, pulls the
 *          RESET pin LOW (active low) for 2 milliseconds to satisfy the minimum
 *          reset pulse duration, and then pulls it HIGH. It incorporates an
 *          additional 2-millisecond delay after releasing the reset to ensure
 *          the internal digital filter and registers have fully initialized
 *          (which requires at least 0.6 ms) before any subsequent SPI communication.
 *
 * @param  hads Pointer to the ADS1248_Handle_t structure that contains the
 *              hardware port and pin mappings for the device.
 *
 * @retval HAL_StatusTypeDef HAL status:
 *         - HAL_OK:    Hardware reset sequence was executed successfully.
 *         - HAL_ERROR: The RESET_Port pointer in the handle was NULL.
 */
HAL_StatusTypeDef ADS1248_Reset_Hard(ADS1248_Handle_t *hads)
{
	if(hads->RESET_Port == NULL)
	{
		return HAL_ERROR;
	}
	hads->RESET_Port->BSRR = hads->RESET_Pin << 16U;
	HAL_Delay(2U);
	hads->RESET_Port->BSRR = hads->RESET_Pin;
	HAL_Delay(2U);
	return HAL_OK;
}
/**
 * @brief  Triggers an ADC conversion using the hardware START pin.
 *
 * @details This function generates a brief HIGH pulse on the physical START pin
 *          to initiate a conversion. It asserts the pin HIGH, waits for 1 microsecond
 *          (satisfying the minimum 3 clock cycles / ~0.7 µs requirement for a
 *          4.096 MHz fCLK), and then pulls it back LOW.
 *
 * @note    Do not mix conversion control methods. If you use the physical START pin
 *          to trigger and control conversions, do not simultaneously use software
 *          SPI commands (like WAKEUP or SYNC) to control conversions, as this can
 *          lead to unexpected state behavior.
 *
 * @param  hads Pointer to the ADS1248_Handle_t structure that contains the
 *              hardware port and pin mappings for the device.
 *
 * @retval HAL_StatusTypeDef HAL status:
 *         - HAL_OK:    The START pulse was successfully generated.
 *         - HAL_ERROR: The START_Port pointer within the handle was NULL.
 */
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
/**
 * @brief  Writes a single 8-bit data value to a specific ADS1248 register.
 *
 * @details This function constructs and executes a 3-byte SPI transaction to write
 *          data to a configuration register. It sequentially transmits the Write
 *          Register (WREG) command combined with the target address, the number of
 *          registers to write minus one (which is 0x00 for a single register), and
 *          the 8-bit data payload. It safely manages the Chip Select (CS) line,
 *          incorporating the necessary microsecond delays before and after the
 *          SPI transmission.
 *
 * @note    The ADS1248 MUST be in Stop Read Data Continuous (SDATAC) mode before
 *          calling this function. If the device is in the default RDATAC mode,
 *          register write commands will be ignored by the ADC.
 *
 * @param  hads        Pointer to the ADS1248_Handle_t structure that contains
 *                     the SPI handle and hardware pin configurations for the device.
 * @param  Ads1248_Reg The specific target register address to write to (of type ADS1248_Reg_t).
 * @param  Data        The 8-bit data value to be written into the specified register.
 *
 * @retval HAL_StatusTypeDef HAL status of the SPI transaction:
 *         - HAL_OK:      The SPI transaction completed and data was written successfully.
 *         - HAL_ERROR:   An error occurred during the SPI transmission.
 *         - HAL_BUSY:    The SPI peripheral is currently locked by another process.
 *         - HAL_TIMEOUT: The SPI transaction exceeded the specified maximum delay limit (ADS1248_SPI_DELAY).
 */
static HAL_StatusTypeDef ADS1248_WriteReg_Once(ADS1248_Handle_t *hads,ADS1248_Reg_t Ads1248_Reg,uint8_t Data)
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

	return status;
}
/**
 * @brief  Reads a single 8-bit register value from the ADS1248 device.
 *
 * @details This function executes a continuous, full-duplex 3-byte SPI transaction
 *          to safely read a specific configuration register. It sequentially transmits
 *          the Read Register (RREG) command, the number of registers to read minus one
 *          (which is 0 for a single register), and a NOP (No Operation) byte. The ADC
 *          shifts out the requested register data during the final NOP byte, which is
 *          then extracted from the third position (rx[2]) of the receive buffer.
 *
 * @param  hads        Pointer to the ADS1248_Handle_t structure that contains
 *                     the SPI handle and hardware pin configurations for the device.
 * @param  Ads1248_Reg The specific target register address to read (of type ADS1248_Reg_t).
 * @param  pData       Pointer to a uint8_t variable where the retrieved register
 *                     data will be stored.
 *
 * @retval HAL_StatusTypeDef HAL status of the SPI transaction:
 *         - HAL_OK:      The SPI transaction completed successfully and the register data was retrieved.
 *         - HAL_ERROR:   An error occurred during the SPI transmission/reception.
 *         - HAL_BUSY:    The SPI peripheral is currently locked by another process.
 *         - HAL_TIMEOUT: The SPI transaction exceeded the specified maximum delay limit (ADS1248_SPI_DELAY).
 */
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
/**
 * @brief  Reads multiple consecutive configuration registers from the ADS1248.
 *
 * @details This function constructs and executes a contiguous, full-duplex SPI
 *          transaction to read `n` sequential registers starting from a specific
 *          base address. It includes robust bounds checking to ensure that the
 *          read operation does not exceed the highest valid register address
 *          (0x0E), intentionally preventing the ADC's hardware address wrap-around
 *          behavior for safety. Variable Length Arrays (VLAs) are used to dynamically
 *          allocate the SPI buffers based on the requested length.
 *
 * @param  hads        Pointer to the ADS1248_Handle_t structure that contains
 *                     the SPI handle and hardware pin configurations.
 * @param  Ads1248_Reg The starting register address to read from (of type ADS1248_Reg_t).
 * @param  n           The number of consecutive registers to read.
 * @param  pData       Pointer to a uint8_t array where the retrieved register
 *                     data will be stored sequentially.
 *
 * @retval HAL_StatusTypeDef HAL status of the SPI transaction:
 *         - HAL_OK:      The registers were successfully read and stored in pData.
 *         - HAL_ERROR:   A NULL pointer was passed, `n` was 0 or out of bounds,
 *                        an invalid starting register was provided, or the request
 *                        would cause an address wrap-around.
 *         - HAL_BUSY:    The SPI peripheral is currently locked.
 *         - HAL_TIMEOUT: The SPI transaction exceeded the specified delay limit.
 */
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
/**
 * @brief  Transmits a standalone, single-byte command to the ADS1248.
 *
 * @details This function is used to send basic operational SPI commands
 *          (such as WAKEUP, SLEEP, SYNC, RESET, and SDATAC) to the ADC. It
 *          manages the Chip Select (CS) pin and ensures the necessary
 *          microsecond timing delays are applied before and after the SPI
 *          transmission. An initial bounds check ensures that only permitted
 *          commands are transmitted.
 *
 * @note    This function is NOT intended for sending calibration commands
 *          (SELFOCAL, SYSOCAL, SYSGCAL) or data/register operations (RREG,
 *          WREG, RDATA). Those commands require specific wait times, internal
 *          multiplexer changes, or multi-byte payloads, and must be handled
 *          by their own dedicated functions.
 *
 * @param  hads        Pointer to the ADS1248_Handle_t structure that contains
 *                     the SPI handle and hardware pin configurations.
 * @param  Ads1248_Cmd The 8-bit command byte to transmit.
 *
 * @retval HAL_StatusTypeDef HAL status of the SPI transaction:
 *         - HAL_OK:      The command was successfully transmitted.
 *         - HAL_ERROR:   The requested command exceeded the allowed upper limit
 *                        (__ADS1248_CMD_SELFOCAL) or an SPI error occurred.
 *         - HAL_BUSY:    The SPI peripheral is currently locked.
 *         - HAL_TIMEOUT: The SPI transaction exceeded the specified delay limit.
 */
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
/**
 * @brief  Configures the sensor burn-out detection current sources on the ADS1248.
 *
 * @details This function performs a read-modify-write operation on the MUX0
 *          register to update the Burnout Current Source (BCS) bits[cite: 161].
 *          These current sources (0.5 µA, 2 µA, or 10 µA) are injected into the
 *          selected analog inputs to help detect open-circuit or short-circuit
 *          sensor fault conditions[cite: 139].
 *
 * @note    Setting the burn-out current source will add noise to the ADC data.
 *          The ADC readings of a functional sensor may be corrupted when the
 *          burn-out current sources are enabled[cite: 139]. It is recommended
 *          to disable the burn-out current sources when performing precision
 *          measurements, and only enable them temporarily to test for sensor
 *          fault conditions[cite: 139].
 *
 * @param  hads        Pointer to the ADS1248_Handle_t structure that contains
 *                     the SPI handle and hardware pin configurations.
 * @param  Ads1248_Bcs The desired burn-out current configuration (e.g., Off, 0.5µA,
 *                     2µA, or 10µA).
 *
 * @retval HAL_StatusTypeDef HAL status of the SPI transactions:
 *         - HAL_OK:    The MUX0 register was successfully read and updated.
 *         - HAL_ERROR: An error occurred during the SPI read or write process.
 */
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
/**
 * @brief  Configures the positive and negative analog input channels for the ADS1248 multiplexer.
 *
 * @details This function performs a read-modify-write operation on the MUX0
 *          register to route specific external analog pins (AIN0 to AIN7) to the
 *          internal Programmable Gain Amplifier (PGA). It includes bounds checking
 *          to ensure the requested channels do not exceed the highest available
 *          pin (AIN7).
 *
 * @note    Do not assign the exact same AIN pin as both the positive and negative
 *          input. Doing so will result in the ADC measuring a dead short (0V)
 *          rather than a valid external differential signal.
 *
 * @param  hads           Pointer to the ADS1248_Handle_t structure that contains
 *                        the SPI handle and hardware pin configurations.
 * @param  Ads1248_Mux_Sp The desired positive analog input channel selection
 *                        (of type Ads1248_Mux_PosInput_t).
 * @param  Ads1248_Mux_Sn The desired negative analog input channel selection
 *                        (of type Ads1248_Mux_NegInput_t).
 *
 * @retval HAL_StatusTypeDef HAL status of the SPI transactions:
 *         - HAL_OK:    The MUX0 register was successfully updated with the new channel routing.
 *         - HAL_ERROR: One or both of the selected channels exceeded the maximum allowed limit
 *                      (AIN7), or an error occurred during the internal SPI read/write process.
 */
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
/**
 * @brief  Configures the internal bias voltage (VBIAS) for specific analog input pins.
 *
 * @details This function performs a read-modify-write operation on the VBIAS register.
 *          It allows the user to apply or remove a mid-supply voltage ((AVDD + AVSS) / 2)
 *          to specific analog inputs. This is crucial for unbiased sensors (like
 *          thermocouples) to ensure the common-mode voltage sits safely within the
 *          Programmable Gain Amplifier's (PGA) linear operating range.
 *
 * @note    Do not enable VBIAS on both the positive and negative input pins of the
 *          same sensor simultaneously. If the bias voltage generator is enabled on
 *          multiple channels, it will physically short those channels together inside
 *          the chip, resulting in an invalid measurement[cite: 209].
 *
 * @param  hads          Pointer to the ADS1248_Handle_t structure that contains
 *                       the SPI handle and hardware pin configurations.
 * @param  Ads1248_Vbias The bitmask representing the specific analog pin(s) to modify.
 * @param  En_Di         The desired state (ENABLE to apply VBIAS, DISABLE to remove it).
 *
 * @retval HAL_StatusTypeDef HAL status of the SPI transactions:
 *         - HAL_OK:    The VBIAS register was successfully updated.
 *         - HAL_ERROR: An invalid En_Di parameter was passed, or an error occurred
 *                      during the internal SPI read/write process.
 */
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
/**
 * @brief  Configures the internal voltage reference state for the ADS1248.
 *
 * @details This function performs a read-modify-write operation on the MUX1
 *          register to update the VREFCON[1:0] bits. It allows the user to
 *          turn the internal 2.048V reference always on, always off, or configure
 *          it to power down automatically during SLEEP mode to save power.
 *
 * @note    The internal voltage reference MUST be enabled if you intend to use
 *          the internal excitation current sources (IDACs) for RTD measurements,
 *          or if you are using the internal temperature diode[cite: 163].
 *
 * @param  hads            Pointer to the ADS1248_Handle_t structure that contains
 *                         the SPI handle and hardware pin configurations.
 * @param  Ads1248_Vrefcon The desired state for the internal voltage reference
 *                         (of type ADS1248_Vref_State).
 *
 * @retval HAL_StatusTypeDef HAL status of the SPI transactions:
 *         - HAL_OK:    The MUX1 register was successfully updated.
 *         - HAL_ERROR: An error occurred during the internal SPI read/write process.
 */
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
/**
 * @brief  Configures the internal voltage reference state for the ADS1248.
 *
 * @details This function performs a read-modify-write operation on the MUX1
 *          register to update the VREFCON[1:0] bits. It allows the user to
 *          turn the internal 2.048V reference always on, always off, or configure
 *          it to power down automatically during SLEEP mode to save power.
 *
 * @note    The internal voltage reference MUST be enabled if you intend to use
 *          the internal excitation current sources (IDACs) for RTD measurements,
 *          or if you are using the internal temperature diode[cite: 163].
 *
 * @param  hads            Pointer to the ADS1248_Handle_t structure that contains
 *                         the SPI handle and hardware pin configurations.
 * @param  Ads1248_Vrefcon The desired state for the internal voltage reference
 *                         (of type ADS1248_Vref_State).
 *
 * @retval HAL_StatusTypeDef HAL status of the SPI transactions:
 *         - HAL_OK:    The MUX1 register was successfully updated.
 *         - HAL_ERROR: An error occurred during the internal SPI read/write process.
 */
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
/**
 * @brief  Configures the system monitor (MUXCAL) multiplexer settings for the ADS1248.
 *
 * @details This function performs a read-modify-write operation on the MUX1
 *          register to update the MUXCAL bits[cite: 1]. These bits allow the user
 *          to route internal system diagnostic signals directly into the Programmable
 *          Gain Amplifier (PGA) for measurement. Setting this multiplexer state can,
 *          for example, disconnect your external analog inputs and internally short
 *          the ADC inputs to mid-supply to evaluate internal offset errors[cite: 1].
 *
 * @param  hads           Pointer to the ADS1248_Handle_t structure that contains
 *                        the SPI handle and hardware pin configurations.
 * @param  Ads1248_MuxCal The desired system monitor diagnostic configuration
 *                        (of type ADS1248_MuxCal_t).
 *
 * @retval HAL_StatusTypeDef HAL status of the SPI transactions:
 *         - HAL_OK:    The MUX1 register was successfully updated.
 *         - HAL_ERROR: An error occurred during the internal SPI read/write process.
 */
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
 * @brief  Configures the Programmable Gain Amplifier (PGA) setting for the ADS1248.
 *
 * @details This function performs a read-modify-write operation on the SYS0
 *          register to update the internal PGA gain bits. The internal PGA
 *          provides selectable gains from 1 to 128 to amplify small differential
 *          signals before they reach the ADC.
 *
 * @note    When toggling between different input channels, SYS0 settings (such
 *          as the PGA gain and Data Rate) MUST be changed before updating the
 *          MUX0 (multiplexer) settings. Changing the multiplexer before reducing
 *          the gain can temporarily overload the internal amplifiers if the new
 *          channel has a significantly larger voltage.
 *
 * @param  hads        Pointer to the ADS1248_Handle_t structure that contains
 *                     the SPI handle and hardware pin configurations.
 * @param  Ads1248_Pga The desired Programmable Gain Amplifier setting
 *                     (of type ADS1248_GainPga_t).
 *
 * @retval HAL_StatusTypeDef HAL status of the SPI transactions:
 *         - HAL_OK:    The SYS0 register was successfully updated.
 *         - HAL_ERROR: An error occurred during the internal SPI read/write process.
 */
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
/**
 * @brief  Configures the continuous conversion data rate for the ADS1248.
 *
 * @details This function performs a read-modify-write operation on the SYS0
 *          register to update the Data Rate (DR) bits. The data rate dictates
 *          the speed at which the ADC performs conversions and updates the
 *          data output register, with typical options ranging from 5 SPS to 2000 SPS.
 *
 * @note    When dynamically switching between different analog input channels,
 *          SYS0 configurations (such as Data Rate and PGA gain) MUST be updated
 *          prior to changing the MUX0 channel settings. Modifying the multiplexer
 *          before applying the new system configurations can lead to temporary
 *          internal amplifier overload if the new channel has a larger voltage swing.
 *
 * @param  hads             Pointer to the ADS1248_Handle_t structure that contains
 *                          the SPI handle and hardware pin configurations.
 * @param  Ads1248_Datarate The desired data rate (samples per second) setting
 *                          (of type ADS1248_DataRate_t).
 *
 * @retval HAL_StatusTypeDef HAL status of the SPI transactions:
 *         - HAL_OK:    The SYS0 register was successfully updated.
 *         - HAL_ERROR: An error occurred during the internal SPI read/write process.
 */
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
/**
 * @brief  Configures the Data Ready (DRDY) pin mode for the ADS1248.
 *
 * @details This function performs a read-modify-write operation on the IDAC0
 *          register to update the DRDY MODE bit. This configuration determines
 *          the behavior of the DOUT/DRDY pin. It controls whether the pin acts
 *          exclusively as a Data Output (requiring the use of the dedicated
 *          hardware DRDY pin to monitor conversion status) or multiplexes both
 *          the Data Out and Data Ready functions onto a single pin to save
 *          microcontroller I/O lines.
 *
 * @param  hads            Pointer to the ADS1248_Handle_t structure that contains
 *                         the SPI handle and hardware pin configurations.
 * @param  Ads1248_Drdy_Mode The desired DRDY pin behavior configuration
 *                         (of type Ads1248_Drdy).
 *
 * @retval HAL_StatusTypeDef HAL status of the SPI transactions:
 *         - HAL_OK:    The IDAC0 register was successfully read and updated.
 *         - HAL_ERROR: An error occurred during the internal SPI read/write process.
 */
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
/**
 * @brief  Configures the magnitude of the internal excitation current sources (IDACs).
 *
 * @details This function performs read-modify-write operations on the MUX1 and
 *          IDAC0 registers to update the Excitation Current Magnitude (IMAG) bits.
 *          Because the IDACs rely on the internal voltage reference to function,
 *          this function automatically forces the VREFCON bits to 'Always On' mode
 *          whenever a valid, non-zero current magnitude is selected.
 *
 * @note    If you use this function to turn OFF the excitation current
 *          (ADS1248_IMAG_OFF), be aware that it will alter the VREFCON bits.
 *          You must manually reconfigure your desired internal reference state
 *          (e.g., turning it off to save power) using the ADS1248_Set_Vref_State()
 *          function immediately after disabling the excitation current.
 *
 * @param  hads          Pointer to the ADS1248_Handle_t structure that contains
 *                       the SPI handle and hardware configurations.
 * @param  Ads1248_IMag  The desired excitation current magnitude (e.g., Off, 10µA,
 *                       100µA, up to 1500µA) of type ADS1248_IdacMagnitude_t.
 *
 * @retval HAL_StatusTypeDef HAL status of the SPI transactions:
 *         - HAL_OK:    The registers were successfully updated.
 *         - HAL_ERROR: A NULL pointer was passed, the requested current magnitude
 *                      exceeded the maximum limit (1500µA), or an error occurred
 *                      during the internal SPI read/write process.
 */
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

/**
 * @brief  Routes a specific internal excitation current source (IDAC) to an output pin.
 *
 * @details This function performs a read-modify-write operation on the IDAC1
 *          register. It updates the routing bits (I1DIR or I2DIR) to direct
 *          the selected excitation current source to a specific analog input pin,
 *          or disconnects it completely. This routing is essential when providing
 *          excitation currents to passive sensors like RTDs or thermistors.
 *
 * @note    When selecting the excitation current output pin, you must explicitly
 *          specify which of the two independent current sources (IDAC1 or IDAC2)
 *          you are routing. Both IDACs can be routed to different pins simultaneously
 *          (e.g., for 3-wire RTD lead-wire compensation).
 *
 * @param  hads           Pointer to the ADS1248_Handle_t structure that contains
 *                        the SPI handle and hardware configurations.
 * @param  Ads1248_idac   The specific internal current source to configure
 *                        (e.g., ADS1248_IDAC_1 or ADS1248_IDAC_2).
 * @param  Ads1248_Ex_Cur The desired destination analog pin for the selected IDAC
 *                        (of type ADS1248_Exci_Pin_t).
 *
 * @retval HAL_StatusTypeDef HAL status of the SPI transactions:
 *         - HAL_OK:    The IDAC1 register was successfully updated with the new routing.
 *         - HAL_ERROR: A NULL pointer was passed, an invalid IDAC or output pin
 *                      was selected, or an error occurred during the internal
 *                      SPI read/write process.
 */
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
/**
 * @brief  Configures a specific analog pin on the ADS1248 to function as a GPIO.
 *
 * @details This function performs a series of read-modify-write operations across the
 *          GPIOCFG, GPIODIR, and GPIODAT registers. It allows the user to repurpose
 *          eligible analog input pins into digital inputs or digital outputs.
 *          If the mode is set to DISABLE, the function safely tears down the GPIO
 *          configuration, clears any residual digital data states, and returns the
 *          pin to its default analog input function.
 *
 * @param  hads            Pointer to the ADS1248_Handle_t structure that contains
 *                         the SPI handle and hardware pin configurations.
 * @param  Ads1248_Pin     The specific analog pin to configure. Must be a valid
 *                         GPIO-capable pin evaluated by the __IS_ADS1248_GPIO_PIN macro.
 * @param  Ads1248_Ain_Mode The desired operational mode for the pin (e.g.,
 *                         ADS1248_AIN_MODE_INPUT, ADS1248_AIN_MODE_OUTPUT, or
 *                         ADS1248_AIN_MODE_DISABLE to return to analog mode).
 *
 * @retval HAL_StatusTypeDef HAL status of the SPI transactions:
 *         - HAL_OK:    The GPIO configuration, direction, and data registers were
 *                      successfully updated.
 *         - HAL_ERROR: A NULL handle was passed, an invalid pin or mode was provided,
 *                      the selected pin does not support GPIO functionality, or an
 *                      SPI communication error occurred.
 */
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
/**
 * @brief  Sets or clears the digital output state of a specific ADS1248 GPIO pin.
 *
 * @details This function performs a read-modify-write operation on the GPIODAT
 *          register to update the logic level (HIGH or LOW) of a specific analog
 *          pin. It includes macro-based validation to ensure only valid GPIO-capable
 *          pins are modified.
 *
 * @note    Before using this function, the target pin must be explicitly configured
 *          as a digital output using the ADS1248_Set_GPIO_Mode() function. Writing
 *          to the GPIODAT register will have no physical effect if the pin is still
 *          configured as an analog input or a digital input.
 *
 * @param  hads         Pointer to the ADS1248_Handle_t structure that contains
 *                      the SPI handle and hardware configurations.
 * @param  Ads1248_Pin  The specific analog pin to modify. Must be a valid GPIO-capable
 *                      pin evaluated by the __IS_ADS1248_GPIO_PIN macro.
 * @param  Ads1248_Gpio The desired digital logic state for the pin (e.g.,
 *                      ADS1248_GPIO_SET or ADS1248_GPIO_RESET) of type ADS1248_Gpio_Pin_State_t.
 *
 * @retval HAL_StatusTypeDef HAL status of the SPI transactions:
 *         - HAL_OK:    The GPIODAT register was successfully updated with the new state.
 *         - HAL_ERROR: An invalid GPIO state was passed, the target pin does not
 *                      support GPIO functionality, or an error occurred during
 *                      the internal SPI read/write process.
 */
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
/**
 * @brief  Reads the current digital logic state of a specific ADS1248 GPIO pin.
 *
 * @details This function performs a read operation on the GPIODAT register to
 *          retrieve the current logic level of a specific analog pin that has
 *          been configured as a GPIO. It isolates the target pin's state by
 *          masking the retrieved register byte and returns whether the pin is
 *          currently HIGH (SET) or LOW (RESET).
 *
 * @note    Before calling this function, the target pin must be explicitly
 *          configured as a digital input (or output) using the
 *          ADS1248_Set_GPIO_Mode() function. Reading an unconfigured pin will
 *          not yield valid digital logic states.
 *
 * @param  hads        Pointer to the ADS1248_Handle_t structure that contains
 *                     the SPI handle and hardware configurations.
 * @param  Ads1248_Pin The specific analog pin to read. Must be a valid GPIO-capable
 *                     pin evaluated by the __IS_ADS1248_GPIO_PIN macro.
 *
 * @retval uint8_t     The current digital state of the pin, or an error code:
 *         - ADS1248_GPIO_SET:   The pin is currently logic HIGH.
 *         - ADS1248_GPIO_RESET: The pin is currently logic LOW.
 *         - 0xFF:               A NULL handle was passed, or the target pin
 *                               does not support GPIO functionality.
 *         - 0x99:               An error occurred during the internal SPI read
 *                               process.
 */
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
/**
 * @brief  Calculates the required delay time for a calibration operation to complete.
 *
 * @details This internal helper function reads the SYS0 register to determine the
 *          current Data Rate setting. It maps this data rate to a pre-calculated
 *          delay value, ensuring the microcontroller waits the proper amount of time
 *          for the ADC to finish internal offset or gain calibrations (such as
 *          SELFOCAL, SYSOCAL, or SYSGCAL)[cite: 1, 3].
 *
 * @note    This is a static internal function. The delay values correspond to the
 *          settling times required at various sample rates.
 *
 * @param  hads Pointer to the ADS1248_Handle_t structure that contains
 *              the SPI handle and hardware configurations.
 *
 * @retval uint32_t The calculated delay time required for calibration.
 *                  Returns a safe, fallback maximum delay (0x9999) if an SPI
 *                  communication error occurs while reading the SYS0 register.
 */
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
/**
 * @brief  Reads the 24-bit ADC conversion result, applies a manual offset, and converts it to voltage.
 *
 * @details This function is compiled when USE_CONFIG_ADS1248_ADD_OFFSET_VALUES_MANUALLY is enabled.
 *          It retrieves 3 bytes of raw conversion data via SPI (sending NOPs). It then reconstructs
 *          the 24-bit value, performs 24-bit to 32-bit sign extension for negative numbers, subtracts
 *          a user-provided manual offset, and calculates the final floating-point voltage using
 *          the configured LSB weight.
 *
 * @param  hads    Pointer to the ADS1248_Handle_t structure containing the SPI handle and configuration.
 * @param  pData32 Pointer to a 32-bit signed integer containing the manual offset value to subtract.
 * @param  pData   Pointer to a float variable where the final calculated voltage will be stored.
 *
 * @retval HAL_StatusTypeDef HAL status of the SPI transaction:
 *         - HAL_OK:      The data was successfully read and the voltage was calculated.
 *         - HAL_ERROR:   An error occurred during the SPI transmission/reception.
 *         - HAL_BUSY:    The SPI peripheral is currently locked.
 *         - HAL_TIMEOUT: The SPI transaction exceeded the specified delay limit.
 */
HAL_StatusTypeDef ADS1248_Get_Voltage_V_Man_Offset(ADS1248_Handle_t *hads,int32_t *pData32,float *pData)
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

	*pData = (float)(raw_data * hads->Init.__voltage_lsb);

	return status;
}
#else
/**
 * @brief  Reads the 24-bit ADC conversion result and converts it directly to voltage.
 *
 * @details This function is compiled when USE_CONFIG_ADS1248_ADD_OFFSET_VALUES_MANUALLY is disabled.
 *          It retrieves 3 bytes of raw conversion data via SPI (sending NOPs). It reconstructs the
 *          24-bit value, properly handles 24-bit to 32-bit two's complement sign extension for
 *          negative values, and calculates the final floating-point voltage using the configured
 *          LSB weight.
 *
 * @param  hads  Pointer to the ADS1248_Handle_t structure containing the SPI handle and configuration.
 * @param  pData Pointer to a float variable where the final calculated voltage will be stored.
 *
 * @retval HAL_StatusTypeDef HAL status of the SPI transaction:
 *         - HAL_OK:      The data was successfully read and the voltage was calculated.
 *         - HAL_ERROR:   An error occurred during the SPI transmission/reception.
 *         - HAL_BUSY:    The SPI peripheral is currently locked.
 *         - HAL_TIMEOUT: The SPI transaction exceeded the specified delay limit.
 */
HAL_StatusTypeDef ADS1248_Get_Voltage_V(ADS1248_Handle_t *hads,float *pData)
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
	raw_data = (int32_t)((((uint32_t)rx[0]<<16)) | (((uint32_t)rx[1]<<8)) | (((uint32_t)rx[2]<<0)));
	if(raw_data & 0x800000)
	{
		raw_data |= 0xFF000000;
	}

	*pData = (float)(raw_data * hads->Init.__voltage_lsb);

	return status;
}
#endif
/**
 * @brief  Initiates a self offset calibration (SELFOCAL) on the ADS1248.
 *
 * @details This function transmits the SELFOCAL command and waits the required
 *          amount of time for the calibration to complete. During a self offset
 *          calibration, the selected inputs are disconnected from the internal
 *          circuitry and a zero differential signal is applied internally, connecting
 *          the inputs to mid-supply[cite: 1]. The device executes this hardware
 *          sequence on its own and does not overwrite or change the actual value
 *          stored in your MUXCAL register bits[cite: 1]. Upon completion of this
 *          command, the device automatically calculates and updates the internal
 *          Offset Calibration Register (OFC)[cite: 1, 3].
 *
 * @note    Unlike the system offset calibration (SYSOCAL), this command only
 *          corrects the internal offset of the ADC itself and does not require
 *          you to physically short the external input pins[cite: 1].
 *
 * @param  hads Pointer to the ADS1248_Handle_t structure that contains
 *              the SPI handle and hardware configurations.
 *
 * @retval HAL_StatusTypeDef HAL status of the calibration process:
 *         - HAL_OK:    The command was successfully transmitted and the delay completed.
 *         - HAL_ERROR: An error occurred while transmitting the calibration command.
 */
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
/**
 * @brief  Initiates a system offset calibration (SYSOCAL) on the ADS1248.
 *
 * @details This function performs a system-level calibration to correct the offset
 *          of your entire measurement path, including inherent offsets introduced by
 *          your external sensors, wiring, or front-end circuitry[cite: 1]. It reads the
 *          current MUX0 setting to identify the selected positive input pin and automatically
 *          enables the internal VBIAS for that pin. This ensures that the externally
 *          applied 0V differential signal (your physical short) is properly biased to
 *          mid-supply, keeping the internal Programmable Gain Amplifier (PGA) perfectly
 *          within its linear operating range during calibration[cite: 1]. Upon completion,
 *          the internal Offset Calibration Register (OFC) is automatically updated[cite: 3].
 *
 * @note    You must physically short your sensor inputs together at your desired
 *          system "zero" point (e.g., at the screw terminals) before calling this
 *          function[cite: 1].
 *
 * @param  hads Pointer to the ADS1248_Handle_t structure containing the SPI handle.
 *
 * @retval HAL_StatusTypeDef HAL status of the SPI transactions.
 */
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
/**
 * @brief  Initiates a system gain calibration (SYSGCAL) on the ADS1248.
 *
 * @details This function temporarily alters the MUXCAL bits in the MUX1 register
 *          to route the internal full-scale reference voltage directly into the PGA,
 *          sends the SYSGCAL command, waits for the calibration delay, and then
 *          restores normal multiplexer routing. After execution, the device automatically
 *          calculates and updates the Full-Scale Calibration Register (FSC)[cite: 3].
 *
 * @note    You should always perform your chosen offset calibration (SYSOCAL or SELFOCAL)
 *          first before running this gain calibration function[cite: 1].
 *
 * @param  hads Pointer to the ADS1248_Handle_t structure containing the SPI handle.
 *
 * @retval HAL_StatusTypeDef HAL status of the SPI transactions.
 */
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
/**
 * @brief  Manually sets the Offset Calibration (OFC) register value.
 *
 * @details While the ADS1248 calculates and updates the OFC register automatically
 *          when hardware calibration commands like SYSOCAL or SELFOCAL are executed,
 *          this function allows you to directly inject a custom 24-bit system-level
 *          offset correction[cite: 3].
 *
 * @param  hads    Pointer to the ADS1248_Handle_t structure containing the SPI handle.
 * @param  pData32 Pointer to a 32-bit unsigned integer containing the 24-bit offset value.
 *
 * @retval HAL_StatusTypeDef HAL status of the SPI transaction, or HAL_ERROR if
 *                           the provided value exceeds the 24-bit maximum (0xFFFFFF).
 */
HAL_StatusTypeDef ADS1248_Set_Offset_Calib(ADS1248_Handle_t *hads, uint32_t *pData32)
{
	if(*pData32 > 0xFFFFFF)
	{
		return HAL_ERROR;
	}
	return ADS1248_WriteReg_3(hads, ADS1248_REG_OFC0,pData32);
}
/**
 * @brief  Manually sets the Full-Scale Calibration (FSC) register value.
 *
 * @details The device typically updates the FSC register automatically after a SYSGCAL
 *          command or when changing the PGA gain setting (loading factory-trimmed values)[cite: 3].
 *          This function allows you to manually apply custom digital gain error corrections
 *          if your application specifically requires it[cite: 3].
 *
 * @param  hads    Pointer to the ADS1248_Handle_t structure containing the SPI handle.
 * @param  pData32 Pointer to a 32-bit unsigned integer containing the 24-bit full-scale value.
 *
 * @retval HAL_StatusTypeDef HAL status of the SPI transaction, or HAL_ERROR if
 *                           the provided value exceeds the 24-bit maximum (0xFFFFFF).
 */
HAL_StatusTypeDef ADS1248_Set_FullScale_Calib(ADS1248_Handle_t *hads, uint32_t *pData32)
{
	if(*pData32 > 0xFFFFFF)
	{
		return HAL_ERROR;
	}
	return ADS1248_WriteReg_3(hads, ADS1248_REG_FSC0,pData32);
}
/**
 * @brief  Writes a 24-bit (3-byte) data value to a specific ADS1248 register.
 *
 * @details This is a static internal helper function used primarily for writing
 *          custom 24-bit values to the OFC and FSC calibration registers. It separates
 *          the lower 24 bits of a 32-bit variable into three 8-bit bytes (MSB first)
 *          and transmits them via SPI.
 *
 * @param  hads        Pointer to the ADS1248_Handle_t structure containing the SPI handle.
 * @param  Ads1248_Reg The target base register address (e.g., ADS1248_REG_OFC0).
 * @param  pData32     Pointer to the 32-bit variable containing the 24-bit payload.
 *
 * @retval HAL_StatusTypeDef HAL status of the SPI transaction.
 */
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
/**
 * @brief  Retrieves the current status of the ADS1248 clock.
 *
 * @details This function performs a read operation on the MUX1 register and applies
 *          a mask to isolate the CLKSTAT bit. This bit indicates whether the
 *          ADC is currently operating using the internal oscillator or an
 *          external clock source provided to the CLK pin.
 *
 * @param  hads Pointer to the ADS1248_Handle_t structure that contains
 *              the SPI handle and hardware configurations.
 *
 * @retval uint8_t The current clock status:
 *         - 0x00: The internal oscillator is being used.
 *         - 1 :  An external clock is being used.
 *         - 0x99: An SPI communication error occurred while reading the MUX1 register.
 */
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
/**
 * @brief  Reads the 24-bit ADC conversion result from the internal temperature sensor and converts it to Celsius.
 *
 * @details This function retrieves 3 bytes of raw conversion data via SPI (sending NOP commands).
 *          It reconstructs the 24-bit value, properly handles 24-bit to 32-bit two's complement
 *          sign extension for negative numbers, and calculates the final floating-point
 *          temperature using the dedicated internal temperature LSB macro.
 *
 * @note    Before using this function to read temperature, you must ensure that both the
 *          internal voltage reference and the internal temperature diode are enabled in
 *          the device configuration.
 *
 * @param  hads  Pointer to the ADS1248_Handle_t structure containing the SPI handle and hardware configurations.
 * @param  pData Pointer to a float variable where the final calculated temperature will be stored.
 *
 * @retval HAL_StatusTypeDef HAL status of the SPI transaction:
 *         - HAL_OK:      The data was successfully read and the temperature was calculated.
 *         - HAL_ERROR:   An error occurred during SPI transmission/reception.
 *         - HAL_BUSY:    The SPI peripheral is currently locked.
 *         - HAL_TIMEOUT: The SPI transaction exceeded the specified delay limit.
 */
HAL_StatusTypeDef ADS1248_Get_Temp_C(ADS1248_Handle_t *hads,float *pData)
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
	raw_data = (int32_t)((((uint32_t)rx[0]<<16)) | (((uint32_t)rx[1]<<8)) | (((uint32_t)rx[2]<<0)));
	if(raw_data & 0x800000)
	{
		raw_data |= 0xFF000000;
	}

	*pData = (float)__ADS1248_TEMP_LSB(raw_data);

	return status;
}
