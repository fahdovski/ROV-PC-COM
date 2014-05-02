#ifndef __ROV_DESC_H
#define __ROV_DESC_H

struct JOYSTICK {   
uint16_t X,Y,Rz,slider,pov; //-1000,1000
uint8_t button[6] ;
} ;  

  struct ROV_STRM{
float roll,pitch,yaw;
float depth;
float voltage;
float current;
uint16_t thruster[6];
};



#endif