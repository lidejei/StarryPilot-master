/**
  ******************************************************************************
  * @file    pwm.c 
  * @author  J Zou
  * @version V1.0
  * @date    11-Oct-2018
  * @brief   PWM Driver
  ******************************************************************************
*/  

#include "pwm.h"
#include "debug.h"

#define PWM_ARR(freq) 	(TIMER_FREQUENCY/freq) 		// CCR reload value, Timer frequency = 3M/60K = 50 Hz

uint8_t _pwm_freq = PWM_DEFAULT_FREQUENCY;
static float _tim_duty_cycle[MAX_PWM_CHAN] = {0.00, 0.00, 0.00, 0.00, 0.00, 0.00, 0.00, 0.00};

void pwm_gpio_init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;

	/* TIM2 clock enable */
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);
	/* TIM3 clock enable */
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);
	/* TIM4 clock enable */
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4, ENABLE);

	/* GPIO clock enable */
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB | RCC_APB2Periph_AFIO, ENABLE);
	
	/* GPIOA Configuration:TIM2 Channel 1, 2 as alternate function push-pull, for IO_CH1, IO_CH2 */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	/* GPIOA Configuration:TIM4 Channel 3, 4 as alternate function push-pull, for IO_CH3, IO_CH4 */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8 | GPIO_Pin_9;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOB, &GPIO_InitStructure);
	/* GPIOA Configuration:TIM3 Channel 1, 2, 3 and 4 as alternate function push-pull, for IO_C51, IO_CH6, IO_CH7 and IO_CH8 */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_7;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1;
	GPIO_Init(GPIOB, &GPIO_InitStructure);
}

void pwm_timer_init(void)
{
	TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
	TIM_OCInitTypeDef  TIM_OCInitStructure;
	uint16_t PrescalerValue = 0;
	RCC_ClocksTypeDef  rcc_clocks;

	// Timer 2,3,4 are all base timer
	RCC_GetClocksFreq(&rcc_clocks);
	uint8_t APB1_Prescaler = rcc_clocks.HCLK_Frequency/rcc_clocks.PCLK1_Frequency;
	uint32_t TimClk = APB1_Prescaler==1 ? rcc_clocks.PCLK1_Frequency : rcc_clocks.PCLK1_Frequency*2;
	/* Compute the prescaler value */
	PrescalerValue = (uint16_t) (TimClk / TIMER_FREQUENCY) - 1;
	/* Time base configuration */
	TIM_TimeBaseStructure.TIM_Period = PWM_ARR(_pwm_freq);
	TIM_TimeBaseStructure.TIM_Prescaler = PrescalerValue;
	TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;

	TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;
	TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
	TIM_OCInitStructure.TIM_Pulse = 0;
	TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;
	TIM_OCInitStructure.TIM_OutputNState = TIM_OutputNState_Disable;
	TIM_OCInitStructure.TIM_OCNPolarity = TIM_OCNPolarity_High;
	TIM_OCInitStructure.TIM_OCIdleState = TIM_OCIdleState_Set;
	TIM_OCInitStructure.TIM_OCNIdleState = TIM_OCIdleState_Reset;

	/* -----------------------------------------------------------------------
	TIM2 Configuration: 
	----------------------------------------------------------------------- */
	TIM_TimeBaseInit(TIM2, &TIM_TimeBaseStructure);

	TIM_OC1Init(TIM2, &TIM_OCInitStructure);
	TIM_OC1PreloadConfig(TIM2, TIM_OCPreload_Enable);
	TIM_OC2Init(TIM2, &TIM_OCInitStructure);
	TIM_OC2PreloadConfig(TIM2, TIM_OCPreload_Enable);

	TIM_ARRPreloadConfig(TIM2, ENABLE);

	/* -----------------------------------------------------------------------
	TIM3 Configuration: 
	----------------------------------------------------------------------- */
	TIM_TimeBaseInit(TIM3, &TIM_TimeBaseStructure);

	TIM_OC1Init(TIM3, &TIM_OCInitStructure);
	TIM_OC1PreloadConfig(TIM3, TIM_OCPreload_Enable);
	TIM_OC2Init(TIM3, &TIM_OCInitStructure);
	TIM_OC2PreloadConfig(TIM3, TIM_OCPreload_Enable);
	TIM_OC3Init(TIM3, &TIM_OCInitStructure);
	TIM_OC3PreloadConfig(TIM3, TIM_OCPreload_Enable);
	TIM_OC4Init(TIM3, &TIM_OCInitStructure);
	TIM_OC4PreloadConfig(TIM3, TIM_OCPreload_Enable);

	TIM_ARRPreloadConfig(TIM3, ENABLE);

	/* -----------------------------------------------------------------------
	TIM4 Configuration: 
	----------------------------------------------------------------------- */
	TIM_TimeBaseInit(TIM4, &TIM_TimeBaseStructure);

	TIM_OC3Init(TIM4, &TIM_OCInitStructure);
	TIM_OC3PreloadConfig(TIM4, TIM_OCPreload_Enable);
	TIM_OC4Init(TIM4, &TIM_OCInitStructure);
	TIM_OC4PreloadConfig(TIM4, TIM_OCPreload_Enable);

	TIM_ARRPreloadConfig(TIM4, ENABLE);
}

uint8_t pwm_write(float* duty_cyc, uint8_t chan_id)
{
	if(chan_id & PWM_CHAN_1){
		TIM_SetCompare1(TIM2, PWM_ARR(_pwm_freq)*duty_cyc[0]);
		_tim_duty_cycle[0] = duty_cyc[0];
		//debug("ch1:%f\n", _tim_duty_cycle[0]);
	}

	if(chan_id & PWM_CHAN_2){
		TIM_SetCompare2(TIM2, PWM_ARR(_pwm_freq)*duty_cyc[1]);
		_tim_duty_cycle[1] = duty_cyc[1];
		//debug("ch2:%f\n", _tim_duty_cycle[1]);
	}
	
	if(chan_id & PWM_CHAN_3){
		TIM_SetCompare3(TIM4, PWM_ARR(_pwm_freq)*duty_cyc[2]);
		_tim_duty_cycle[2] = duty_cyc[2];
		//debug("ch3:%f\n", _tim_duty_cycle[2]);
	}

	if(chan_id & PWM_CHAN_4){
		TIM_SetCompare4(TIM4, PWM_ARR(_pwm_freq)*duty_cyc[3]);
		_tim_duty_cycle[3] = duty_cyc[3];
		//debug("ch4:%f\n", _tim_duty_cycle[3]);
	}

	if(chan_id & PWM_CHAN_5){
		TIM_SetCompare1(TIM3, PWM_ARR(_pwm_freq)*duty_cyc[4]);
		_tim_duty_cycle[4] = duty_cyc[4];
		//debug("ch5:%f\n", _tim_duty_cycle[4]);
	}

	if(chan_id & PWM_CHAN_6){
		TIM_SetCompare2(TIM3, PWM_ARR(_pwm_freq)*duty_cyc[5]);
		_tim_duty_cycle[5] = duty_cyc[5];
		//debug("ch6:%f\n", _tim_duty_cycle[5]);
	}

	if(chan_id & PWM_CHAN_7){
		TIM_SetCompare3(TIM3, PWM_ARR(_pwm_freq)*duty_cyc[6]);
		_tim_duty_cycle[6] = duty_cyc[6];
		//debug("ch7:%f\n", _tim_duty_cycle[6]);
	}

	if(chan_id & PWM_CHAN_8){
		TIM_SetCompare4(TIM3, PWM_ARR(_pwm_freq)*duty_cyc[7]);
		_tim_duty_cycle[7] = duty_cyc[7];
		//debug("ch8:%f\n", _tim_duty_cycle[7]);
	}

	return 0;
}

uint8_t pwm_read(float* buffer, uint8_t chan_id)
{
	for(uint8_t i = 0 ; i < MAX_PWM_CHAN ; i++){
		if(chan_id & (1<<i)){
			buffer[i] = _tim_duty_cycle[i];
		}
	}

	return 0;
}

uint8_t pwm_configure(uint8_t cmd, void *args)
{
	if(cmd == PWM_CMD_SET_FREQ){
		_pwm_freq = *((int*)args);

		// configure timer
		pwm_timer_init();
		// after frequency changing, the timer compare value should be re-configured also
		pwm_write(_tim_duty_cycle, PWM_CHAN_ALL);

		return 0;
	}else if(cmd == PWM_CMD_ENABLE){
		int enable = *((int*)args);
		//debug("pwm enable:%d ", enable);
		if(enable){
			TIM_Cmd(TIM2, ENABLE);
			TIM_Cmd(TIM4, ENABLE);
			TIM_Cmd(TIM3, ENABLE);
		}else{
			TIM_Cmd(TIM2, DISABLE);
			TIM_Cmd(TIM4, DISABLE);
			TIM_Cmd(TIM3, DISABLE);
		}

		return 0;
	}else{
		// unknown command
		return 1;
	}	
}

uint8_t pwm_init(void)
{
	pwm_gpio_init();
	pwm_timer_init();

	TIM_Cmd(TIM2, DISABLE);	// disable by default
	TIM_Cmd(TIM4, DISABLE);
	TIM_Cmd(TIM3, DISABLE);

	return 0;
}
