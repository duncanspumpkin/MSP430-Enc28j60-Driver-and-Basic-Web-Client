#include "io430.h"
#include "enc28j60.h"
#include <string.h>

#define SEL_MAC(x)  (x==1) ? (P2OUT&=(~BIT0)) : (P2OUT|=BIT0)  

unsigned char uip_len;
unsigned char uip_buf[250];
const unsigned char bytMacAddress[6] = {0x00,0xa0,0xc9,0x14,0xc8,0x00};
typedef struct
{
  unsigned char DestAddrs[6];
  unsigned char SrcAddrs[6];
  unsigned char type[2];
}EtherNetII;

typedef struct
{
  unsigned char hardware[2];
  unsigned char  protocol[2];
  unsigned char hardwareSize;
  unsigned char protocolSize;
  unsigned char  opCode[2]; //Just do a <<
  unsigned char senderMAC[6];
  unsigned char senderIP[4];
  unsigned char targetMAC[6];
  unsigned char targetIP[4];
}ARP;

void PrepArp()
{
  EtherNetII ethPacket;
  ARP arpPacket;
  memcpy(&ethPacket.SrcAddrs[0],&bytMacAddress[0],sizeof(bytMacAddress));
  for( int i = 0; i < 6; ++i)
  {
    ethPacket.DestAddrs[i] = 0xff;
    arpPacket.targetMAC[i] = 0x00;
  }
  ethPacket.type[0] = 0x08;
  ethPacket.type[1] = 0x06;
  arpPacket.hardware[0] = 0x00;
  arpPacket.hardware[1] = 0x01;
  arpPacket.protocol[0] = 0x08;
  arpPacket.protocol[1] = 0x00;
  arpPacket.hardwareSize = 0x06;
  arpPacket.protocolSize = 0x04;
  arpPacket.opCode[0] = 0x00;
  arpPacket.opCode[1] = 0x01;
  memcpy(&arpPacket.senderMAC[0],&bytMacAddress[0],sizeof(bytMacAddress));
//  for(int i = 0; i < 4; ++i)
//  {
//    arpPacket.senderIP[i] = 0x00;
//  }
  arpPacket.senderIP[0] = 192;
  arpPacket.senderIP[1] = 168;
  arpPacket.senderIP[2] = 0;
  arpPacket.senderIP[3] = 50;
  arpPacket.targetIP[0] = 192;
  arpPacket.targetIP[1] = 168;
  arpPacket.targetIP[2] = 0;
  arpPacket.targetIP[3] = 1;
  memcpy(&uip_buf[0],&ethPacket,sizeof(EtherNetII));
  memcpy(&uip_buf[sizeof(EtherNetII)],&arpPacket,sizeof(ARP));
  uip_len = sizeof(EtherNetII) + sizeof(ARP);
}
int main( void )
{
  // Stop watchdog timer to prevent time out reset
  WDTCTL = WDTPW + WDTHOLD;
  DCOCTL = CALDCO_16MHZ;
  BCSCTL1 = CALBC1_16MHZ;
  P1DIR = 0x01;
  P1OUT = 0x0;
  initMAC();
  PrepArp();
  
  while(1) {
    MACWrite();
    if( MACRead() )
    {
      EtherNetII* eth = (EtherNetII*)&uip_buf[0];
      if( eth->type[0] == 0x08 && eth->type[1] == 0x06 && eth->DestAddrs[0]!= 0xff)
      {
      P1OUT = 0x1;
      while(1);
      }
    }
    PrepArp();
//    
//    ARP* arp = (ARP*)&uip_buf[sizeof(EtherNetII)];
//    
//    if( eth->type[0] == 0x08 && eth->type[1] == 0x06 && eth->DestAddrs[0]!= 0xff)
//    {
//      P1OUT = 0x1;
////      EtherNetII eth2;
////      ARP arp2;
////      memcpy(&(eth->DestAddrs[0]),&eth2.DestAddrs[0],6);
////      memcpy(&(eth->DestAddrs[0]),&arp2.targetMAC[0],6);
////      memcpy(&(arp->senderIP[0]),&arp2.targetIP[0],4);
////      
////      memcpy(&eth2.SrcAddrs[0],&bytMacAddress[0],sizeof(bytMacAddress));
////      eth2.type[0] = 0x08;
////      eth2.type[1] = 0x06;
//      
//      PrepArp();
//    }
  }
  //return 0;

  
//  initSPI();  
//  unsigned char opcode = 0xff;
//  SEL_MAC(1);
//  SPIWrite(&opcode,1);
//  SEL_MAC(0);
////   temp = ReadETHReg(ECON1);       // Read ECON1 0x1f register
////  temp &= ~ECON1_BSEL;            // mask off the BSEL 3 bits
////  temp |= bank;                   // set BSEL bits
////  WriteCtrReg(ECON1, temp);       // write new values back to ENC28J60
////   bytAddress |= WCR_OP;       // Set opcode 0x40
////  SEL_MAC(TRUE);              // ENC CS low
////  SPIWrite(&bytAddress,1);    // Tx opcode and address
////  SPIWrite(&bytData,1);       // Tx data
////  SEL_MAC(FALSE);
//  unsigned char bytData;
//  unsigned char bytAddress = 0x1F;
//  SEL_MAC(1);                 // ENC CS low
//  SPIWrite(&bytAddress,1);       // write opcode
//  SPIRead(&bytData, 1);          // read value
//  SEL_MAC(0);
//  bytAddress = 0x1f | 0x40;
//  bytData &= ~3;
//  bytData |= 3;
//  SEL_MAC(1);                 // ENC CS low
//  SPIWrite(&bytAddress,1);       // write opcode
//  SPIWrite(&bytData, 1);         // write value
//  SEL_MAC(0);
//  
//  bytAddress = 0x1f;
//  SEL_MAC(1);                 // ENC CS low
//  SPIWrite(&bytAddress,1);       // write opcode
//  SPIRead(&bytData, 1);          // read value
//  SEL_MAC(0);
  
  //return 0;
}
