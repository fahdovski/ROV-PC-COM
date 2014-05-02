/**
  ******************************************************************************
  * @file    main.c
  * @author  MCD Application Team
  * @version V4.0.0
  * @date    21-January-2013
  * @brief   Virtual Com Port Demo main file
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT 2013 STMicroelectronics</center></h2>
  *
  * Licensed under MCD-ST Liberty SW License Agreement V2, (the "License");
  * You may not use this file except in compliance with the License.
  * You may obtain a copy of the License at:
  *
  *        http://www.st.com/software_license_agreement_liberty_v2
  *
  * Unless required by applicable law or agreed to in writing, software 
  * distributed under the License is distributed on an "AS IS" BASIS, 
  * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  * See the License for the specific language governing permissions and
  * limitations under the License.
  *
  ******************************************************************************
  */


/* Includes ------------------------------------------------------------------*/
#include "hw_config.h"
#include "usb_lib.h"
#include "usb_desc.h"
#include "usb_pwr.h"
#include "usart.h"
#include "stdio.h"
#include "rov_desc.h"
#include "crc16.h"
    
struct JOYSTICK ROV_joy ;    
int check_range(void);
void send_ack()   ;
void send_nack()   ;  
void store_joypacket()  ;
void print_joystick();  
void debug_trame();
    
uint8_t Dpacket[13];
uint16_t Rov_state=0;

extern __IO uint8_t Receive_Buffer[64];
extern __IO  uint32_t Receive_length ;
extern __IO  uint32_t length ;
uint8_t Send_Buffer[64];
uint32_t packet_sent=1;
uint32_t packet_receive=1;

uint16_t crc_remote,crc;

static __IO uint32_t TimingDelay;
void Delay(__IO uint32_t nTime);
void Led_init(void);

/*******************************************************************************
* Function Name  : main.
* Descriptioan    : Main routine.
* Input          : None.
* Output         : None.
* Return         : None.
*******************************************************************************/
int main(void)
{
  Set_System();
  Set_USBClock();
  USB_Interrupts_Config();
  USB_Init();
  init_USART1();
  Led_init();
  //Configure Systick timing
 if (SysTick_Config(SystemCoreClock / 1000))
  { 
    /* Capture error */ 
    STM_EVAL_LEDOn(LED10);
    while (1);
  }
  //Convert float to string
   char str[30];
   /* float flt = 2.4567F;
    sprintf(str, "%.4f", flt );   */
  
  
   STM_EVAL_LEDOn(LED3);  
     
  while (1)
  {
    if (bDeviceState == CONFIGURED)
    {
      STM_EVAL_LEDOn(LED4);  
       /*OUT transfers (receive the data from the PC to STM32):
	When a packet is received from the PC on the OUT pipe (EP3), EP3_IN_Callback will process the receive data.
	To receive something to the STM32 data are stored in the Receive_Buffer[] by calling CDC_Receive_DATA() */
      CDC_Receive_DATA();
      
      //Check to see if we have data yet 
      if (Receive_length  != 0)
      {        
        if( Receive_Buffer[0]==0xFC &&  Receive_length< 14)// CMD Packet
        {
           for(int i=0;i< Receive_length ;i++)
              Dpacket[i] =  Receive_Buffer[i]; //copy tab to static var to avoid compiler warning
            
           uint8_t error_packet =check_crc(Dpacket,Receive_length);
           
                if( error_packet ==0 )
                {         
                   if (packet_sent == 1)
                        send_ack();
                   
                   sprintf(str,"CRC1: %d ", error_packet);
                   USART_puts( USART1, str);
                  //CRC is OK send ACK to PC
                   
                }
               else
                {
                    if (packet_sent == 1)
                        send_nack();    
                
                   sprintf(str,"CRC2: %d ", error_packet);
                   USART_puts( USART1, str);
              //CRC is NOK send NACK to PC
                         
                }
            
                if( Dpacket[1] == 1) //Init ROV
                {
                Rov_state = Dpacket[3] << 8 | Dpacket[4]; 
                
               // sprintf(str,"State: %x ", Rov_state);
               // USART_puts( USART1, str);
                }
                else if(Rov_state == 0xFAFE && Dpacket[1] >= 45 && Dpacket[1] <= 50) // Joystick Packet
                {
                 store_joypacket();      
                 print_joystick(); //Display only
                }
                else if   (Dpacket[1] == 2) //Debug test
                {
                  debug_trame();
                }
       }
     }
     else
     {
        STM_EVAL_LEDToggle(LED4);
     }
       
     //reset packet
       for(int i=0;i< 13 ;i++) 
              Dpacket[i] = 0; 
           
        Receive_length = 0;
  }
 
      
      /* Toggle LED3 */
   //  STM_EVAL_LEDToggle(LED3);  
   //  Delay(500);
   
    
 }//End while(1)
}

void debug_trame()
{
   char strx[50];
  USART_puts( USART1, "Debug: ");
  for(int i=0;i<13;i++)
  {
   sprintf(strx,"%d, ",Dpacket[i]);
   USART_puts( USART1, strx);
  }
  USART_puts( USART1, "\n");
}

int check_range(void)
{
  return 1;
}

void send_ack()
{     
  uint8_t Tx_buffer[3];
  Tx_buffer[0]=  0xFB;
  Tx_buffer[1]= (uint8_t)(60000 >> 8);
  Tx_buffer[2]= (uint8_t)(60000 & 0x00FF);
    
  CDC_Send_DATA (Tx_buffer,3);
}

void send_nack()
{
  uint8_t Tx_buffer[3];
  Tx_buffer[0]=  0xFB;
  Tx_buffer[1]= (uint8_t)(60001 >> 8);
  Tx_buffer[2]= (uint8_t)(60001 & 0x00FF);
    
  CDC_Send_DATA (Tx_buffer,3);
}

void  store_joypacket()
{
 
          if( Dpacket[1]== 45) //X axis -1000 ,1000 range
          ROV_joy.X= Dpacket[3] << 8 | Dpacket[4];
          
          if( Dpacket[1]== 46) //Y axis
          ROV_joy.Y= Dpacket[3] << 8 | Dpacket[4];
          
          if( Dpacket[1]== 47) //RZ axis
          ROV_joy.Rz= Dpacket[3] << 8 | Dpacket[4];
          
          if( Dpacket[1]== 48) //Slider
          ROV_joy.slider= Dpacket[3] << 8 | Dpacket[4];
          
          if( Dpacket[1]== 49) //POV
          ROV_joy.pov=  Dpacket[3] << 8 | Dpacket[4];
          
          if( Dpacket[1]== 50) //Buttons
          {
            int k=0;
            for(int i=3;i<Dpacket[2]+3;i+=2) 
            {
              ROV_joy.button[k]= Dpacket[i]  ;
              k++;
            }
          }
          

}

void print_joystick()
{
  char strx[50];
   USART_puts( USART1, "JOYSTICK: ");
   sprintf(strx,"X:%d Y:%d Rz:%d\t",ROV_joy.X,ROV_joy.Y,ROV_joy.Rz);
 //  sprintf(strx,"X:%d Y:%d Rz:%d Slider:%d Pov:%d Buttons:%d %d %d %d %d %d \n",ROV_joy.X,ROV_joy.Y,ROV_joy.Rz,ROV_joy.slider, ROV_joy.pov,ROV_joy.button[0],ROV_joy.button[1],ROV_joy.button[2],ROV_joy.button[3],ROV_joy.button[4],ROV_joy.button[5]);
   USART_puts( USART1, strx);
   USART_puts( USART1, "\t");
}

/**
  * @brief  Inserts a delay time.
  * @param  nTime: specifies the delay time length, in milliseconds.
  * @retval None
  */
void Delay(__IO uint32_t nTime)
{ 
  TimingDelay = nTime;

  while(TimingDelay != 0);
}

/**
  * @brief  Decrements the TimingDelay variable.
  * @param  None
  * @retval None
  */
void TimingDelay_Decrement(void)
{
  if (TimingDelay != 0x00)
  { 
    TimingDelay--;
  }
}

void Led_init(void)
{
   /* Initialize Leds mounted on STM32F3-discovery */
  STM_EVAL_LEDInit(LED3);
  STM_EVAL_LEDInit(LED4);
  STM_EVAL_LEDInit(LED5);
  STM_EVAL_LEDInit(LED6);
  STM_EVAL_LEDInit(LED7);
  STM_EVAL_LEDInit(LED8);
  STM_EVAL_LEDInit(LED9);
  STM_EVAL_LEDInit(LED10);
  

  STM_EVAL_LEDOn(LED3);
  STM_EVAL_LEDOff(LED4);
  STM_EVAL_LEDOff(LED5);
  STM_EVAL_LEDOff(LED6);
  STM_EVAL_LEDOff(LED7);
  STM_EVAL_LEDOff(LED8);
  STM_EVAL_LEDOff(LED9);
  STM_EVAL_LEDOff(LED10);
  

}

#ifdef USE_FULL_ASSERT
/*******************************************************************************
* Function Name  : assert_failed
* Description    : Reports the name of the source file and the source line number
*                  where the assert_param error has occurred.
* Input          : - file: pointer to the source file name
*                  - line: assert_param error line source number
* Output         : None
* Return         : None
*******************************************************************************/
void assert_failed(uint8_t* file, uint32_t line)
{
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

  /* Infinite loop */
  while (1)
  {}
}
#endif

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
