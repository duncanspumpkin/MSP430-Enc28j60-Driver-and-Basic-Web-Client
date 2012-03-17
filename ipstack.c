#include "ipstack.h"
#include "enc28j60.h"
#include <string.h>
// Switch to host order for the enc28j60
#define HTONS(x) ((x<<8)|(x>>8))
unsigned char uip_buf[250];
unsigned char uip_len;
unsigned char bytRouterMac[6] = {0xff,0xff,0xff,0xff,0xff,0xff};
unsigned char serverIP[4];

unsigned char bytIPAddress[4] = {192,168,0,50};
unsigned char routerIP[4] = {192,168,0,1};

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

//Change prep ARP so that it takes a ip address argument
void SendArp(unsigned char* targetIP)
{
  ARP arpPacket;
  memcpy(&arpPacket.eth.SrcAddrs[0],&bytMacAddress[0],sizeof(bytMacAddress));
  memset(&arpPacket.eth.DestAddrs[0],0xff,sizeof(bytMacAddress));
  memset(&arpPacket.targetMAC[0],0,sizeof(bytMacAddress));

  arpPacket.eth.type = HTONS(ARPPACKET);
  arpPacket.hardware = HTONS(0x0001);
  arpPacket.protocol = HTONS(0x0800);
  arpPacket.hardwareSize = 0x06;
  arpPacket.protocolSize = 0x04;
  arpPacket.opCode = HTONS(ARPREQUEST);
  
  memcpy(&arpPacket.senderMAC[0],&bytMacAddress[0],sizeof(bytMacAddress));
  if ( !memcmp( &targetIP[0], &bytIPAddress[0], sizeof(bytIPAddress) ) )
  { //If we are checking this IP then we want senderIP = 0
    memset(&arpPacket.senderIP[0],0,sizeof(arpPacket.senderIP));
  }
  else
  {
    memcpy(&arpPacket.senderIP[0],&bytIPAddress[0],sizeof(bytIPAddress));
  }
  memcpy(&arpPacket.targetIP[0],&targetIP[0],sizeof(routerIP));
  memcpy(&uip_buf[0],&arpPacket,sizeof(ARP));
  uip_len = sizeof(ARP);
  
  MACWrite();
}

void ReplyArp()
{
  
  ARP* arpPacket = (ARP*)&uip_buf[0];
  //To reply we want to make sure the IP address is for us first
  if( !memcmp( &arpPacket->targetIP[0],&bytIPAddress[0],4 ) )
  {
    //The arp is for us so swap the src and dest
    memcpy(&arpPacket->eth.DestAddrs[0],&arpPacket->eth.SrcAddrs[0],sizeof(bytMacAddress));
    memcpy(&arpPacket->eth.SrcAddrs[0],&bytMacAddress[0],sizeof(bytMacAddress));
    memcpy(&arpPacket->targetMAC[0],&arpPacket->senderMAC[0],sizeof(bytMacAddress));
    memcpy(&arpPacket->senderMAC[0],&bytMacAddress[0],sizeof(bytMacAddress));
    arpPacket->opCode = HTONS(ARPREPLY);
    uip_len = sizeof(ARP);  
    MACWrite();
  }
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

void PingReply()
{
  ICMPhdr* ping = (ICMPhdr*)&uip_buf[0];
  if ( ping->type == ICMPREQUEST )
  {
    ping->type = ICMPREPLY;
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

int GetPacket( int protocol )
{
  while(1)//Should make this return 0 after so many failed packets
  {
    if ( MACRead() )
    {
      EtherNetII* eth = (EtherNetII*)&uip_buf[0];
      if ( eth->type == HTONS(ARPPACKET) )
      {
        ARP* arpPacket = (ARP*)&uip_buf[0];
        if ( arpPacket->opCode == HTONS(ARPREQUEST))
          //We have an arp and we should reply
          ReplyArp();
      }
      else if( eth->type == HTONS(IPPACKET) )
      {
        //We have an IP packet and we need to check protocol.
        IPhdr* ip = (IPhdr*)&uip_buf[0];
        if( ip->protocol == protocol )
        {
          return 1;
        }
        
        //Reply to any Pings
        if( ip->protocol == ICMPPROTOCOL )
        {
          PingReply();
        }
      }
    }
  }
}
  
int IPstackInit( unsigned char const* MacAddress)
{
  initMAC();
  // Announce we are here
  SendArp(&bytIPAddress[0]);
  // Just waste a bit of time confirming no one has our IP
  for(unsigned int i = 0; i < 0x5fff; i++)
  {
    if(MACRead())
    {
      EtherNetII* eth = (EtherNetII*)&uip_buf;
      if( eth->type == HTONS(ARPPACKET) )
      {
        ARP* arp = (ARP*)&uip_buf;
        if( !memcmp(&arp->senderIP[0],&bytIPAddress[0],4))
        {
          // Uh oh this IP address is already in use
          return 0;
        }
      }
    }
    // Every now and then send out another ARP
    if( !(i % 0x1000) )
    {
      SendArp(&bytIPAddress[0]);
    }
  }
  // Well no one replied so its safe to assume IP address is OK
  // Now we need to get the routers MAC address
  SendArp(&routerIP[0]);
  for(unsigned int i = 0; i < 0x5fff; i++)
  {
    if(MACRead())
    {
      EtherNetII* eth = (EtherNetII*)&uip_buf;
      if( eth->type == HTONS(ARPPACKET) )
      {
        ARP* arp = (ARP*)&uip_buf;
        if( arp->opCode == HTONS(ARPREPLY) && !memcmp(&arp->senderIP[0],&routerIP[0],4))
        {
          // Should be the routers reply so copy the mac address
          memcpy(&bytRouterMac[0],&arp->senderMAC[0],6);
          return 1;
        }
      }
    }
    // Every now and then send out another ARP
    if( !(i % 0x1000) )
    {
      SendArp(&routerIP[0]);
    }
  }
  return 0;
}

void DNSLookup( char* url )
{
  DNShdr* dns = (DNShdr*)&uip_buf[0];
  
  dns->udp.sourcePort = HTONS(6543);//Place a number in here
  dns->udp.destPort = HTONS(DNSUDPPORT);
  dns->udp.len = 0;
  dns->udp.chksum = 0;
  dns->udp.ip.hdrlen = 5;
  dns->udp.ip.version = 4;
  dns->udp.ip.diffsf = 0;
  dns->udp.ip.ident = 0;
  dns->udp.ip.fragmentOffset1 = 0;
  dns->udp.ip.fragmentOffset2 = 0;
  dns->udp.ip.flags = 0x2;
  dns->udp.ip.ttl = 128;
  dns->udp.ip.protocol = UDPPROTOCOL;
  dns->udp.ip.chksum = 0;
  memcpy(&dns->udp.ip.source[0], &bytIPAddress[0], 4);
  memcpy(&dns->udp.ip.dest[0], &routerIP[0], 4);
  dns->udp.ip.eth.type = HTONS(IPPACKET);
  memcpy(&dns->udp.ip.eth.SrcAddrs[0],&bytMacAddress[0],6);
  memcpy(&dns->udp.ip.eth.DestAddrs[0],&bytRouterMac[0],6);
  
  dns->id = 0x4; //Chosen at random roll of dice
  dns->flags = HTONS(0x0100);
  dns->qdCount = HTONS(1);
  dns->anCount = 0;
  dns->nsCount = 0;
  dns->arCount = 0;
  //Add in question header
  char* dnsq = (char*)&uip_buf[sizeof(DNShdr)+1];//Note the +1
  int noChars = 0;
  for( char* c = &url[0]; *c != '\0' && *c !='\\'; ++c, ++dnsq)
  {
    *dnsq = *c;
    if ( *c == '.' )
    {
      *(dnsq-(noChars+1)) = noChars;
      noChars = 0;
    }
    else ++noChars;
  }
  *(dnsq-(noChars+1)) = noChars;
  *dnsq++ = 0;
  //Next finish off the question with the host type and class
  *dnsq++ = 0;
  *dnsq++ = 1;
  *dnsq++ = 0;
  *dnsq++ = 1;
  char lenOfQuery = (unsigned char*)dnsq-&uip_buf[sizeof(DNShdr)];
  uip_len = (unsigned char*)dnsq-&uip_buf[0];
  
  dns->udp.len = HTONS(uip_len-sizeof(IPhdr));
  dns->udp.ip.len = HTONS(uip_len-sizeof(EtherNetII));
  int pseudochksum = chksum(UDPPROTOCOL+uip_len-sizeof(EtherNetII),&dns->udp.ip.source[0],8);
  dns->udp.chksum = HTONS(~(chksum(pseudochksum,&uip_buf[sizeof(IPhdr)],uip_len-sizeof(IPhdr))));
  dns->udp.ip.chksum = HTONS(~(chksum(0,&uip_buf[sizeof(EtherNetII)],sizeof(IPhdr)-sizeof(EtherNetII))));
  //Calculate all lengths
  //Calculate all checksums
  MACWrite();
  
  while(1)
  {
    GetPacket(UDPPROTOCOL);
    dns = (DNShdr*)&uip_buf[0];
    if ( dns->id == 0x4) //See above for reason
    {
      //IP address is after original query so we should go to the end plus lenOfQuery
      //There are also 12 bytes of other data we do not need to know
      //Grab IP from message and copy into serverIP
      memcpy(&serverIP[0],&uip_buf[sizeof(DNShdr)+lenOfQuery+12],4);
      return;
    }
  }
}
int IPstackHTMLPost( char* url, char* data)
{
  //First we need to do some DNS looking up
  DNSLookup(url); //Fills in serverIP
  //Now that we have the IP we can connect to the server
  //Syn server
  GetPacket(TCPPROTOCOL);
  //ack syn/ack
  //Send the actual data
  return 0;
}

int IPstackIdle()
{
  GetPacket(0);
  return 1;
}