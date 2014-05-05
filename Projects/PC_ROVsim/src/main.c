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
#include "stdbool.h"    
struct JOYSTICK ROV_joy ;    
struct ROV_STRM ROV_stream ;   
int check_range(void);
void send_ack()   ;
void send_nack()   ;  
void store_joypacket()  ;
void print_joystick();  
void debug_trame();

bool check_ack();

uint8_t Dpacket[13];
uint16_t Rov_state=0;

extern __IO uint8_t Receive_Buffer[64];
extern __IO  uint32_t Receive_length ;
extern __IO  uint32_t length ;
uint8_t Send_Buffer[64];
uint32_t packet_sent=1;
uint32_t packet_receive=1;

uint8_t Tx_buffer[13]; 
uint16_t crc_remote,crc;
uint16_t pack_counter=0;

static __IO uint32_t TimingDelay;
static __IO uint32_t TimingCounter=0;
void Delay(__IO uint32_t nTime);
void Led_init(void);
void TimeSending();
void Packet_CMD(uint8_t CMD_Header, uint8_t CMD_ID,uint8_t CMD_Size,uint8_t *CMD_Data);
void init_streaming();
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
  init_streaming();
  
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
                  
                   send_ack();
                   pack_counter++;
                   sprintf(str,"%d ",pack_counter);
                   USART_puts( USART1, str);
                  //CRC is OK send ACK to PC
                   
                }
               else
                {
                   send_nack();    
                
                   sprintf(str,"CRC Fail");
                   USART_puts( USART1, str);
              //CRC is NOK send NACK to PC
                         
                }
            
                if( Dpacket[1] == 1) //Init ROV
                {
                Rov_state = Dpacket[3] << 8 | Dpacket[4]; 
                
               // sprintf(str,"State: %x ", Rov_state);
               // USART_puts( USART1, str);
                }
                
                else if   (Dpacket[1] == 2) //Debug test
                {
                  debug_trame();
                }
                
                else if(Rov_state == 0xFAFE && Dpacket[1] >= 45 && Dpacket[1] <= 50) // Joystick Packet
                {
                 store_joypacket();      
                 print_joystick(); //Display only
                }     
       }
       else if ( Receive_Buffer[0]==0xFB &&  Receive_length == 3)// CMD Packet
        {
                         for(int i=0;i< Receive_length ;i++)
                              Dpacket[i] =  Receive_Buffer[i]; //copy tab to static var to avoid compiler warning
              
              if(!check_ack())
              {
                //ack failed
                 sprintf(str,"ACK Fail ");
                   USART_puts( USART1, str);
              }
              else
              {
                 sprintf(str,"ACK Success ");
                   USART_puts( USART1, str);
              }

        }
     }

          
      //if(  Rov_state == 0xFAFE ) //Send only if rov is initialized
     //   TimeSending();
        
      
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


bool check_ack()
{
    bool Tx_done=false;
    uint16_t  Rx_ID = (uint16_t)((Dpacket[1] << 8) | Dpacket[2]);
                if (Rx_ID == 60000)
                {
                    Tx_done = true; //ack ok                   
                }
                else if (Rx_ID == 60001)
                {
                    Tx_done = false; //ack nok
                }  
         return Tx_done;
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
  uint8_t local_buffer[3];
  local_buffer[0]=  0xFB;
  local_buffer[1]= (uint8_t)(60000 >> 8);
  local_buffer[2]= (uint8_t)(60000 & 0x00FF);
    
   if (packet_sent == 1)
  CDC_Send_DATA (local_buffer,3);
}

void send_nack()
{
  uint8_t local_buffer[3];
  local_buffer[0]=  0xFB;
  local_buffer[1]= (uint8_t)(60001 >> 8);
  local_buffer[2]= (uint8_t)(60001 & 0x00FF);
    
   if (packet_sent == 1)
  CDC_Send_DATA (local_buffer,3);
}


typedef union _data {
  float f;
  uint8_t  s[4];
} myData;
myData q;


void init_streaming()
{
  ROV_stream.roll = 0.50f;
  ROV_stream.pitch = 45.40f;
  ROV_stream.yaw = -180.0f;
  
  ROV_stream.depth = 12.46f;
  
  ROV_stream.current = 25.0f;
  ROV_stream.voltage = 230;
  
  for(int i=0;i<6;i++)
  ROV_stream.thruster[i]= 1000+(100*i);
  
}





 void Packet_CMD(uint8_t CMD_Header, uint8_t CMD_ID,uint8_t CMD_Size,uint8_t *CMD_Data)
        {
        
          //Header 1byte
           Tx_buffer[0]=CMD_Header;
          //ID 1byte
           Tx_buffer[1]=CMD_ID;
          //Length 1byte
           Tx_buffer[2]=CMD_Size;
          //Data (max 8 byte)
           for(int i=0;i<CMD_Size;i++)
           Tx_buffer[i+3]=CMD_Data[i] ;
          //Crc (2byte)
          uint16_t comp_crc= compute_crc(Tx_buffer, CMD_Size+3);
           Tx_buffer[CMD_Size]= (uint8_t)(comp_crc >> 8);
           Tx_buffer[CMD_Size+1]=(uint8_t)(comp_crc & 0x00FF);
             
            
          
        }

void sendstrm_float(float data ,uint8_t id)
{
  q.f=data;//convert float to  byte tab
  Packet_CMD(0xFE, id,4,q.s); //update Tx buffer
   if (packet_sent == 1)
  CDC_Send_DATA(Tx_buffer,4);
}


void sendstrm_uint16(uint16_t data ,uint8_t id)
{
  uint8_t spckt[2];
  spckt[0] =(uint8_t)(data >> 8);
  spckt[1] =(uint8_t)(data & 0x00FF);
  
  Packet_CMD(0xFE, id,2,spckt); //update Tx buffer
  
  
   if (packet_sent == 1)
  CDC_Send_DATA(Tx_buffer,2);
}






void  store_joypacket()
{
 
          if( Dpacket[1]== 45) //X axis 0 ,65535 range
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
   sprintf(strx,"X:%d t",ROV_joy.X);
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

void TimingCounter_Increment(void)
{
  if(TimingCounter <100)
  TimingCounter++;
  else
  TimingCounter=0;
}

void TimeSending(void) //send Mandatory streaming data to PC
{
  
  if (TimingCounter>=20) // send @50hz
  {
    sendstrm_float(ROV_stream.roll,80); 
    sendstrm_float(ROV_stream.pitch,81); 
    sendstrm_float(ROV_stream.yaw,82); 
   
    sendstrm_float(ROV_stream.current,102); 
    sendstrm_float(ROV_stream.voltage,103); 

  }
     
  else if (TimingCounter>=100) //send @10Hz
  {
     sendstrm_float(ROV_stream.depth,83); 
     
      for(int i=0;i<6;i++)
    sendstrm_uint16(ROV_stream.thruster[i] ,39+i);
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
