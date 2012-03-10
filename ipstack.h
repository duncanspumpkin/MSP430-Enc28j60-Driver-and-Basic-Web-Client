#ifndef IPSTACK_H
#define IPSTACK_H

int IPstackInit( unsigned char const* MacAddress);
int IPstackHTMLPost( char* url, char* data);
int IPstackIdle();

#pragma pack(1)
typedef struct
{
  unsigned char DestAddrs[6];
  unsigned char SrcAddrs[6];
  unsigned int type;
}  EtherNetII;
// Ethernet packet types
#define ARPPACKET 0x0806
#define IPPACKET 0x0800

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

#define ARPREPLY  0x0002
#define ARPREQUEST 0x0001
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
// IP protocols
#define ICMPPROTOCOL 0x1
#define UDPPROTOCOL 0x11
#define TCPPROTOCOL 0x6

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

#define ICMPREPLY 0x0
#define ICMPREQUEST 0x8

#endif