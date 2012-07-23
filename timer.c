#include "timer.h"
#include "gpio.h"
#include "Sensors/ppg.h"

/**
  * @brief  Configure the timer channels for PWM out on CRT board
  * @param  None
  * @retval None
  * This initialiser function assumes the clocks and gpio have been configured
  */
void setup_pwm(void) {
  /* -----------------------------------------------------------------------
    TIM4 Configuration: generate 2 PWM signals with 2 different duty cycles:
    The TIM4CLK frequency is set to SystemCoreClock (Hz), to get TIM4 counter
    clock at 72 MHz the Prescaler is computed as following:
     - Prescaler = (TIM4CLK / TIM4 counter clock) - 1
    SystemCoreClock is set to 72 MHz

    The TIM4 is running at 11.905KHz: TIM4 Frequency = TIM4 counter clock/(ARR + 1)
                                                  = 4.5 MHz / 378
    (with 239.5clk adc sampling -> 252adc clk/sample, and 12mhz adc clk this gives quadrature
    sampling)

  ----------------------------------------------------------------------- */
  TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure={};
  TIM_OCInitTypeDef  TIM_OCInitStructure={};
  /*Enable the Tim2 clk*/
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);
  /*Enable the Tim4 clk*/
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4, ENABLE);
  /*Enable the Tim1 clk*/
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM1, ENABLE);
  #if BOARD>=3
  /*Enable the Tim3 clk*/
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);
  #endif

  TIM_DeInit(TIM1);
  TIM_DeInit(TIM2);
  #if BOARD>=3
  TIM_DeInit(TIM3);
  #endif
  TIM_DeInit(TIM4);
  /* Prescaler of 16 times*/
  uint16_t PrescalerValue = 15;
  /* Time base configuration  - timer 4 as pwm2*/
  #if BOARD<3
  TIM_TimeBaseStructure.TIM_Period = NORMAL_PWM_PERIOD(0);
  #else
  TIM_TimeBaseStructure.TIM_Period = NORMAL_PWM_PERIOD(2);//PWM2 on revision 3 boards
  #endif
  TIM_TimeBaseStructure.TIM_Prescaler = PrescalerValue;
  TIM_TimeBaseStructure.TIM_ClockDivision = 0;
  TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
  /*setup 4*/
  TIM_TimeBaseInit(TIM4, &TIM_TimeBaseStructure);

  /*Setup the initstructure*/
  TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
  TIM_OCInitStructure.TIM_Pulse = 4;
  #if BOARD<3
  TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1; //Mode 1 on early board revisions
  /* PWM1 Mode configuration: Channel3 */
  TIM_OC3Init(TIM4, &TIM_OCInitStructure);
  TIM_OC3PreloadConfig(TIM4, TIM_OCPreload_Enable);
  #else
  TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM2; //As Board revisions >=3 using P channel mosfet drivers, so inverted levels
  #endif
  /* PWM1 Mode configuration: Channel4 */
  TIM_OC4Init(TIM4, &TIM_OCInitStructure);
  TIM_OC4PreloadConfig(TIM4, TIM_OCPreload_Enable);

  /* TIM4 enable counter */
  TIM_ARRPreloadConfig(TIM4, ENABLE);


  /*Now setup timer2 as PWM0*/
  #if BOARD<3
  TIM_TimeBaseStructure.TIM_Period = NORMAL_PWM_PERIOD(1);
  #else
  TIM_TimeBaseStructure.TIM_Period = NORMAL_PWM_PERIOD(0);
  #endif
  TIM_TimeBaseInit(TIM2, &TIM_TimeBaseStructure);//same as timer4
  /* PWM1 Mode configuration: Channel3 */
  TIM_OC3Init(TIM2, &TIM_OCInitStructure);

  TIM_OC3PreloadConfig(TIM2, TIM_OCPreload_Enable);

  /* TIM2 enable counter */
  TIM_ARRPreloadConfig(TIM2, ENABLE);

  #if BOARD>=3
  /*Now setup timer3 as PWM1*/
  TIM_TimeBaseStructure.TIM_Period = NORMAL_PWM_PERIOD(1);
  TIM_TimeBaseInit(TIM3, &TIM_TimeBaseStructure);//same as timer4
  /* PWM1 Mode configuration: Channel1 */
  TIM_OC1Init(TIM3, &TIM_OCInitStructure);

  TIM_OC1PreloadConfig(TIM3, TIM_OCPreload_Enable);

  /* TIM3 enable counter */
  TIM_ARRPreloadConfig(TIM3, ENABLE);
  #endif

  /*We enable all the timers at once with interrupts disabled*/
  __disable_irq();
  #if BOARD<3
    TIM_Cmd(TIM4, ENABLE);
    #if PPG_CHANNELS>1
      TIM_Cmd(TIM2, ENABLE);
    #endif
  #else
    TIM_Cmd(TIM2, ENABLE);
    #if PPG_CHANNELS>1
      TIM_Cmd(TIM3, ENABLE);
    #endif
    #if PPG_CHANNELS>2
      TIM4->CNT=PWM_PERIOD_CENTER/2;//This causes the third timer to be in antiphase, giving reduce peak ADC signal
      TIM_Cmd(TIM4, ENABLE);
    #endif
  #endif
  __enable_irq();

  /*Now setup timer1 as motor control */
  PrescalerValue = 0;//no prescaler
  TIM_TimeBaseStructure.TIM_Period = 2047;//gives a slower frequency - 35KHz, meeting Rohm BD6231F spec, and giving 11 bits of res each way
  TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;//Make sure we have mode1 
  TIM_OCInitStructure.TIM_OutputNState = TIM_OutputNState_Disable;//These settings need to be applied on timers 1 and 8                 
  TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High; 
  TIM_OCInitStructure.TIM_OCIdleState = TIM_OCIdleState_Reset;
  TIM_TimeBaseInit(TIM1, &TIM_TimeBaseStructure);//same as timer4 
  /* PWM1 Mode configuration: Channel1 */
  TIM_OC1Init(TIM1, &TIM_OCInitStructure);
  TIM_OC1PreloadConfig(TIM1, TIM_OCPreload_Enable);
  TIM_CtrlPWMOutputs(TIM1, ENABLE);	//Needs to be applied on 1 and 8
  /* TIM1 enable counter */
  TIM_ARRPreloadConfig(TIM1, ENABLE);
  TIM_Cmd(TIM1, ENABLE); 
  Set_PWM_Motor(0);			//Make sure motor off
}

/**
  * @brief Try to correct the timer phase by adjusting the reload register for one pwm cycle
  * @param Pointer to unsigned 32 bit integer bitmask for the timers to be corrected
  * @retval none
  * Note: this could be improved, atm it just uses some macros and is hardcoded for the timers, but making it more flexible would need arrays of pointers
  * (need to use different timers for each output) 
  */
void Tryfudge(uint32_t* Fudgemask) {
	if((*Fudgemask)&(uint32_t)0x01 && TIM2->CNT<(FUDGED_PWM_PERIOD(0)-2)) {//If the second bit is set, adjust the first timer in the list if it is safe to do so
		TIM2->CR1 &= ~TIM_CR1_ARPE;//Disable reload buffering so we can load directly
		TIM2->ARR = FUDGED_PWM_PERIOD(0);//Load reload register directly
		TIM2->CR1 |= TIM_CR1_ARPE;//Enable buffering so we load buffered register
		TIM2->ARR = NORMAL_PWM_PERIOD(0);//Load the buffer, so the pwm period returns to normal after 1 period
		*Fudgemask&=~(uint32_t)0x01;//Clear the bit
	}
	if((*Fudgemask)&(uint32_t)0x04 && TIM4->CNT<(FUDGED_PWM_PERIOD(2)-2)) {//If the first bit is set, adjust the first timer in the list if it is safe to do so
		TIM4->CR1 &= ~TIM_CR1_ARPE;//Disable reload buffering so we can load directly
		TIM4->ARR = FUDGED_PWM_PERIOD(2);//Load reload register directly
		TIM4->CR1 |= TIM_CR1_ARPE;//Enable buffering so we load buffered register
		TIM4->ARR = NORMAL_PWM_PERIOD(2);//Load the buffer, so the pwm period returns to normal after 1 period
		*Fudgemask&=~(uint32_t)0x04;//Clear the bit
	}
}

/**
  * @brief  Configure the timer channel for PWM out to pump motor, -ive duty turns on valve
  * @param  None
  * @retval None
  * setting duty=0 gives idle state
  */
void Set_Motor(int16_t duty) {
	duty=(duty>MAX_DUTY)?MAX_DUTY:duty;	//enforce limits on range
	if(duty<0) {//We are dumping with the solenoid valve
		SET_SOLENOID(1);
		Set_PWM_Motor(0);
	}
	else {//We are driving the pump
		SET_SOLENOID(0);
		Set_PWM_Motor(duty);
	}
}
