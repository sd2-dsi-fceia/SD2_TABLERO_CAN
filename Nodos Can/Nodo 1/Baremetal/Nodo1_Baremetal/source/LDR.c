/**
 * @file LDR.c
 * @author Zuliani, Agustin
 */

#include "LDR.h"
#include "fsl_adc16.h"
#include "fsl_common.h"
#include "fsl_port.h"
#include "fsl_gpio.h"
#include "pin_mux.h"

#define ADC16_BASE ADC0
#define ADC16_CHANNEL_GROUP 0U
#define ADC16_LDR_CHANNEL 3U /* PTE20, ADC0_SE0 */

#define ADC16_IRQn ADC0_IRQn

/**
 * @brief Bandera de finalizacion de conversion de adc.
 */
volatile bool g_Adc16ConversionDoneFlag = false;
/**
 * @brief Valor de la ultima conversion.
 *
 * Retiene el valor de la ultima conversion del adc.
 */
volatile uint32_t g_Adc16ConversionValue;

static adc16_config_t adc16ConfigStruct;
static adc16_channel_config_t adc16ChannelConfigStruct;

extern Error_t LDR_init(void)
{
	/* Configuracion del ADC */
	ADC16_GetDefaultConfig(&adc16ConfigStruct);
	ADC16_Init(ADC16_BASE, &adc16ConfigStruct);
	ADC16_EnableHardwareTrigger(ADC16_BASE, false);

#if defined(FSL_FEATURE_ADC16_HAS_CALIBRATION) && FSL_FEATURE_ADC16_HAS_CALIBRATION
	if (kStatus_Success == ADC16_DoAutoCalibration(ADC16_BASE))
	{
	}
	else
	{
		return ERROR_INIT;
	}
#endif /* FSL_FEATURE_ADC16_HAS_CALIBRATION */

	adc16ChannelConfigStruct.channelNumber = ADC16_LDR_CHANNEL;
	adc16ChannelConfigStruct.enableInterruptOnConversionCompleted = true;
	adc16ChannelConfigStruct.enableDifferentialConversion = false;

	g_Adc16ConversionDoneFlag = false;

	EnableIRQ(ADC16_IRQn);

	/* Configuracion de los pines */
	/* Port E Clock Gate Control: Clock enabled */
	CLOCK_EnableClock(kCLOCK_PortE);

	/* PORTE22 (pin 20) is configured as ADC0_SE3 */
	PORT_SetPinMux(BOARD_ADC_LIGHT_SNS_PORT, BOARD_ADC_LIGHT_SNS_PIN,
			kPORT_PinDisabledOrAnalog);

	return ERROR_OK;
}

extern void LDR_read(void)
{
	g_Adc16ConversionValue = ADC16_GetChannelConversionValue(ADC16_BASE,
			ADC16_CHANNEL_GROUP);

	return;
}

extern uint16_t LDR_UltimaConversion(void)
{
	return g_Adc16ConversionValue;
}

extern Error_t LDR_convertir(void)
{
	ADC16_SetChannelConfig(ADC16_BASE, ADC16_CHANNEL_GROUP,
			&adc16ChannelConfigStruct);

	return ERROR_OK;
}

extern bool LDR_getConvComplete(void)
{
	return g_Adc16ConversionDoneFlag;
}

extern void LDR_setConvComplete(void)
{
	g_Adc16ConversionDoneFlag = true;

	return;
}

extern void LDR_clearConvComplete(void)
{
	g_Adc16ConversionDoneFlag = false;

	return;
}
