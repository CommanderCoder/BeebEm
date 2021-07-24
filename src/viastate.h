#ifndef VS_HEADER
#define VS_HEADER

typedef struct {
  unsigned char ora,orb;
  unsigned char ira,irb;
  unsigned char ddra,ddrb;
  unsigned char acr,pcr;
  unsigned char ifr,ier;
  int timer1c,timer2c; /* NOTE: Timers descrement at 2MHz and values are */
  int timer1l,timer2l; /*   fixed up on read/write - latches hold 1MHz values*/
  bool timer1hasshot; /* True if we have already caused an interrupt for one shot mode */
  bool timer2hasshot; /* True if we have already caused an interrupt for one shot mode */
  int timer1adjust; // Adjustment for 1.5 cycle counts, every other interrupt, it becomes 2 cycles instead of one
  int timer2adjust; // Adjustment for 1.5 cycle counts, every other interrupt, it becomes 2 cycles instead of one
  unsigned char sr;
  bool ca2;
  bool cb2;
} VIAState;

// 6522 Peripheral Control Register
const unsigned char PCR_CB2_CONTROL           = 0xe0;
const unsigned char PCR_CB1_INTERRUPT_CONTROL = 0x10;
const unsigned char PCR_CA2_CONTROL           = 0x0e;
const unsigned char PCR_CA1_INTERRUPT_CONTROL = 0x01;

// PCR CB2 control bits
const unsigned char PCR_CB2_OUTPUT_PULSE = 0xa0;
const unsigned char PCR_CB2_OUTPUT_LOW   = 0xc0;
const unsigned char PCR_CB2_OUTPUT_HIGH  = 0xe0;

// PCR CB1 interrupt control bit
const unsigned char PCB_CB1_POSITIVE_INT = 0x10;

// PCR CA2 control bits
const unsigned char PCR_CA2_OUTPUT_PULSE = 0x0a;
const unsigned char PCR_CA2_OUTPUT_LOW   = 0x0c;
const unsigned char PCR_CA2_OUTPUT_HIGH  = 0x0e;

// PCR CA1 interrupt control bit
const unsigned char PCB_CA1_POSITIVE_INT = 0x01;

#endif
