#ifndef __CRC16_H
#define __CRC16_H

#include "stdint.h"
extern uint16_t crc_remote,crc;
extern const unsigned short crc_table[256];
int check_crc(volatile uint8_t *buff, uint8_t length);
unsigned short crc16(unsigned short crcval, unsigned char newchar);
uint16_t compute_crc(volatile uint8_t  *buff, uint8_t length);
#endif