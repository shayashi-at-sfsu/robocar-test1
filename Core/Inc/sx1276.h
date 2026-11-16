#ifndef SX1276_H
#define SX1276_H

#include "main.h"
#include <stdint.h>

#define SX1276_PACKET_SIZE 12U

int SX1276_Init(void);
int SX1276_SendPacket(const uint8_t *data, uint8_t len, uint32_t timeout_ms);
void SX1276_StartRx(void);
int SX1276_PollPacket(uint8_t *buffer, uint8_t capacity); /* 0 no packet; -1 CRC; -2 size */

#endif
