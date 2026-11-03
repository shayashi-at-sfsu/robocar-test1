#include "rc_lora.h"
#include "sx1276.h"
#include "motor.h"
#include "main.h"
#include <string.h>

#define RC_MAGIC0 0x52U
#define RC_MAGIC1 0x43U
#define RC_VERSION 1U
#define RC_TX_TIMEOUT_MS 100U
#define RC_MAX_DUTY 40 /* wheels-up test limit */

static uint8_t tx[12], rx[12];
static uint16_t seq;
static uint16_t last_seq;
static uint8_t seen_seq;
static uint32_t last_tx_ms, last_valid_ms;
static uint8_t link_active;
static int8_t desired_throttle, desired_steering;

static uint8_t crc8(const uint8_t *p, uint8_t n)
{
    uint8_t c = 0;
    for (uint8_t i=0;i<n;i++) {
        c ^= p[i];
        for (uint8_t b=0;b<8;b++) c = (c & 0x80U) ? (uint8_t)((c<<1)^0x07U) : (uint8_t)(c<<1);
    }
    return c;
}
static int clamp(int v,int lo,int hi) {return v<lo?lo:(v>hi?hi:v);}
static void apply_motor(int throttle,int steering)
{
    int left=clamp(throttle+steering,-RC_MAX_DUTY,RC_MAX_DUTY);
    int right=clamp(throttle-steering,-RC_MAX_DUTY,RC_MAX_DUTY);
    /* Initial version supports symmetric forward/back/turn commands only.
       Reject mixed differential values until a signed dual-motor API is added. */
    if(left==0 && right==0) Motor_Stop();
    else if(left>0 && right>0) Motor_Forward((uint8_t)((left<right)?left:right));
    else if(left<0 && right<0) Motor_Backward((uint8_t)((-left<-right)?-left:-right));
    else if(left<0 && right>0) Motor_Left((uint8_t)((-left<right)?-left:right));
    else if(left>0 && right<0) Motor_Right((uint8_t)((left<-right)?left:-right));
    else Motor_Stop(); /* fail-safe on unsupported differential combinations */
}
void RC_SetCommand(int8_t throttle,int8_t steering)
{
    desired_throttle=(int8_t)clamp(throttle,-RC_MAX_DUTY,RC_MAX_DUTY);
    desired_steering=(int8_t)clamp(steering,-RC_MAX_DUTY,RC_MAX_DUTY);
}
void RC_LoRa_Init(void)
{
    if(SX1276_InitPingPong()!=0) Error_Handler();
    seq=0;seen_seq=0;link_active=0;
    last_tx_ms=HAL_GetTick();last_valid_ms=HAL_GetTick();
#if RC_CONTROLLER
    RC_SetCommand(0,0); /* always boot STOP */
#else
    Motor_Stop();
    SX1276_StartRx();
#endif
}
void RC_LoRa_Process(void)
{
    uint32_t now=HAL_GetTick();
#if RC_CONTROLLER
    if((uint32_t)(now-last_tx_ms)>=RC_SEND_PERIOD_MS) {
        last_tx_ms=now;
        memset(tx,0,sizeof(tx));
        tx[0]=RC_MAGIC0;tx[1]=RC_MAGIC1;tx[2]=RC_VERSION;
        tx[3]=(uint8_t)(seq>>8);tx[4]=(uint8_t)seq;
        tx[5]=(uint8_t)desired_throttle;tx[6]=(uint8_t)desired_steering;
        tx[7]=0; /* reserved flags */
        tx[11]=crc8(tx,11);
        (void)SX1276_SendPacket(tx,sizeof(tx),RC_TX_TIMEOUT_MS);
        seq++;
    }
#else
    int n=SX1276_PollPacket(rx,sizeof(rx));
    if(n==12 && rx[0]==RC_MAGIC0 && rx[1]==RC_MAGIC1 &&
       rx[2]==RC_VERSION && rx[11]==crc8(rx,11)) {
        uint16_t s=(uint16_t)(((uint16_t)rx[3]<<8)|rx[4]);
        int throttle=(int8_t)rx[5], steering=(int8_t)rx[6];
        if(throttle>=-RC_MAX_DUTY && throttle<=RC_MAX_DUTY &&
           steering>=-RC_MAX_DUTY && steering<=RC_MAX_DUTY &&
           (!seen_seq || (int16_t)(s-last_seq)>0)) {
            seen_seq=1;last_seq=s;
            last_valid_ms=HAL_GetTick();link_active=1;
            apply_motor(throttle,steering);
        }
    }
    if(link_active && (uint32_t)(HAL_GetTick()-last_valid_ms)>RC_FAILSAFE_MS) {
        Motor_Stop();link_active=0;
    }
#endif
}
