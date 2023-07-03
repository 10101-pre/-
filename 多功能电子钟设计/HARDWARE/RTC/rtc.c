#include "rtc.h"
#include "led.h"
#include "delay.h"
#include "usart.h" 
#include "beep.h"
#include "gui.h"
//////////////////////////////////////////////////////////////////////////////////	 
//������ֻ��ѧϰʹ�ã�δ��������ɣ��������������κ���;
//ALIENTEK STM32F407������
//RTC ��������	   
//����ԭ��@ALIENTEK
//������̳:www.openedv.com
//��������:2014/5/5
//�汾��V1.1
//��Ȩ���У�����ؾ���
//Copyright(C) ������������ӿƼ����޹�˾ 2014-2024
//All rights reserved									  
//********************************************************************************
//�޸�˵��
//V1.1 20140726
//����:RTC_Get_Week����,���ڸ�����������Ϣ,�õ�������Ϣ.
////////////////////////////////////////////////////////////////////////////////// 
extern int alarm_allow;
extern int alarm_clock_working;
extern void show_date(void);
extern void show_time(void);
extern void draw_time(void);
extern void show_temprature(void);
extern int now;
NVIC_InitTypeDef   NVIC_InitStructure;

//RTCʱ������
//hour,min,sec:Сʱ,����,����
//ampm:@RTC_AM_PM_Definitions  :RTC_H12_AM/RTC_H12_PM
//����ֵ:SUCEE(1),�ɹ�
//       ERROR(0),�����ʼ��ģʽʧ�� 
ErrorStatus RTC_Set_Time(u8 hour,u8 min,u8 sec,u8 ampm)
{
	RTC_TimeTypeDef RTC_TimeTypeInitStructure;
	ErrorStatus sta;
	RTC_TimeTypeInitStructure.RTC_Hours=hour;
	RTC_TimeTypeInitStructure.RTC_Minutes=min;
	RTC_TimeTypeInitStructure.RTC_Seconds=sec;
	RTC_TimeTypeInitStructure.RTC_H12=ampm;
	sta=RTC_SetTime(RTC_Format_BIN,&RTC_TimeTypeInitStructure);
	set_alarm_clock(get_next_alarm_clock());
	return sta;
	
}
//RTC��������
//year,month,date:��(0~99),��(1~12),��(0~31)
//week:����(1~7,0,�Ƿ�!)
//����ֵ:SUCEE(1),�ɹ�
//       ERROR(0),�����ʼ��ģʽʧ�� 
ErrorStatus RTC_Set_Date(u8 year,u8 month,u8 date,u8 week)
{
	ErrorStatus sta;
	RTC_DateTypeDef RTC_DateTypeInitStructure;
	RTC_DateTypeInitStructure.RTC_Date=date;
	RTC_DateTypeInitStructure.RTC_Month=month;
	RTC_DateTypeInitStructure.RTC_WeekDay=week;
	RTC_DateTypeInitStructure.RTC_Year=year;
	sta=RTC_SetDate(RTC_Format_BIN,&RTC_DateTypeInitStructure);
	set_alarm_clock(get_next_alarm_clock());
	return sta;
}

//RTC��ʼ��
//����ֵ:0,��ʼ���ɹ�;
//       1,LSE����ʧ��;
//       2,�����ʼ��ģʽʧ��;
void init_alarm_clock(void);
u8 My_RTC_Init(void)
{
	RTC_InitTypeDef RTC_InitStructure;
	u16 retry=0X1FFF; 
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR, ENABLE);//ʹ��PWRʱ��
	PWR_BackupAccessCmd(ENABLE);	//ʹ�ܺ󱸼Ĵ������� 
	init_alarm_clock();
	if(RTC_ReadBackupRegister(RTC_BKP_DR0)!=0x5050)		//�Ƿ��һ������?
	{
		RCC_LSEConfig(RCC_LSE_ON);//LSE ����    
		while (RCC_GetFlagStatus(RCC_FLAG_LSERDY) == RESET)	//���ָ����RCC��־λ�������,�ȴ����پ������
			{
			retry++;
			delay_ms(10);
			}
		if(retry==0)return 1;		//LSE ����ʧ��. 
			
		RCC_RTCCLKConfig(RCC_RTCCLKSource_LSE);		//����RTCʱ��(RTCCLK),ѡ��LSE��ΪRTCʱ��    
		RCC_RTCCLKCmd(ENABLE);	//ʹ��RTCʱ�� 

    RTC_InitStructure.RTC_AsynchPrediv = 0x7F;//RTC�첽��Ƶϵ��(1~0X7F)
    RTC_InitStructure.RTC_SynchPrediv  = 0xFF;//RTCͬ����Ƶϵ��(0~7FFF)
    RTC_InitStructure.RTC_HourFormat   = RTC_HourFormat_24;//RTC����Ϊ,24Сʱ��ʽ
    RTC_Init(&RTC_InitStructure);
 
		RTC_Set_Time(10,43,1,RTC_H12_AM);	//����ʱ��
		RTC_Set_Date(23,6,22,4);		//��������
	 
		RTC_WriteBackupRegister(RTC_BKP_DR0,0x5050);	//����Ѿ���ʼ������
	} 
 
	return 0;
}

//��������ʱ��(����������,24Сʱ��)
//week:���ڼ�(1~7) @ref  RTC_Alarm_Definitions
//hour,min,sec:Сʱ,����,����
void RTC_Set_AlarmA(u8 week,u8 hour,u8 min,u8 sec)
{ 
	EXTI_InitTypeDef   EXTI_InitStructure;
	RTC_AlarmTypeDef RTC_AlarmTypeInitStructure;
	RTC_TimeTypeDef RTC_TimeTypeInitStructure;
	
	RTC_AlarmCmd(RTC_Alarm_A,DISABLE);//�ر�����A 
	
  RTC_TimeTypeInitStructure.RTC_Hours=hour;//Сʱ
	RTC_TimeTypeInitStructure.RTC_Minutes=min;//����
	RTC_TimeTypeInitStructure.RTC_Seconds=sec;//��
	RTC_TimeTypeInitStructure.RTC_H12=RTC_H12_AM;
  
	RTC_AlarmTypeInitStructure.RTC_AlarmDateWeekDay=week;//����
	RTC_AlarmTypeInitStructure.RTC_AlarmDateWeekDaySel=RTC_AlarmDateWeekDaySel_WeekDay;//��������
	RTC_AlarmTypeInitStructure.RTC_AlarmMask=RTC_AlarmMask_None;//��ȷƥ�����ڣ�ʱ����
	RTC_AlarmTypeInitStructure.RTC_AlarmTime=RTC_TimeTypeInitStructure;
  RTC_SetAlarm(RTC_Format_BIN,RTC_Alarm_A,&RTC_AlarmTypeInitStructure);
 
	
	RTC_ClearITPendingBit(RTC_IT_ALRA);//���RTC����A�ı�־
  EXTI_ClearITPendingBit(EXTI_Line17);//���LINE17�ϵ��жϱ�־λ 
	
	RTC_ITConfig(RTC_IT_ALRA,ENABLE);//��������A�ж�
	RTC_AlarmCmd(RTC_Alarm_A,ENABLE);//��������A 
	
	EXTI_InitStructure.EXTI_Line = EXTI_Line17;//LINE17
  EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;//�ж��¼�
  EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising; //�����ش��� 
  EXTI_InitStructure.EXTI_LineCmd = ENABLE;//ʹ��LINE17
  EXTI_Init(&EXTI_InitStructure);//����

	NVIC_InitStructure.NVIC_IRQChannel = RTC_Alarm_IRQn; 
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0x02;//��ռ���ȼ�1
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0x02;//�����ȼ�2
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;//ʹ���ⲿ�ж�ͨ��
  NVIC_Init(&NVIC_InitStructure);//����
}

//�����Ի��Ѷ�ʱ������  
/*wksel:  @ref RTC_Wakeup_Timer_Definitions
#define RTC_WakeUpClock_RTCCLK_Div16        ((uint32_t)0x00000000)
#define RTC_WakeUpClock_RTCCLK_Div8         ((uint32_t)0x00000001)
#define RTC_WakeUpClock_RTCCLK_Div4         ((uint32_t)0x00000002)
#define RTC_WakeUpClock_RTCCLK_Div2         ((uint32_t)0x00000003)
#define RTC_WakeUpClock_CK_SPRE_16bits      ((uint32_t)0x00000004)
#define RTC_WakeUpClock_CK_SPRE_17bits      ((uint32_t)0x00000006)
*/
//cnt:�Զ���װ��ֵ.����0,�����ж�.
void RTC_Set_WakeUp(u32 wksel,u16 cnt)
{ 
	EXTI_InitTypeDef   EXTI_InitStructure;
	
	RTC_WakeUpCmd(DISABLE);//�ر�WAKE UP
	
	RTC_WakeUpClockConfig(wksel);//����ʱ��ѡ��
	
	RTC_SetWakeUpCounter(cnt);//����WAKE UP�Զ���װ�ؼĴ���
	
	
	RTC_ClearITPendingBit(RTC_IT_WUT); //���RTC WAKE UP�ı�־
  EXTI_ClearITPendingBit(EXTI_Line22);//���LINE22�ϵ��жϱ�־λ 
	 
	RTC_ITConfig(RTC_IT_WUT,ENABLE);//����WAKE UP ��ʱ���ж�
	RTC_WakeUpCmd( ENABLE);//����WAKE UP ��ʱ����
	
	EXTI_InitStructure.EXTI_Line = EXTI_Line22;//LINE22
  EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;//�ж��¼�
  EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising; //�����ش��� 
  EXTI_InitStructure.EXTI_LineCmd = ENABLE;//ʹ��LINE22
  EXTI_Init(&EXTI_InitStructure);//����
 
 
	NVIC_InitStructure.NVIC_IRQChannel = RTC_WKUP_IRQn; 
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0x02;//��ռ���ȼ�1
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0x02;//�����ȼ�2
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;//ʹ���ⲿ�ж�ͨ��
  NVIC_Init(&NVIC_InitStructure);//����
}

//RTC�����жϷ�����
void RTC_Alarm_IRQHandler(void)
{ int length;
	if(RTC_GetFlagStatus(RTC_FLAG_ALRAF)==SET)//ALARM A�ж�?
	{
		alarm_clock_working=1;
		length=50;
		RTC_ClearFlag(RTC_FLAG_ALRAF);//����жϱ�־
		alarm_allow=1;
		BEEP=1;
		while(alarm_allow&&length--){
			if(now==CLOCK){
			show_date();
			show_time();
			draw_time();
			show_temprature();
			}
			BEEP=!BEEP;
			LED1=!LED1;
			delay_ms(100);
		}
		BEEP=0;
		LED1=1;
		alarm_clock_working=0;
		set_alarm_clock(get_next_alarm_clock());
	}   
	EXTI_ClearITPendingBit(EXTI_Line17);	//����ж���17���жϱ�־ 											 
}

//RTC WAKE UP�жϷ�����
void RTC_WKUP_IRQHandler(void)
{    
	if(RTC_GetFlagStatus(RTC_FLAG_WUTF)==SET)//WK_UP�ж�?
	{ 
		RTC_ClearFlag(RTC_FLAG_WUTF);	//����жϱ�־
	}    
	EXTI_ClearITPendingBit(EXTI_Line22);//����ж���22���жϱ�־ 								
}
 
//����ʱ��
struct time time_begin={0,0,0};
struct alarm_clock my_alarm_clock;
int seconds_between(struct time begin,struct time end){//��ȡ��ʱ��֮�������
	  struct time temp1=begin,temp2=end;
	  int length=0; 
	  if(temp1.week==0&&temp2.week==0){
			 length=(temp2.hour-temp1.hour)*60*60+(temp2.minute-temp1.minute)*60;
			 if(length<0)
				 length+=60*60*24;
				 
		}
	  else if(temp1.week==0||temp2.week==0){
		    length=(temp2.hour-temp1.hour)*60*60+(temp2.minute-temp1.minute)*60;
			if(length<0)
				 length+=60*60*24;
		}
		else{
		   length=(temp2.week-temp1.week)*60*60*24+(temp2.hour-temp1.hour)*60*60+(temp2.minute-temp1.minute)*60;
				if(length<0)
				 length+=60*60*24*7;
		}
		return length;
}
void copy_to_time(RTC_DateTypeDef const*src,RTC_TimeTypeDef const*src1,struct time*dst){
    dst->week=src->RTC_WeekDay;
	  dst->hour=src1->RTC_Hours;
	  dst->minute=src1->RTC_Minutes;
}
void init_alarm_clock(void){
     my_alarm_clock.point=0;
     my_alarm_clock.maxsize=6;
}

struct time get_next_alarm_clock(void){//��ȡ��һ��Ҫ���õ�����
    RTC_DateTypeDef RTC_DateStruct;	
	  RTC_TimeTypeDef RTC_TimeStruct;
   	int length,i,min_length;
	  struct time now,dst;
	
	  RTC_GetDate(RTC_Format_BIN, &RTC_DateStruct);
	  RTC_GetTime(RTC_Format_BIN,&RTC_TimeStruct);
	
	  
	  copy_to_time(&RTC_DateStruct,&RTC_TimeStruct,&now);
	  min_length=60*60*24*7;
	
	  for(i=0;i<my_alarm_clock.point;i++){
			
	  length=seconds_between(now,my_alarm_clock.clock[i]);
		
		if(length!=0&&length<min_length){
			
		min_length=length;
		
		dst=my_alarm_clock.clock[i];
		}
		}
	  
		return dst;
}
void set_alarm_clock(struct time temp){
    RTC_DateTypeDef RTC_DateStruct;	
	  RTC_TimeTypeDef RTC_TimeStruct;
	  int week=temp.week;
   	struct time now;
	 
	  
	  RTC_GetDate(RTC_Format_BIN, &RTC_DateStruct);
	  RTC_GetTime(RTC_Format_BIN,&RTC_TimeStruct);
	
	 
	  copy_to_time(&RTC_DateStruct,&RTC_TimeStruct,&now); 
	  if(week==0){
		  if(now.hour>temp.hour||(now.hour==temp.hour&&now.minute>temp.minute)){
			week=now.week+1;
			}
			else{
			week=now.week;
			}
		} 
	  RTC_Set_AlarmA(week,temp.hour,temp.minute,0);
	  
}
int allow_adding(void){//�Ƿ��������
	
return my_alarm_clock.point<my_alarm_clock.maxsize;
}
int not_exist(struct time temp){//�������������
	int i;
  for(i=0;i<my_alarm_clock.point;i++){
	 if(temp.week==my_alarm_clock.clock[i].week&&temp.hour==my_alarm_clock.clock[i].hour&&temp.minute==my_alarm_clock.clock[i].minute){
	   return 0;
	 }
  }
	return 1;
}
void add_alarm_clock(struct time temp){//���һ������
	//set_alarm_clock(temp);
	//my_alarm_clock.maxsize=10;
if(allow_adding()&&not_exist(temp)){
my_alarm_clock.clock[my_alarm_clock.point++]=temp;
if(my_alarm_clock.point==1)
	set_alarm_clock(temp);
else{
  set_alarm_clock(get_next_alarm_clock());
}
}/**/
}
void drop_alarm_clock(int i){
for(;i<my_alarm_clock.point-1;i++){
my_alarm_clock.clock[i]=my_alarm_clock.clock[i+1];
}
my_alarm_clock.point--;

if(my_alarm_clock.point==0)
	RTC_AlarmCmd(RTC_Alarm_A,DISABLE);
else
  set_alarm_clock(get_next_alarm_clock());
}












