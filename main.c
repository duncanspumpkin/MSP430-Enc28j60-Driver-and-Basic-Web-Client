#include <msp430.h>
#include "enc28j60.h"
#include <string.h>


// Switch to host order for the enc28j60
#define HTONS(x) ((x<<8)|(x>>8))

unsigned char uip_buf[250];
unsigned char uip_len;
unsigned char bytRouterMac[6];

// Mac address of the enc28j60
const unsigned char bytMacAddress[6] = {0x00,0xa0,0xc9,0x14,0xc8,0x00};
const unsigned char bytIPAddress[4] = {192,168,0,50};
const unsigned char routerIP[4] = {192,168,0,1};

void
add32(unsigned char *op32, unsigned int op16)
{
  op32[3] += (op16 & 0xff);
  op32[2] += (op16 >> 8);
  
  if(op32[2] < (op16 >> 8)) {
    ++op32[1];
    if(op32[1] == 0) {
      ++op32[0];
    }
  }
  
  
  if(op32[3] < (op16 & 0xff)) {
    ++op32[2];
    if(op32[2] == 0) {
      ++op32[1];
      if(op32[1] == 0) {
	++op32[0];
      }
    }
  }
}

static unsigned int
chksum(unsigned int sum, const unsigned char *data, unsigned int len)
{
  unsigned int t;
  const unsigned char *dataptr;
  const unsigned char *last_byte;

  dataptr = data;
  last_byte = data + len - 1;
  
  while(dataptr < last_byte) {	/* At least two more bytes */
    t = (dataptr[0] << 8) + dataptr[1];
    sum += t;
    if(sum < t) {
      sum++;		/* carry */
    }
    dataptr += 2;
  }
  
  if(dataptr == last_byte) {
    t = (dataptr[0] << 8) + 0;
    sum += t;
    if(sum < t) {
      sum++;		/* carry */
    }
  }

  /* Return sum in host byte order. */
  return sum;
}

#pragma pack(1)
typedef struct
{
  unsigned char DestAddrs[6];
  unsigned char SrcAddrs[6];
  unsigned int type;
}  EtherNetII;

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

typedef struct
{
  EtherNetII eth;
  unsigned char hdrlen : 4;
  unsigned char version : 4;
  unsigned char diffsf;
  unsigned int len;
  unsigned int ident;
  unsigned int fragmentOffset1: 5;
  unsigned int flags : 3;
  unsigned int fragmentOffset2 : 8;
  unsigned char ttl;
  unsigned char protocol;
  unsigned int chksum;
  unsigned char source[4];
  unsigned char dest[4];
}IPhdr;

typedef struct
{
  IPhdr ip;
  unsigned int sourcePort;
  unsigned int destPort;
  unsigned char seqNo[4];
  unsigned char ackNo[4];
  unsigned char reserverd : 4;
  unsigned char hdrLen : 4;
  unsigned char NS:1;
  unsigned char CWR:1;
  unsigned char ECE:1;
  unsigned char URG:1;
  unsigned char ACK:1;
  unsigned char PSH:1;
  unsigned char RST:1;
  unsigned char SYN:1;
  unsigned char FIN:1;
  unsigned int wndSize;
  unsigned int chksum;
  unsigned int urgentPointer;
}TCPhdr;

typedef struct
{
  IPhdr ip;
  unsigned char type;
  unsigned char code;
  unsigned int chksum;
  unsigned int iden;
  unsigned int seqNum;
}ICMPhdr;

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
  arpPacket.targetIP[3] = 1;
  memcpy(&uip_buf[0],&arpPacket,sizeof(ARP));
  uip_len = sizeof(ARP);
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
  uip_len = sizeof(ARP);  
}

void ackTcp(TCPhdr* tcp)
{
  //Zero the checksums
  tcp->chksum = 0x0;
  tcp->ip.chksum = 0x0;
  // First sort out the destination mac and source
  memcpy(&tcp->ip.eth.DestAddrs[0],&tcp->ip.eth.SrcAddrs[0],6);
  memcpy(&tcp->ip.eth.SrcAddrs[0],&bytMacAddress[0],6);
  // Next flip the ips
  memcpy(&tcp->ip.dest[0],&tcp->ip.source[0],4);
  memcpy(&tcp->ip.source[0],&bytIPAddress[0],4);
  // Next flip ports
  tcp->destPort = tcp->sourcePort;
  tcp->sourcePort = HTONS(0x85ca);
  // Swap ack and seq
  char ack[4];
  memcpy(&ack,&tcp->ackNo[0],4);
  memcpy(&tcp->ackNo[0],&tcp->seqNo[0],4);
  memcpy(&tcp->seqNo[0],&ack[0],4);
  
  if( tcp->SYN )
  {
    add32(&tcp->ackNo[0],1);
  }
  else
  {
    add32(&tcp->ackNo[0],uip_len-sizeof(TCPhdr));
  }
  uip_len = sizeof(TCPhdr);
  tcp->chksum = HTONS(~(chksum(0,&uip_buf[sizeof(IPhdr)],sizeof(TCPhdr)-sizeof(IPhdr))));
  tcp->ip.len = HTONS(sizeof(TCPhdr)-sizeof(EtherNetII));
  tcp->ip.chksum = HTONS(~(chksum(0,&uip_buf[sizeof(EtherNetII)],sizeof(IPhdr)-sizeof(EtherNetII))));
}

int main( void )
{
  // Stop watchdog timer to prevent time out reset
  WDTCTL = WDTPW + WDTHOLD;
  P1DIR = 0x01;
  P1OUT = 0x0;
  initMAC();
  // Need a long delay to ensure everything is setup.
  // Infact could wait for others to send first packet.
  while( !MACRead() );
  
  // Declare our presense to the network
  PrepArp();
  MACWrite();
  while(1) {
    if( MACRead() )
    {
      EtherNetII* eth = (EtherNetII*)&uip_buf;
      if( eth->type == HTONS(0x0806))
      {
        ARP* arp = (ARP*)&uip_buf[0];
        if ( arp->targetIP[3] == 50 && arp->opCode == HTONS(0x0001))
        {
          memcpy(&bytRouterMac[0], &arp->senderMAC[0], 6);
          ReplyArp(*arp);
          MACWrite();
          
          ICMPhdr* ping = (ICMPhdr*)&uip_buf[0];
          uip_len = sizeof(ICMPhdr);
          memcpy(&ping->ip.eth.DestAddrs[0],&bytRouterMac[0],6);
          memcpy(&ping->ip.eth.SrcAddrs[0],&bytMacAddress[0],6);
          ping->ip.eth.type = HTONS(0x0800);
          ping->ip.version = 0x4;
          ping->ip.hdrlen = 0x5;
          ping->ip.diffsf = 0;
          ping->ip.ident = 2;
          ping->ip.flags = 0x2;
          ping->ip.fragmentOffset1 = 0x0;
          ping->ip.fragmentOffset2 = 0x0;
          ping->ip.ttl = 128;
          ping->ip.protocol = 0x01;
          ping->ip.chksum = 0x0;
          memcpy(&ping->ip.source[0],&bytIPAddress[0],4);
          memcpy(&ping->ip.dest[0],&routerIP[0],4);
          ping->type = 0x8;
          ping->code = 0x0;
          ping->chksum = 0x0;
          ping->iden = HTONS(0x1);
          ping->seqNum = HTONS(0x2);

          ping->chksum = HTONS(~(chksum(0,&uip_buf[sizeof(IPhdr)],60-sizeof(IPhdr))));
          ping->ip.len = HTONS(sizeof(ICMPhdr)-sizeof(EtherNetII));
          ping->ip.chksum = HTONS(~(chksum(0,&uip_buf[sizeof(EtherNetII)],sizeof(IPhdr)-sizeof(EtherNetII))));
          
          MACWrite();
        }
      }
      else if( eth->type == HTONS(0x0800) )
      {
        IPhdr* ip = (IPhdr*)&uip_buf[0];
        if( ip->protocol == 0x01 )
        {
          ICMPhdr* ping = (ICMPhdr*)&uip_buf[0];
          if ( ping->type == 0x8 )
          {
            ping->type = 0x0;
            ping->chksum = 0x0;
            ping->ip.chksum = 0x0;
            memcpy(&ping->ip.eth.DestAddrs[0],&ping->ip.eth.SrcAddrs[0],6);
            memcpy(&ping->ip.eth.SrcAddrs[0],&bytMacAddress[0],6);
            memcpy(&ping->ip.dest[0],&ping->ip.source[0],4);
            memcpy(&ping->ip.source[0],&bytIPAddress[0],4);
            
            ping->chksum = HTONS(~(chksum(0,&uip_buf[sizeof(IPhdr)],uip_len-sizeof(IPhdr))));;
            ping->ip.chksum = HTONS(~(chksum(0,&uip_buf[sizeof(EtherNetII)],sizeof(IPhdr)-sizeof(EtherNetII))));
            
            MACWrite();
          }
        }
        else if( ip -> protocol == 0x6 ) //TCP
        {
          ackTcp((TCPhdr*)&uip_buf[0]);
        }
      }
    }
   
  }
}
