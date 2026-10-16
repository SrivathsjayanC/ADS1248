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

/**
 * @defgroup ADS1248_Reference_Voltages ADS1248 Reference Voltage Values
 * @brief    Macros defining the voltage values (in Volts) for the ADC reference sources.
 * @details  These floating-point values are used by the driver to calculate the LSB weight
 *           for voltage and temperature conversions.
 *
 *           Note: If you are going to use an external reference as your source, you must
 *           manually update the ADS1248_VREF_VLTG_REF0 or ADS1248_VREF_VLTG_REF1 macro
 *           with the exact hardware voltage applied to those pins.
 * @{
 */
#define ADS1248_VREF_VLTG_INT  2.0474f  /*!< Voltage value of the internal reference */
#define ADS1248_VREF_VLTG_REF0 2.048f   /*!< Voltage value of the external reference on REFP0/REFN0 */
#define ADS1248_VREF_VLTG_REF1 0.0f     /*!< Voltage value of the external reference on REFP1/REFN1 */
/**
 * @}
 */


/**
 * @defgroup ADS1248_Configuration_Switches ADS1248 Driver Configuration
 * @brief    Compile-time switches to configure driver behavior for calibration and offsets.
 * @{
 */

/**
 * @brief  Enables automatic hardware calibration for the ADS1248.
 *
 * @details When set to 1, the driver compiles functions that utilize the ADC's
 *          built-in hardware calibration commands (SYSOCAL, SYSGCAL, SELFOCAL).
 *          Executing these commands causes the device to automatically calculate
 *          and update its internal Offset Calibration (OFC) and Full-Scale
 *          Calibration (FSC) registers[cite: 3]. When set to 0, the driver compiles
 *          functions allowing the user to manually inject custom 24-bit values
 *          directly into the OFC and FSC registers.
 */
#define USE_CONFIG_ADS1248_SET_CALIB_VALUES_AUTOMATICALLY 1

/**
 * @brief  Enables manual software-based offset subtraction.
 *
 * @details When set to 1, the driver compiles the ADS1248_Get_Voltage_V_Man_Offset()
 *          function. This allows the application to pass a custom 32-bit software
 *          offset that is mathematically subtracted from the raw 24-bit ADC reading
 *          prior to floating-point voltage conversion. When set to 0, the standard
 *          ADS1248_Get_Voltage_V() function is compiled for direct conversion.
 */
#define USE_CONFIG_ADS1248_ADD_OFFSET_VALUES_MANUALLY 0

/**
 * @}
 */
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


/**
 * @brief  ADS1248 Burnout Current Source (BCS) configuration.
 *
 * @details These values specify the magnitude of the internal burnout current
 *          sources. This diagnostic feature is used to detect open-circuit
 *          sensor faults (e.g., a broken thermocouple or RTD wire) by injecting
 *          a small, known current into the measurement path. If the sensor is
 *          broken, this current will force the ADC reading to a full-scale value.
 */
typedef enum
{
	ADS1248_BCS_0_UA   = 0x00U,  /*!< Burnout current source disabled (0 µA) */
	ADS1248_BCS_0_5_UA = 0x01U,  /*!< 0.5 µA burnout current selected */
	ADS1248_BCS_2_UA   = 0x02U,  /*!< 2 µA burnout current selected */
	ADS1248_BCS_10_UA  = 0x03U   /*!< 10 µA burnout current selected */
} ADS1248_Burnout_Current;

/**
 * @brief  ADS1248 Multiplexer Positive Input Channel Selection.
 *
 * @details These values specify which physical analog input pin is routed
 *          to the positive side of the internal Programmable Gain Amplifier
 *          (PGA). This routing is controlled by the MUX0 register.
 */
typedef enum
{
	ADS1248_MUX_SP_AIN0 = 0x00,  /*!< Selects Analog Input 0 (AIN0) as the positive input */
	ADS1248_MUX_SP_AIN1 = 0x01,  /*!< Selects Analog Input 1 (AIN1) as the positive input */
	ADS1248_MUX_SP_AIN2 = 0x02,  /*!< Selects Analog Input 2 (AIN2) as the positive input */
	ADS1248_MUX_SP_AIN3 = 0x03,  /*!< Selects Analog Input 3 (AIN3) as the positive input */
	ADS1248_MUX_SP_AIN4 = 0x04,  /*!< Selects Analog Input 4 (AIN4) as the positive input */
	ADS1248_MUX_SP_AIN5 = 0x05,  /*!< Selects Analog Input 5 (AIN5) as the positive input */
	ADS1248_MUX_SP_AIN6 = 0x06,  /*!< Selects Analog Input 6 (AIN6) as the positive input */
	ADS1248_MUX_SP_AIN7 = 0x07   /*!< Selects Analog Input 7 (AIN7) as the positive input */
} Ads1248_Mux_PosInput_t;
/**
 * @brief  ADS1248 Multiplexer Negative Input Channel Selection.
 *
 * @details These values specify which physical analog input pin is routed
 *          to the negative side of the internal Programmable Gain Amplifier
 *          (PGA). This routing is controlled by the lower nibble of the
 *          MUX0 register.
 */
typedef enum
{
    ADS1248_MUX_SN_AIN0 = 0x00,  /*!< Selects Analog Input 0 (AIN0) as the negative input */
    ADS1248_MUX_SN_AIN1 = 0x01,  /*!< Selects Analog Input 1 (AIN1) as the negative input */
    ADS1248_MUX_SN_AIN2 = 0x02,  /*!< Selects Analog Input 2 (AIN2) as the negative input */
    ADS1248_MUX_SN_AIN3 = 0x03,  /*!< Selects Analog Input 3 (AIN3) as the negative input */
    ADS1248_MUX_SN_AIN4 = 0x04,  /*!< Selects Analog Input 4 (AIN4) as the negative input */
    ADS1248_MUX_SN_AIN5 = 0x05,  /*!< Selects Analog Input 5 (AIN5) as the negative input */
    ADS1248_MUX_SN_AIN6 = 0x06,  /*!< Selects Analog Input 6 (AIN6) as the negative input */
    ADS1248_MUX_SN_AIN7 = 0x07   /*!< Selects Analog Input 7 (AIN7) as the negative input */
} Ads1248_Mux_NegInput_t;

/**
 * @brief  ADS1248 Multiplexer Calibration and System Monitor Routing.
 *
 * @details These values configure the MUXCAL bits in the MUX1 register. They are
 *          used to temporarily override normal analog input routing to measure
 *          internal system monitors (like temperature, supply voltages, and references),
 *          or to force specific internal pathways for manual calibration routines.
 */
typedef enum {
    ADS1248_MUXCAL_NORMAL       = 0x00,  /*!< Normal operation; selected external analog inputs are routed to the PGA */
    ADS1248_MUXCAL_OFFSET_CALIB = 0x01,  /*!< Offset calibration; disconnects external inputs and internally shorts the ADC inputs to mid-supply[cite: 1] */
    ADS1248_MUXCAL_GAIN_CALIB   = 0x02,  /*!< Gain calibration; routes the internal full-scale reference directly into the PGA */
    ADS1248_MUXCAL_TEMP         = 0x03,  /*!< System monitor; routes the internal temperature sensor diode to the PGA */
    ADS1248_MUXCAL_REF1         = 0x04,  /*!< System monitor; routes the external REF1 measurement to the PGA */
    ADS1248_MUXCAL_REF0         = 0x05,  /*!< System monitor; routes the external REF0 measurement to the PGA */
    ADS1248_MUXCAL_ANALOG_SUP   = 0x06,  /*!< System monitor; routes the analog supply (AVDD - AVSS) / 4 to the PGA */
    ADS1248_MUXCAL_DIGITAL_SUP  = 0x07   /*!< System monitor; routes the digital supply (DVDD - DVSS) / 4 to the PGA */
} ADS1248_MuxCal_t;
/**
 * @brief  ADS1248 Voltage Reference Source Selection.
 *
 * @details These values specify the voltage reference source used by the ADC
 *          for its conversions. This routing is configured via the REFSELT bits
 *          in the MUX2 register. The driver allows you to choose between the
 *          on-board internal reference, external hardware references, or routing
 *          the internal reference out to external pins.
 */
typedef enum {
    ADS1248_REFSEL_REFP0_REFN0     = 0x00,  /*!< Selects the external reference connected to the REFP0 and REFN0 pins */
    ADS1248_REFSEL_REFP1_REFN1     = 0x01,  /*!< Selects the external reference connected to the REFP1 and REFN1 pins */
    ADS1248_REFSEL_INT             = 0x02,  /*!< Selects the internal voltage reference */
    ADS1248_REFSEL_INT_REFP0_REFN0 = 0x03   /*!< Selects the internal voltage reference and physically routes it out to the REFP0 and REFN0 pins */
} Ads1248_Reference_Src_t;s1248_Reference_Src_t;

/**
 * @brief  ADS1248 Programmable Gain Amplifier (PGA) Settings.
 *
 * @details These values configure the internal PGA gain, which is controlled
 *          by bits [6:4] in the SYS0 register. Adjusting the gain allows the
 *          ADC to measure smaller differential signals with higher resolution.
 *          Note that whenever you change the PGA gain setting, the device
 *          automatically loads the appropriate factory-trimmed Full-Scale
 *          Calibration (FSC) value into the FSC register[cite: 3].
 */
typedef enum {
    ADS1248_PGA_1   = 0x00,  /*!< PGA Gain = 1 */
    ADS1248_PGA_2   = 0x01,  /*!< PGA Gain = 2 */
    ADS1248_PGA_4   = 0x02,  /*!< PGA Gain = 4 */
    ADS1248_PGA_8   = 0x03,  /*!< PGA Gain = 8 */
    ADS1248_PGA_16  = 0x04,  /*!< PGA Gain = 16 */
    ADS1248_PGA_32  = 0x05,  /*!< PGA Gain = 32 */
    ADS1248_PGA_64  = 0x06,  /*!< PGA Gain = 64 */
    ADS1248_PGA_128 = 0x07   /*!< PGA Gain = 128 */
} ADS1248_GainPga_t;

/**
 * @brief  ADS1248 Output Data Rate Settings.
 *
 * @details These values configure the continuous output data rate (samples per second)
 *          of the ADC, controlled by bits [3:0] in the SYS0 register. The selected
 *          data rate influences the overall noise performance of the conversions and
 *          dictates the required settling delay time when executing hardware
 *          calibration commands (SYSOCAL, SYSGCAL, SELFOCAL).
 */
typedef enum {
    ADS1248_DATARATE_5SPS    = 0x00,  /*!< 5 Samples Per Second (SPS) */
    ADS1248_DATARATE_10SPS   = 0x01,  /*!< 10 Samples Per Second (SPS) */
    ADS1248_DATARATE_20SPS   = 0x02,  /*!< 20 Samples Per Second (SPS) */
    ADS1248_DATARATE_40SPS   = 0x03,  /*!< 40 Samples Per Second (SPS) */
    ADS1248_DATARATE_80SPS   = 0x04,  /*!< 80 Samples Per Second (SPS) */
    ADS1248_DATARATE_160SPS  = 0x05,  /*!< 160 Samples Per Second (SPS) */
    ADS1248_DATARATE_320SPS  = 0x06,  /*!< 320 Samples Per Second (SPS) */
    ADS1248_DATARATE_640SPS  = 0x07,  /*!< 640 Samples Per Second (SPS) */
    ADS1248_DATARATE_1000SPS = 0x08,  /*!< 1000 Samples Per Second (SPS) */
    ADS1248_DATARATE_2000SPS = 0x09   /*!< 2000 Samples Per Second (SPS) */
} ADS1248_DataRate_t;

/**
 * @brief  ADS1248 Excitation Current Magnitude (IDAC) Settings.
 *
 * @details These values configure the output magnitude of the internal
 *          excitation current sources (IDAC1 and IDAC2). This is controlled
 *          by the IMAG bits in the IDAC0 register. These programmable currents
 *          are primarily used to provide excitation for passive sensors such
 *          as RTDs (Resistance Temperature Detectors) or thermistors.
 */
typedef enum {
    ADS1248_IMAG_OFF     = 0x00,  /*!< Excitation current disabled (Off) */
    ADS1248_IMAG_50_UA   = 0x01,  /*!< 50 µA excitation current selected */
    ADS1248_IMAG_100_UA  = 0x02,  /*!< 100 µA excitation current selected */
    ADS1248_IMAG_250_UA  = 0x03,  /*!< 250 µA excitation current selected */
    ADS1248_IMAG_500_UA  = 0x04,  /*!< 500 µA excitation current selected */
    ADS1248_IMAG_750_UA  = 0x05,  /*!< 750 µA excitation current selected */
    ADS1248_IMAG_1000_UA = 0x06,  /*!< 1000 µA (1 mA) excitation current selected */
    ADS1248_IMAG_1500_UA = 0x07   /*!< 1500 µA (1.5 mA) excitation current selected */
} ADS1248_IdacMagnitude_t;

/**
 * @brief  ADS1248 Internal Voltage Reference (VREFCON) Configuration.
 *
 * @details These values specify the operating state of the ADC's internal
 *          voltage reference. This state is controlled by the VREFCON bits
 *          in the MUX1 register.
 *
 * @note    If you are actively using the internal excitation current sources
 *          (IDAC1/IDAC2) for sensors like RTDs, the internal reference must
 *          be explicitly configured to 'Always On' mode.
 */
typedef enum
{
	ADS1248_VREFCON_INT_ALWAYS_OFF = 0x00,  /*!< Internal reference is always OFF (default state, lowest power) */
	ADS1248_VREFCON_INT_ALWAYS_ON  = 0x01,  /*!< Internal reference is always ON (required for IDAC operation) */
	ADS1248_VREFCON_INT_CONV_ON    = 0x02   /*!< Internal reference turns ON automatically only during active conversions */
} ADS1248_Vref_State;

/**
 * @brief  ADS1248 Data Ready (DRDY) Pin Mode Configuration.
 *
 * @details These values configure the behavior of the DOUT/DRDY pin,
 *          controlled by the DRDY MODE bit in the IDAC0 register. It
 *          determines whether the pin acts exclusively as a data output,
 *          or if it multiplexes both the data output and the data ready
 *          conversion status to save microcontroller I/O lines.
 */
typedef enum
{
	ADS1248_DRDY_MODE_DATA_OUT_ONLY = 0x00U,  /*!< DOUT/DRDY pin acts only as Data Out (requires dedicated DRDY pin for status) */
	ADS1248_DRDY_MODE_DATA_RDY_OUT  = 0x01U   /*!< DOUT/DRDY pin multiplexes both Data Out and Data Ready signals */
} Ads1248_Drdy;
/**
 * @brief  ADS1248 Excitation Current Output Pin Routing.
 *
 * @details These values specify the physical pin where the internal excitation
 *          current sources (IDAC1 or IDAC2) are routed. This routing is
 *          controlled by the IDAC1 register. It allows you to direct the
 *          programmable current to specific analog inputs or dedicated excitation pins.
 */
typedef enum
{
	ADS1248_EX_CUR_OUT_AIN0   = 0x00,  /*!< Routes excitation current to Analog Input 0 (AIN0) */
	ADS1248_EX_CUR_OUT_AIN1   = 0x01,  /*!< Routes excitation current to Analog Input 1 (AIN1) */
	ADS1248_EX_CUR_OUT_AIN2   = 0x02,  /*!< Routes excitation current to Analog Input 2 (AIN2) */
	ADS1248_EX_CUR_OUT_AIN3   = 0x03,  /*!< Routes excitation current to Analog Input 3 (AIN3) */
	ADS1248_EX_CUR_OUT_AIN4   = 0x04,  /*!< Routes excitation current to Analog Input 4 (AIN4) */
	ADS1248_EX_CUR_OUT_AIN5   = 0x05,  /*!< Routes excitation current to Analog Input 5 (AIN5) */
	ADS1248_EX_CUR_OUT_AIN6   = 0x06,  /*!< Routes excitation current to Analog Input 6 (AIN6) */
	ADS1248_EX_CUR_OUT_AIN7   = 0x07,  /*!< Routes excitation current to Analog Input 7 (AIN7) */
	ADS1248_EX_CUR_OUT_IEXC1  = 0x08,  /*!< Routes excitation current to the dedicated IEXC1 pin */
	ADS1248_EX_CUR_OUT_IEXC2  = 0x09,  /*!< Routes excitation current to the dedicated IEXC2 pin */
	ADS1248_EX_CUR_OUT_DISCON = 0x0F   /*!< Excitation current is disconnected (not routed to any pin) */
} ADS1248_Exci_Pin_t;

/**
 * @brief  ADS1248 GPIO Pin State.
 *
 * @details These values specify the logic state (low or high) of the ADS1248's
 *          programmable GPIO pins. The ADS1248 allows certain analog pins to be
 *          reconfigured as general-purpose digital I/O. This enum is used to
 *          read or write the logic levels of those pins via the GPIODAT register.
 */
typedef enum
{
	ADS1248_GPIO_RESET = 0U,  /*!< GPIO pin logic state is Low (0) */
	ADS1248_GPIO_SET   = 1U   /*!< GPIO pin logic state is High (1) */
} ADS1248_Gpio_Pin_State_t;

/**
 * @defgroup ADS1248_GPIO_Pins ADS1248 GPIO Pin Selections
 * @brief    Macros defining the user input values for GPIO configuration.
 * @details  These values are passed as arguments to driver functions to independently
 *           select and configure specific analog pins as digital I/O within the
 *           GPIOCFG, GPIODIR, and GPIODAT registers.
 * @{
 */
#define ADS1248_PIN_REFP0     (1U << 0U)  /*!< Selects the REFP0 pin */
#define ADS1248_PIN_REFN0     (1U << 1U)  /*!< Selects the REFN0 pin */
#define ADS1248_PIN_AIN_2     (1U << 2U)  /*!< Selects the Analog Input 2 (AIN2) pin */
#define ADS1248_PIN_AIN_3     (1U << 3U)  /*!< Selects the Analog Input 3 (AIN3) pin */
#define ADS1248_PIN_AIN_4     (1U << 4U)  /*!< Selects the Analog Input 4 (AIN4) pin */
#define ADS1248_PIN_AIN_5     (1U << 5U)  /*!< Selects the Analog Input 5 (AIN5) pin */
#define ADS1248_PIN_AIN_6     (1U << 6U)  /*!< Selects the Analog Input 6 (AIN6) pin */
#define ADS1248_PIN_AIN_7     (1U << 7U)  /*!< Selects the Analog Input 7 (AIN7) pin */
/**
 * @}
 */

/**
 * @defgroup ADS1248_VBIAS_Pins ADS1248 VBIAS Pin Selections
 * @brief    Macros defining the user input values to select pins for internal bias voltage routing.
 * @details  These values are passed as arguments to driver functions to configure
 *           the VBIAS register. The device features an internal VBIAS module that
 *           automatically outputs (AVDD+AVSS)/2[cite: 1].
 *
 * @note     If you enable the internal bias voltage generator on multiple channels
 *           simultaneously, it internally shorts those channels together inside
 *           the chip[cite: 1]. When biasing an external short for calibration
 *           (SYSOCAL), ensure you apply VBIAS to only one pin so the ADC measures
 *           the true real-world offset of your physical board layout[cite: 1].
 * @{
 */
#define ADS1248_VBIAS_AIN0     (1U << 0U)  /*!< Selects Analog Input 0 (AIN0) for mid-supply bias */
#define ADS1248_VBIAS_AIN1     (1U << 1U)  /*!< Selects Analog Input 1 (AIN1) for mid-supply bias */
#define ADS1248_VBIAS_AIN2     (1U << 2U)  /*!< Selects Analog Input 2 (AIN2) for mid-supply bias */
#define ADS1248_VBIAS_AIN3     (1U << 3U)  /*!< Selects Analog Input 3 (AIN3) for mid-supply bias */
#define ADS1248_VBIAS_AIN4     (1U << 4U)  /*!< Selects Analog Input 4 (AIN4) for mid-supply bias */
#define ADS1248_VBIAS_AIN5     (1U << 5U)  /*!< Selects Analog Input 5 (AIN5) for mid-supply bias */
#define ADS1248_VBIAS_AIN6     (1U << 6U)  /*!< Selects Analog Input 6 (AIN6) for mid-supply bias */
#define ADS1248_VBIAS_AIN7     (1U << 7U)  /*!< Selects Analog Input 7 (AIN7) for mid-supply bias */
/**
 * @}
 */

/**
 * @defgroup ADS1248_GPIO_Mode ADS1248 AIN/GPIO Pin Mode Configuration
 * @brief    Macros defining the operational mode of the shared AIN/GPIO pins.
 * @details  These values are used by the driver to configure the GPIOCFG and
 *           GPIODIR registers. They determine whether a specific pin operates
 *           as a standard analog input, or if it is repurposed as a digital
 *           input or output.
 * @{
 */
#define ADS1248_AIN_MODE_OUTPUT  0U  /*!< Pin configured as a digital output */
#define ADS1248_AIN_MODE_INPUT   1U  /*!< Pin configured as a digital input */
#define ADS1248_AIN_MODE_DISABLE 2U  /*!< GPIO function disabled; pin acts as a standard analog input */
/**
 * @}
 */

/**
 * @defgroup ADS1248_IDAC_Selection ADS1248 Excitation Current Source (IDAC) Selection
 * @brief    Macros to select which internal excitation current source to configure.
 * @details  The ADS1248 features two independent programmable current sources
 *           (IDAC1 and IDAC2). These values are passed as arguments to driver
 *           functions when you need to specify which of the two current sources
 *           you are routing to a specific pin.
 * @{
 */
#define ADS1248_IDAC_1           0U  /*!< Selects internal excitation current source 1 (IDAC1) */
#define ADS1248_IDAC_2           1U  /*!< Selects internal excitation current source 2 (IDAC2) */
/**
 * @}
 */

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
#define __ADS1248_TEMP_LSB(ADC_24)   (25U + ((((ADC_24*ADS1248_VREF_VLTG_INT)/8388608.0f) - 0.118f)/0.000405f))

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
