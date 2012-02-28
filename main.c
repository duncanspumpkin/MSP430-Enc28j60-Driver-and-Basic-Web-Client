#include <msp430.h>
#include "enc28j60.h"
#include <string.h>

// Switch to host order for the enc28j60
#define HTONS(x) ((x<<8)|(x>>8))

unsigned char uip_buf[250];
unsigned char uip_len;
// Mac address of the enc28j60
const unsigned char bytMacAddress[6] = {0x00,0xa0,0xc9,0x14,0xc8,0x00};

typedef struct
{
  unsigned char DestAddrs[6];
  unsigned char SrcAddrs[6];
  unsigned int type;
}EtherNetII;

typedef struct
{
  EtherNetII eth;
  unsigned int hardware;
  unsigned int protocol;
  unsigned char hardwareSize;
  unsigned char protocolSize;
  unsigned int opCode;
  unsigned char senderMAC[6];
  unsigned char senderIP[4];
  unsigned char targetMAC[6];
  unsigned char targetIP[4];
}ARP;

void PrepArp()
{
  ARP arpPacket;
  memcpy(&arpPacket.eth.SrcAddrs[0],&bytMacAddress[0],sizeof(bytMacAddress));
  for( int i = 0; i < 6; ++i)
  {
    arpPacket.eth.DestAddrs[i] = 0xff;
    arpPacket.targetMAC[i] = 0x00;
  }
  arpPacket.eth.type = HTONS(0x0806);
  arpPacket.hardware = HTONS(0x0001);
  arpPacket.protocol = HTONS(0x0800);
  arpPacket.hardwareSize = 0x06;
  arpPacket.protocolSize = 0x04;
  arpPacket.opCode = HTONS(0x0001);
  
  memcpy(&arpPacket.senderMAC[0],&bytMacAddress[0],sizeof(bytMacAddress));

  arpPacket.senderIP[0] = 192;
  arpPacket.senderIP[1] = 168;
  arpPacket.senderIP[2] = 0;
  arpPacket.senderIP[3] = 50;
  arpPacket.targetIP[0] = 192;
  arpPacket.targetIP[1] = 168;
  arpPacket.targetIP[2] = 0;
  arpPacket.targetIP[3] = 6;
  memcpy(&uip_buf[0],&arpPacket,sizeof(ARP));
  uip_len = sizeof(EtherNetII) + sizeof(ARP);
}

void ReplyArp(ARP senderArp)
{
  ARP arpPacket;
  memcpy(&arpPacket.eth.SrcAddrs[0],&bytMacAddress[0],sizeof(bytMacAddress));
  memcpy(&arpPacket.eth.DestAddrs[0],&senderArp.eth.SrcAddrs[0],sizeof(bytMacAddress));
  memcpy(&arpPacket.targetMAC[0],&senderArp.eth.SrcAddrs[0],sizeof(bytMacAddress));
  memcpy(&arpPacket.senderMAC[0],&bytMacAddress[0],sizeof(bytMacAddress));
  arpPacket.eth.type = HTONS(0x0806);
  arpPacket.hardware = HTONS(0x0001);
  arpPacket.protocol = HTONS(0x0800);
  arpPacket.hardwareSize = 0x06;
  arpPacket.protocolSize = 0x04;
  arpPacket.opCode = HTONS(0x0002);

  arpPacket.senderIP[0] = 192;
  arpPacket.senderIP[1] = 168;
  arpPacket.senderIP[2] = 0;
  arpPacket.senderIP[3] = 50;
  memcpy(&arpPacket.targetIP[0],&senderArp.senderIP[0],4);
  memcpy(&uip_buf[0],&arpPacket,sizeof(ARP));
  uip_len = sizeof(EtherNetII) + sizeof(ARP);  
}
int main( void )
{
  // Stop watchdog timer to prevent time out reset
  WDTCTL = WDTPW + WDTHOLD;

  P1DIR = 0x01;
  P1OUT = 0x0;
  initMAC();
  __delay_cycles(16000);
  
  PrepArp();
  MACWrite();
  while(1) {
    //MACWrite();
    if( MACRead() )
    {
      EtherNetII* eth = (EtherNetII*)&uip_buf;
      if( eth->type == HTONS(0x0806))
      {
        ARP* arp = (ARP*)&uip_buf[0];
        if ( arp->eth.DestAddrs[0] == 0xff )
        {
          ReplyArp(*arp);
          MACWrite();
        }
        P1OUT = 0x1;
      }
    }
   
  }
 }
