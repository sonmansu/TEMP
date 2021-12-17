#include "stm32f10x.h"
#include "stm32f10x_exti.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_usart.h"
#include "stm32f10x_rcc.h"
#include "misc.h"
#include "lcd.h"
#include "touch.h"

/* function prototype */
void RCC_Configure(void);
void GPIO_Configure(void);
void EXTI_Configure(void);
void NVIC_Configure(void);
void Delay(void);

//---------------------------------------------------------------------------------------------------
uint16_t value = 0;
int color[12] = {WHITE,CYAN,BLUE,RED,MAGENTA,LGRAY,GREEN,YELLOW,BROWN,BRRED,GRAY};

void RCC_Configure(void)
{
  // TODO: Enable the APB2 peripheral clock using the function 'RCC_APB2PeriphClockCmd'
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE); 
  
  /*  LED PD4, 7 핀 */ 
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOD, ENABLE);
  // 1초마다 켜지게 하기 위한 tim2
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);
  
  /* 서보모터 */
  // 서보모터 pwm 신호 주기 위한 tim3
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);
  // 서보모터 연결 핀 PB0 (TIM3_채널3은 PB0핀 써야함)
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
  //PB0 을 TIM3_CH3으로 사용하기 위해선 ALTERNATE FUNCTION 사용함
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);

}

void GPIO_Configure(void)
{
  GPIO_InitTypeDef GPIO_InitStructure; 
  // LED PD4, 7
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4 | GPIO_Pin_7; //이래도 되나 yes 
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(GPIOD, &GPIO_InitStructure);
  // 서보모터 PB0
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(GPIOB, &GPIO_InitStructure);
}

void NVIC_Configure(void) {
  
  NVIC_InitTypeDef NVIC_InitStructure;
  
  // TODO: fill the arg you want
  NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
  
  // TODO: Initialize the NVIC using the structure 'NVIC_InitTypeDef' and the function 'NVIC_Init'
  NVIC_Init(&NVIC_InitStructure);
  
  //    NVIC_EnableIRQ(ADC1_2_IRQn);//ADC IRQ 인터럽트 활성화
  NVIC_InitStructure.NVIC_IRQChannel = TIM2_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0; 
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0; 
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);
}
void TIM2_Configure() { // LED 점멸에 사용
  uint16_t prescale = (uint16_t) (SystemCoreClock/ 10000); //SYSCLK_FREQ_72MHz
  
  TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure;
  
  TIM_TimeBaseInitStructure.TIM_Period = 10000;
  TIM_TimeBaseInitStructure.TIM_Prescaler = prescale;
  TIM_TimeBaseInitStructure.TIM_ClockDivision = 0;
  TIM_TimeBaseInitStructure.TIM_CounterMode = TIM_CounterMode_Down;
  
  TIM_TimeBaseInit(TIM2, &TIM_TimeBaseInitStructure);
  TIM_ARRPreloadConfig(TIM2, ENABLE);
  TIM_Cmd(TIM2, ENABLE);
}
uint16_t led_count = 0;

void TIM2_IRQHandler() {
  if (TIM_GetITStatus(TIM2, TIM_IT_Update) != RESET) {
    // 1번 led 1초마다 토글 
    if (led_count % 2 == 0) {
      GPIO_SetBits(GPIOD, GPIO_Pin_4);  
    } else {
      GPIO_ResetBits(GPIOD, GPIO_Pin_4); 
    }
    // 2번 led 5초마다 토글되도록 함 
    if (led_count == 0) {
      GPIO_SetBits(GPIOD, GPIO_Pin_7); 
    } else if (led_count == 5) {
      GPIO_ResetBits(GPIOD, GPIO_Pin_7); 
    }
    led_count++;
    led_count %= 10;
    
    TIM_ClearITPendingBit(TIM2, TIM_IT_Update);
  }
}

TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure;
TIM_OCInitTypeDef TIM_OCInitStructure; //TIM Output Compare 

void PWM_Configure() {
   //주파수 50Hz로 맞춤
   uint16_t prescale = (uint16_t) (SystemCoreClock / 1000000);
   TIM_TimeBaseInitStructure.TIM_Period = 20000;
   TIM_TimeBaseInitStructure.TIM_Prescaler = prescale;
   TIM_TimeBaseInitStructure.TIM_ClockDivision = 0;
   TIM_TimeBaseInitStructure.TIM_CounterMode = TIM_CounterMode_Down;
    
  TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;
  TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;
  TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
  TIM_OCInitStructure.TIM_Pulse =  1500; //us//TODO;duty ratio 들어가는 부분 데이터 시트 참고 
  TIM_OC3Init(TIM3, &TIM_OCInitStructure);
  
  TIM_TimeBaseInit(TIM3, &TIM_TimeBaseInitStructure);
  TIM_OC3PreloadConfig(TIM3, TIM_OCPreload_Disable);
  TIM_ARRPreloadConfig(TIM3, ENABLE);
  TIM_Cmd(TIM3, ENABLE);
}

void TIM3_Pulse_Change(int n){
  TIM_OCInitStructure.TIM_Pulse = n; // us
  TIM_OC3Init(TIM3, &TIM_OCInitStructure);
}

uint16_t x = 0, y = 0;

int main(void)
{
  SystemInit();
  RCC_Configure();
  GPIO_Configure();
  NVIC_Configure();
  TIM2_Configure();
  PWM_Configure();  
  //--------------
  LCD_Init();
  Touch_Configuration();
  Touch_Adjust(); //정확한 초점 맞추기 
  LCD_Clear(WHITE);
  
  LCD_ShowString(30, 30, "THU_Team07", BROWN, WHITE); 
  
  char *msg_led = "ON";
  LCD_ShowString(30, 50, msg_led, BROWN, WHITE); 
  TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE);

  LCD_DrawRectangle(40, 80, 80, 120);
  LCD_ShowString(50, 90, "BTN", BROWN, WHITE); 
  
  int flagLed = 0;
  
  while (1) { 
    // TODO: implement
    Touch_GetXY(&x, &y, 1);    //터치 좌표를 저장 
    Convert_Pos(x, y, &x, &y); //받은 좌표를 lcd 크기에 맞게 변환
    if ( 50 <= x && x <= 100 && 50 <= y && y <= 100) { //BTN 영역 터치시
      if (flagLed) {
          LCD_ShowString(30, 50, "ON ", BROWN, WHITE); 
          TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE); //인터럽트 ENABLE
          TIM3_Pulse_Change(700); //서보모터 각도 -90도 
          flagLed = 0;
      } else {
          LCD_ShowString(30, 50, "OFF", BROWN, WHITE); 
          TIM_ITConfig(TIM2, TIM_IT_Update, DISABLE);  //인터럽트 DISENABLE      
          TIM3_Pulse_Change(2300); //서보모터 각도 90도 
          flagLed = 1;
      }
    }
  }
  
  return 0;
}
