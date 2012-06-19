#include <msp430.h>
#include <string.h>
#include "enc28j60.h"
/* Duncan Frost - 17/06/2012 */
/* Tests ENC28J60 driver and ARP header
The enc28j60 is tested by sending an ARP request and reciveing a reply.

An ARP request will resolve an IP address to its corresponding MAC address. For
this test the MAC that will be resolved is for the router.

If sent correctly a reply will be recieved that contains the resolved MAC. After
resolving the IP the device will stop.
*/

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

//ARP opCodes
#define ARPREPLY  0x0002
#define ARPREQUEST 0x0001
//ARP hardware types
#define ETHERNET 0x0001

//Host order to device order.
#define HTONS(x) ((x<<8)|(x>>8))

// MAC address of the enc28j60
const unsigned char deviceMAC[6] = {0x00,0xa0,0xc9,0x14,0xc8,0x00};
// Router MAC is not known at beginning and requires an ARP reply to know.
unsigned char routerMAC[6] = {0xff,0xff,0xff,0xff,0xff,0xff};
// IP address of the enc28j60
unsigned char deviceIP[4] = {192,168,0,153};
// IP address of the router
unsigned char routerIP[4] = {192,168,0,1};

void SendArpPacket(unsigned char* targetIP)
{
  ARP arpPacket;
  
  /*----Setup EtherNetII Header----*/
  //The source of the packet will be the device mac address.
  memcpy( arpPacket.eth.SrcAddrs, deviceMAC, sizeof(deviceMAC) );
  //The destination is broadcast ie all bits are 0xff.
  memset( arpPacket.eth.DestAddrs, 0xff, sizeof(deviceMAC) );
  //The type of packet being sent is an ARP
  arpPacket.eth.type = HTONS(ARPPACKET);
  
  /*----Setup ARP Header----*/
  arpPacket.hardware = HTONS(ETHERNET);
  //We are wanting IP address resolved.
  arpPacket.protocol = HTONS(IPPACKET);
  arpPacket.hardwareSize = sizeof(deviceMAC);
  arpPacket.protocolSize = sizeof(deviceIP);
  arpPacket.opCode = HTONS(ARPREQUEST);
  
  //Target MAC is set to 0 as it is unknown.
  memset( arpPacket.targetMAC, 0, sizeof(deviceMAC) );
  //Sender MAC is the device MAC address.
  memcpy( arpPacket.senderMAC, deviceMAC, sizeof(deviceMAC) );
  //The target IP is the IP address we want resolved.
  memcpy( arpPacket.targetIP, targetIP, sizeof(routerIP));
  
  //If we are just making sure this IP address is not in use fill in the 
  //sender IP address as 0. Otherwise use the device IP address.
  if ( !memcmp( targetIP, deviceIP, sizeof(deviceIP) ) )
  { 
    memset( arpPacket.senderIP, 0, sizeof(deviceIP) );
  }
  else
  {
    memcpy( arpPacket.senderIP, deviceIP, sizeof(deviceIP) );
  }
  
  //Send out the packet.
  MACWrite((unsigned char*)&arpPacket,sizeof(ARP));
}

int main( void )
{  
  // Stop watchdog timer to prevent time out reset.
  WDTCTL = WDTPW + WDTHOLD;
  //Initalise the ENC28J60 device.
  initMAC(deviceMAC);
  // Send an ARP at the router.
  SendArpPacket(routerIP);
  
  while(1)
  {
    ARP arpPacket;
    MACRead((unsigned char*)&arpPacket,sizeof(ARP));
    if ( !memcmp( arpPacket.senderIP, routerIP, sizeof(routerIP) ) )
    {
      //Successfully recieved reply
      //Copy in the routers MAC address.
      memcpy( routerMAC, arpPacket.senderMAC, sizeof(routerMAC) );
      break;
    }
  }
}
