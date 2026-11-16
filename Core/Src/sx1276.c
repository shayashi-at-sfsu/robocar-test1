#include "sx1276.h"
extern SPI_HandleTypeDef hspi2;
#define CS_PORT GPIOB
#define CS_PIN GPIO_PIN_12
#define RST_PORT GPIOB
#define RST_PIN GPIO_PIN_11
#define FIFO 0x00
#define OPMODE 0x01
#define IRQ 0x12
#define IRQ_RXDONE 0x40
#define IRQ_CRC 0x20
#define IRQ_TXDONE 0x08
#define LORA 0x80
#define STDBY 0x01
#define TX 0x03
#define RXCONT 0x05
static void select_radio(void){HAL_GPIO_WritePin(CS_PORT,CS_PIN,GPIO_PIN_RESET);}
static void deselect_radio(void){HAL_GPIO_WritePin(CS_PORT,CS_PIN,GPIO_PIN_SET);}
static uint8_t rd(uint8_t reg){uint8_t tx[2]={reg&0x7f,0},rx[2]={0};select_radio();HAL_SPI_TransmitReceive(&hspi2,tx,rx,2,100);deselect_radio();return rx[1];}
static void wr(uint8_t reg,uint8_t val){uint8_t b[2]={reg|0x80,val};select_radio();HAL_SPI_Transmit(&hspi2,b,2,100);deselect_radio();}
static void burst_wr(const uint8_t *data,uint8_t n){uint8_t a=0x80|FIFO;select_radio();HAL_SPI_Transmit(&hspi2,&a,1,100);HAL_SPI_Transmit(&hspi2,(uint8_t*)data,n,100);deselect_radio();}
static void burst_rd(uint8_t *data,uint8_t n){uint8_t a=FIFO;select_radio();HAL_SPI_Transmit(&hspi2,&a,1,100);HAL_SPI_Receive(&hspi2,data,n,100);deselect_radio();}
int SX1276_Init(void){
 deselect_radio();HAL_GPIO_WritePin(RST_PORT,RST_PIN,GPIO_PIN_RESET);HAL_Delay(10);HAL_GPIO_WritePin(RST_PORT,RST_PIN,GPIO_PIN_SET);HAL_Delay(20);
 if(rd(0x42)!=0x12)return -1;
 wr(OPMODE,LORA);HAL_Delay(10);wr(OPMODE,LORA|STDBY);
 wr(0x06,0xE4);wr(0x07,0xC0);wr(0x08,0x00); /* 915 MHz */
 wr(0x09,0x8F); /* preserve known-good PA setting; validate RF power */
 wr(0x0C,rd(0x0C)|0x03);
 wr(0x1D,0x93); /* BW500, CR4/5, implicit header */
 wr(0x1E,0x64); /* SF6, CRC enabled */
 wr(0x26,0x04); /* AGC */
 wr(0x31,(rd(0x31)&0xF8)|0x05);wr(0x37,0x0C); /* SF6 detection */
 wr(0x20,0);wr(0x21,8);wr(0x39,0x12);
 wr(0x0E,0);wr(0x0F,0);wr(0x22,SX1276_PACKET_SIZE);
 wr(IRQ,0xFF);wr(OPMODE,LORA|STDBY);HAL_Delay(10);return 0;
}
int SX1276_SendPacket(const uint8_t *data,uint8_t len,uint32_t timeout_ms){
 if(!data||len==0||len>255)return -1;
 wr(OPMODE,LORA|STDBY);wr(IRQ,0xFF);wr(0x0D,0);burst_wr(data,len);wr(0x22,len);wr(OPMODE,LORA|TX);
 uint32_t start=HAL_GetTick();
 while(!(rd(IRQ)&IRQ_TXDONE)){
  if(HAL_GetTick()-start>=timeout_ms){wr(OPMODE,LORA|STDBY);wr(IRQ,0xFF);return -2;}
 }
 wr(IRQ,IRQ_TXDONE);wr(OPMODE,LORA|STDBY);return len;
}
void SX1276_StartRx(void){wr(OPMODE,LORA|STDBY);wr(0x22,SX1276_PACKET_SIZE);wr(0x0D,0);wr(IRQ,0xFF);wr(OPMODE,LORA|RXCONT);}
int SX1276_PollPacket(uint8_t *buffer,uint8_t capacity){
 uint8_t flags=rd(IRQ);if(!(flags&IRQ_RXDONE))return 0;
 if(flags&IRQ_CRC){wr(IRQ,0xFF);return -1;}
 uint8_t n=rd(0x13);if(n!=SX1276_PACKET_SIZE||n>capacity){wr(IRQ,0xFF);return -2;}
 wr(0x0D,rd(0x10));burst_rd(buffer,n);wr(IRQ,0xFF);return n;
}
