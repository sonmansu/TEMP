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
int color[12] = {WHITE,CYAN,BLUE,RED,MAGENTA,LGRAY,GREEN,YELLOW,BROWN,BRRED,GRAY};

void RCC_Configure(void)
{
  // TODO: Enable the APB2 peripheral clock using the function 'RCC_APB2PeriphClockCmd'  
  /*  조도센서 핀 */
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE); 

  /* DMA */
  RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);
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
  
  //  ADC_ITConfig(ADC1, ADC_IT_EOC, ENABLE); 
  ADC_DMACmd(ADC1, ENABLE); // 인터럽트를 쓰지말고 DMA를 이용해야 하므로 ADC_ITConfig 대신 이 함수 써야함.
  ADC_Cmd(ADC1, ENABLE); // ADC1 활성화
  
  ADC_ResetCalibration(ADC1);
  while(ADC_GetResetCalibrationStatus(ADC1));
  ADC_StartCalibration(ADC1);
  while(ADC_GetCalibrationStatus(ADC1));
  ADC_SoftwareStartConvCmd(ADC1, ENABLE);
}
//volatile unsigned 32bits
volatile uint32_t ADC_value[1];

void DMA_Configure(void) {
    DMA_InitTypeDef DMA_InitStructure;
      
    DMA_DeInit(DMA1_Channel1); // 1-7중에서 1번 선택
    DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)&ADC1->DR; // 어디에 있는 걸 가져올 지(ADC->DR = conversion 된 데이터 결과 쓰여짐)
    DMA_InitStructure.DMA_MemoryBaseAddr = (uint32_t)&ADC_value[0]; // 가져온 걸 어디에 쓸 지
    DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralSRC; //이 adc핀의 값을 읽어오는 sourse 이므로
    DMA_InitStructure.DMA_BufferSize = 1;
      
    DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;  // peripheral address 레지스터는 증가하면 안될듯 
    DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable; // 메모리 주소 register는 증가해야되나? 하면 안될거같은데 잘 모르겠다
    DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Word;
    DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Word;
    DMA_InitStructure.DMA_Mode = DMA_Mode_Circular;
    DMA_InitStructure.DMA_Priority = DMA_Priority_High;
    DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;
    DMA_Init(DMA1_Channel1, &DMA_InitStructure);
    DMA_Cmd(DMA1_Channel1, ENABLE); // 채널 Enable 
}

uint16_t x1 = 0, y1 = 0;

int main(void)
{
  SystemInit();
  RCC_Configure();
  GPIO_Configure();
  ADC_Configure();
  DMA_Configure();
  //--------------
  LCD_Init();
  Touch_Configuration();
//  Touch_Adjust(); //정확한 초점 맞추기 
  LCD_Clear(WHITE);
 
  int threshold = 700; //평소엔 1200~1500 / 플래시 비추면 200
  
  while (1) {
    if (ADC_value[0] < threshold) { //플래시로 비췄을 때
      LCD_Clear(GRAY);
      LCD_ShowNum(50, 50, ADC_value[0], 5, YELLOW, GRAY);
    } else {
      LCD_Clear(WHITE);
      LCD_ShowNum(50, 50, ADC_value[0], 5, BLACK, WHITE);
    }
  }
  return 0;
}
//12주차 수민 
