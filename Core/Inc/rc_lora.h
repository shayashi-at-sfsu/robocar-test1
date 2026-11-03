#ifndef RC_LORA_H
#define RC_LORA_H

#include <stdint.h>

#define RC_CONTROLLER 0 /* 1=controller, 0=car */
#define RC_SEND_PERIOD_MS 100U
#define RC_FAILSAFE_MS 400U

void RC_LoRa_Init(void);
void RC_LoRa_Process(void);
/* Controller-only: throttle and steering -100..100 */
void RC_SetCommand(int8_t throttle, int8_t steering);

#endif
