/**
 * Copyright (C) 2013, 2014 Johannes Taelman
 *
 * This file is part of Axoloti.
 *
 * Axoloti is free software: you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later
 * version.
 *
 * Axoloti is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
 * A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * Axoloti. If not, see <http://www.gnu.org/licenses/>.
 */
#include "ch.h"
#include "hal.h"
#include "axoloti_defines.h"
#include "axoloti_board.h"

void BlinkenLights(void) {
  // LEDS
  palSetPadMode(GPIOE, 9, PAL_MODE_OUTPUT_PUSHPULL);
  palSetPadMode(GPIOE, 11, PAL_MODE_OUTPUT_PUSHPULL);
  palSetPadMode(GPIOE, 13, PAL_MODE_OUTPUT_PUSHPULL);
  palSetPadMode(GPIOE, 14, PAL_MODE_OUTPUT_PUSHPULL);
  int k;
  for (k = 0; k < 1; k++) {
    palSetPad(GPIOE, 9);
    chThdSleepMilliseconds(50);
    palClearPad(GPIOE, 9);
    palSetPad(GPIOE, 11);
    chThdSleepMilliseconds(50);
    palClearPad(GPIOE, 11);
    palSetPad(GPIOE, 13);
    chThdSleepMilliseconds(50);
    palClearPad(GPIOE, 13);
    palSetPad(GPIOE, 14);
    chThdSleepMilliseconds(50);
    palClearPad(GPIOE, 14);
  }
  palClearPad(GPIOE, 9);
  palClearPad(GPIOE, 11);
  palClearPad(GPIOE, 13);
  palClearPad(GPIOE, 14);
}

Mutex Mutex_DMAStream_1_7; // shared: SPI3 (axoloti control) and I2C2 (codec)

void axoloti_board_init(void) {
  chMtxInit(&Mutex_DMAStream_1_7);
}

/*
 * PWM configuration structure.
 * Cyclic callback enabled, channels 1 and 4 enabled without callbacks,
 * the active state is a logic one.
 */
static const PWMConfig pwmcfg = {400000, /* 400kHz PWM clock frequency.  */
                                 4096, /* PWM period is 128 cycles.    */
                                 NULL, { {PWM_OUTPUT_ACTIVE_HIGH, NULL}, {
                                     PWM_OUTPUT_ACTIVE_HIGH, NULL},
                                        {PWM_OUTPUT_ACTIVE_HIGH, NULL}, {
                                            PWM_OUTPUT_ACTIVE_HIGH, NULL}},
                                 /* HW dependent part.*/
                                 0};

void InitPWM(void) {
  /*
   * Initializes the PWM drivers
   */
  pwmStart(&PWMD3, &pwmcfg);
  pwmStart(&PWMD4, &pwmcfg);
  pwmStart(&PWMD5, &pwmcfg);
//  pwmStart(&PWMD8, &pwmcfg);
}

/* Total number of channels to be sampled by a single ADC operation.*/
#define ADC_GRP1_NUM_CHANNELS   16

/* Depth of the conversion buffer, channels are sampled four times each.*/
#define ADC_GRP1_BUF_DEPTH      1

void adc_init(void) {
#if (BOARD_AXOLOTI_V03)
  palSetPadMode(GPIOA, 0, PAL_MODE_INPUT_ANALOG);
  palSetPadMode(GPIOA, 1, PAL_MODE_INPUT_ANALOG);
  palSetPadMode(GPIOA, 2, PAL_MODE_INPUT_ANALOG);
  palSetPadMode(GPIOA, 3, PAL_MODE_INPUT_ANALOG);

  palSetPadMode(GPIOA, 4, PAL_MODE_INPUT_ANALOG);
  palSetPadMode(GPIOA, 5, PAL_MODE_INPUT_ANALOG);
  palSetPadMode(GPIOA, 6, PAL_MODE_INPUT_ANALOG);
  palSetPadMode(GPIOA, 7, PAL_MODE_INPUT_ANALOG);

  palSetPadMode(GPIOB, 0, PAL_MODE_INPUT_ANALOG);
  palSetPadMode(GPIOB, 1, PAL_MODE_INPUT_ANALOG);

  palSetPadMode(GPIOC, 0, PAL_MODE_INPUT_ANALOG);
  palSetPadMode(GPIOC, 1, PAL_MODE_INPUT_ANALOG);
  palSetPadMode(GPIOC, 2, PAL_MODE_INPUT_ANALOG);
  palSetPadMode(GPIOC, 3, PAL_MODE_INPUT_ANALOG);
  palSetPadMode(GPIOC, 4, PAL_MODE_INPUT_ANALOG);
  palSetPadMode(GPIOC, 5, PAL_MODE_INPUT_ANALOG);
  adcStart(&ADCD1, NULL);
#elif (BOARD_STM32F4DISCOVERY)

  palSetPadMode(GPIOA, 0, PAL_MODE_INPUT_ANALOG);
  palSetPadMode(GPIOA, 1, PAL_MODE_INPUT_ANALOG);
  palSetPadMode(GPIOA, 2, PAL_MODE_INPUT_ANALOG);
  palSetPadMode(GPIOA, 3, PAL_MODE_INPUT_ANALOG);
  // skip GPIOA4: LRCLK
  // skip GPIOA5,GPIOA6,GPIOA7: accelerometer
  palSetPadMode(GPIOB, 0, PAL_MODE_INPUT_ANALOG);
  palSetPadMode(GPIOB, 1, PAL_MODE_INPUT_ANALOG);
  //skip GPIOPC0: USB PowerOn
  palSetPadMode(GPIOC, 1, PAL_MODE_INPUT_ANALOG);
  palSetPadMode(GPIOC, 2, PAL_MODE_INPUT_ANALOG);
  palSetPadMode(GPIOC, 3, PAL_MODE_INPUT_ANALOG);
  palSetPadMode(GPIOC, 4, PAL_MODE_INPUT_ANALOG);
  palSetPadMode(GPIOC, 5, PAL_MODE_INPUT_ANALOG);
  adcStart(&ADCD1, NULL);
#else
#error "ADC: No board defined?"
#endif

}

/*
 * ADC samples buffer.
 */
unsigned short adcvalues[ADC_GRP1_NUM_CHANNELS * ADC_GRP1_BUF_DEPTH] __attribute__ ((section (".sram2")));

/*
 * ADC conversion group.
 * Mode:        Linear buffer, 8 samples of 1 channel, SW triggered.
 * Channels:    IN11.
 */
static const ADCConversionGroup adcgrpcfg1 = {FALSE,      //circular buffer mode
    ADC_GRP1_NUM_CHANNELS,        //Number of the analog channels
    NULL,                         //Callback function (not needed here)
    0,             //Error callback
    0, /* CR1 */
    ADC_CR2_SWSTART, /* CR2 */
    ADC_SMPR1_SMP_AN10(ADC_SAMPLE_84) | ADC_SMPR1_SMP_AN11(ADC_SAMPLE_84)
        | ADC_SMPR1_SMP_AN12(ADC_SAMPLE_84) | ADC_SMPR1_SMP_AN13(ADC_SAMPLE_84)
        | ADC_SMPR1_SMP_AN14(ADC_SAMPLE_84) | ADC_SMPR1_SMP_AN15(ADC_SAMPLE_84), //sample times ch10-18
    ADC_SMPR2_SMP_AN0(ADC_SAMPLE_84) | ADC_SMPR2_SMP_AN1(ADC_SAMPLE_84)
        | ADC_SMPR2_SMP_AN2(ADC_SAMPLE_84) | ADC_SMPR2_SMP_AN3(ADC_SAMPLE_84)
        | ADC_SMPR2_SMP_AN4(ADC_SAMPLE_84) | ADC_SMPR2_SMP_AN5(ADC_SAMPLE_84)
        | ADC_SMPR2_SMP_AN6(ADC_SAMPLE_84) | ADC_SMPR2_SMP_AN7(ADC_SAMPLE_84)
        | ADC_SMPR2_SMP_AN8(ADC_SAMPLE_84) | ADC_SMPR2_SMP_AN9(ADC_SAMPLE_84), //sample times ch0-9
    ADC_SQR1_SQ13_N(ADC_CHANNEL_IN12) | ADC_SQR1_SQ14_N(ADC_CHANNEL_IN13)
        | ADC_SQR1_SQ15_N(ADC_CHANNEL_IN14) | ADC_SQR1_SQ16_N(ADC_CHANNEL_IN15)
        | ADC_SQR1_NUM_CH(ADC_GRP1_NUM_CHANNELS), //SQR1: Conversion group sequence 13...16 + sequence length
    ADC_SQR2_SQ7_N(ADC_CHANNEL_IN6) | ADC_SQR2_SQ8_N(ADC_CHANNEL_IN7)
        | ADC_SQR2_SQ9_N(ADC_CHANNEL_IN8) | ADC_SQR2_SQ10_N(ADC_CHANNEL_IN9)
        | ADC_SQR2_SQ11_N(ADC_CHANNEL_IN10) | ADC_SQR2_SQ12_N(ADC_CHANNEL_IN11), //SQR2: Conversion group sequence 7...12
    ADC_SQR3_SQ1_N(ADC_CHANNEL_IN0) | ADC_SQR3_SQ2_N(ADC_CHANNEL_IN1)
        | ADC_SQR3_SQ3_N(ADC_CHANNEL_IN2) | ADC_SQR3_SQ4_N(ADC_CHANNEL_IN3)
        | ADC_SQR3_SQ5_N(ADC_CHANNEL_IN4) | ADC_SQR3_SQ6_N(ADC_CHANNEL_IN5) //SQR3: Conversion group sequence 1...6
};

void adc_convert(void) {
  adcStopConversion(&ADCD1);
  adcStartConversion(&ADCD1, &adcgrpcfg1, adcvalues, ADC_GRP1_BUF_DEPTH);
}

