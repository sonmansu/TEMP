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
  
  /*  조도센서 핀 */
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
  
}

void GPIO_Configure(void)
{
  GPIO_InitTypeDef GPIO_InitStructure;
  // 조도센서 핀 C0 에 연결 (B0에도 가능)
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
  GPIO_Init(GPIOC, &GPIO_InitStructure);
}
//ADC init 함수
void ADC_Configure(void) {
  ADC_InitTypeDef ADC_InitStructure;
  // ADC1 Configuration
  ADC_InitStructure.ADC_Mode = ADC_Mode_Independent; //조도 센서 하나만 사용하므로
  ADC_InitStructure.ADC_ScanConvMode = DISABLE; //// 데피니션 보고 싱글 채널 사용하므로 DISABLE
  ADC_InitStructure.ADC_ContinuousConvMode = ENABLE; // 연속적으로 해야돼서 이네이블.. 레퍼런스 219 
  ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None; //외부 입력핀에 의한 트리거 비활성화
  ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right; //디폴트
  ADC_InitStructure.ADC_NbrOfChannel = 1; //채널 1개 사용
  ADC_Init(ADC1, &ADC_InitStructure); //위 설정을 ADC에 적용
  
  //조도센서 핀 B0에 꼽았으면 CHANNEL_8 써야되고 C0에 꽂으면 CHANNEL_10 써야함. 
  ADC_RegularChannelConfig(ADC1, ADC_Channel_10, 1, ADC_SampleTime_239Cycles5);  //10채널 단독 사용
  
  //  ADC_ITConfig(ADC1, ADC_IT_EOC, ENABLE); // interrupt enable 해야하나 무분별한 핸들러 호출 방지를 위해 주석 처리
  ADC_Cmd(ADC1, ENABLE); // ADC1 활성화
  
  ADC_ResetCalibration(ADC1);
  
  while(ADC_GetResetCalibrationStatus(ADC1));
  ADC_StartCalibration(ADC1);
  while(ADC_GetCalibrationStatus(ADC1));
  ADC_SoftwareStartConvCmd(ADC1, ENABLE);
}
void ADC1_2_IRQHandler() {
  if(ADC_GetITStatus(ADC1, ADC_IT_EOC) != RESET) { //End Of Conversion, ADC 변환이 끝났을 때
    value = ADC_GetConversionValue(ADC1);               //조도센서 값 입력       
    ADC_ClearITPendingBit(ADC1, ADC_IT_EOC);
  }
}  

void NVIC_Configure(void) {
  
  NVIC_InitTypeDef NVIC_InitStructure;
  
  // TODO: fill the arg you want
  NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
  
  // TODO: Initialize the NVIC using the structure 'NVIC_InitTypeDef' and the function 'NVIC_Init'
  NVIC_Init(&NVIC_InitStructure);
  
  //    NVIC_EnableIRQ(ADC1_2_IRQn);//ADC IRQ 인터럽트 활성화
  NVIC_InitStructure.NVIC_IRQChannel = ADC1_2_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0; 
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0; 
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);
}

void Delay(void) {
  int i;
  
  for (i = 0; i < 2000000; i++) {}
}

uint16_t x1 = 0, y1 = 0;

int main(void)
{
  SystemInit();
  RCC_Configure();
  GPIO_Configure();
  ADC_Configure();
  NVIC_Configure();
  
  //--------------
  LCD_Init();
  Touch_Configuration();
  Touch_Adjust(); //정확한 초점 맞추기 
  LCD_Clear(WHITE);
  
  char *msg = "THU_Team07";
  //    LCD_ShowCharString(10, 10, msg, BROWN, WHITE); 
  LCD_ShowString(40, 40, "THU_Team07", BROWN, WHITE); 
  
  
  while (1) { 
    // TODO: implement
    
    Touch_GetXY(&x1, &y1, 1);
    Convert_Pos(x1, y1, &x1, &y1); //이게 있어야 제 자리에 점이 찍힘 
    
    LCD_ShowNum(50,50, x1, 4, BLACK, WHITE);
    LCD_ShowNum(50, 70, y1, 4, BLACK, WHITE);
    
    ADC_ITConfig(ADC1,ADC_IT_EOC,ENABLE);  //  adc 출력할때만 인터럽트 호출. adc 변환 끝났을때 인터럽트 발생함
    LCD_ShowNum(60,100, value, 4, BLACK, WHITE);
    ADC_ITConfig(ADC1,ADC_IT_EOC,DISABLE); // 무분별한 인터럽트 방지를 위해 비활성화
    LCD_DrawCircle(x1, y1, 4);
  }
  
  return 0;
}
//10주차꺼 하던거 백업. 조도센서 값이 안나오고 여러모로 잘안됨 ..
