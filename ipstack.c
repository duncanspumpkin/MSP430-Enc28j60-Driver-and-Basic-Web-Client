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
unsigned char dnsServer[4] = {192,168,0,1};
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

void SetupBasicIPPacket( unsigned char protocol, unsigned char* destIP )
{
  IPhdr* ip = (IPhdr*)&uip_buf[0];
  
  ip->eth.type = HTONS(IPPACKET);
  memcpy(&ip->eth.DestAddrs[0],&bytRouterMac[0],6);
  memcpy(&ip->eth.SrcAddrs[0],&bytMacAddress[0],6);
  
  ip->version = 0x4;
  ip->hdrlen = 0x5;
  ip->diffsf = 0;
  ip->ident = 2; //Chosen at random roll of dice
  ip->flags = 0x2;
  ip->fragmentOffset1 = 0x0;
  ip->fragmentOffset2 = 0x0;
  ip->ttl = 128;
  ip->protocol = protocol;
  ip->chksum = 0x0;
  memcpy(&ip->source[0],&bytIPAddress[0],4);
  memcpy(&ip->dest[0],&destIP[0],4);
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
    memcpy(&arpPacket->targetIP[0],&arpPacket->senderIP[0],4);
    memcpy(&arpPacket->senderIP[0],&bytIPAddress[0],4);
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
  unsigned int destPort = tcp->destPort;
  tcp->destPort = tcp->sourcePort;
  tcp->sourcePort = destPort;
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
  tcp->SYN = 0;
  tcp->ACK = 1;
  tcp->hdrLen = (sizeof(TCPhdr)-sizeof(IPhdr))/4;
  uip_len = sizeof(TCPhdr);
  tcp->ip.len = HTONS(uip_len-sizeof(EtherNetII));
  int pseudochksum = chksum(TCPPROTOCOL+uip_len-sizeof(IPhdr),&tcp->ip.source[0],8);
  tcp->chksum = HTONS(~(chksum(pseudochksum,&uip_buf[sizeof(IPhdr)],uip_len-sizeof(IPhdr))));
  tcp->ip.chksum = HTONS(~(chksum(0,&uip_buf[sizeof(EtherNetII)],sizeof(IPhdr)-sizeof(EtherNetII))));
}
void SendPing( unsigned char* targetIP )
{
  SetupBasicIPPacket( ICMPPROTOCOL, targetIP );
  ICMPhdr* ping = (ICMPhdr*)&uip_buf[0];
  uip_len = sizeof(ICMPhdr);
  
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
  SetupBasicIPPacket(UDPPROTOCOL,&dnsServer[0]);
  DNShdr* dns = (DNShdr*)&uip_buf[0];
  
  dns->udp.sourcePort = HTONS(0xDC6F);//Place a number in here
  dns->udp.destPort = HTONS(DNSUDPPORT);
  dns->udp.len = 0;
  dns->udp.chksum = 0;
  
  dns->id = HTONS(0xfae3); //Chosen at random roll of dice
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
  //char lenOfQuery = (unsigned char*)dnsq-&uip_buf[sizeof(DNShdr)];
  uip_len = (unsigned char*)dnsq-&uip_buf[0];
  //Calculate all lengths
  dns->udp.len = HTONS(uip_len-sizeof(IPhdr));
  dns->udp.ip.len = HTONS(uip_len-sizeof(EtherNetII));
  //Calculate all checksums
  int pseudochksum = chksum(UDPPROTOCOL+uip_len-sizeof(IPhdr),&dns->udp.ip.source[0],8);
  dns->udp.chksum = HTONS(~(chksum(pseudochksum,&uip_buf[sizeof(IPhdr)],uip_len-sizeof(IPhdr))));
  dns->udp.ip.chksum = HTONS(~(chksum(0,&uip_buf[sizeof(EtherNetII)],sizeof(IPhdr)-sizeof(EtherNetII))));
  
  
  MACWrite();
  while(1)
  {
    GetPacket(UDPPROTOCOL);
    if( ((UDPhdr*)&uip_buf[0])->sourcePort == HTONS(DNSUDPPORT))
    {
      dns = (DNShdr*)&uip_buf[0];
      if ( dns->id == HTONS(0xfae3)) //See above for reason
      {
        //IP address is after original query so we should go to the end plus lenOfQuery
        //There are also 12 bytes of other data we do not need to know
        //Grab IP from message and copy into serverIP
        //The address should be the last 4 bytes of the buffer. This is not fool proof and
        //should be changed in the future
        memcpy(&serverIP[0],&uip_buf[uip_len-4],4);
        return;
      }
    }
  }
}
int IPstackHTMLPost( char* url, char* data)
{
  //First we need to do some DNS looking up
  DNSLookup(url); //Fills in serverIP
  //Now that we have the IP we can connect to the server
  memset(&uip_buf[0],0,sizeof(TCPhdr));//zero the memory as we need all flags = 0
  
  //Syn server
  SetupBasicIPPacket( TCPPROTOCOL, &serverIP[0] );
  TCPhdr* tcp = (TCPhdr*)&uip_buf[0];
  tcp->sourcePort = HTONS(0xe2d7);
  tcp->destPort = HTONS(80);
  //Seq No and Ack No are zero
  tcp->seqNo[0] = 1;
  tcp->hdrLen = (sizeof(TCPhdr)-sizeof(IPhdr))/4;
  tcp->SYN = 1;
  tcp->wndSize = HTONS(200);
  //Add in some basic options
  unsigned char opts[] = { 0x02, 0x04,0x05,0xb4,0x01,0x01,0x04,0x02};
  //memcpy(&tcp->options[0],&opts[0],8);
  uip_len = sizeof(TCPhdr);
  tcp->ip.len = HTONS(sizeof(TCPhdr)-sizeof(EtherNetII));
  int pseudochksum = chksum(TCPPROTOCOL+uip_len-sizeof(IPhdr),&tcp->ip.source[0],8);
  tcp->chksum = HTONS(~(chksum(pseudochksum,&uip_buf[sizeof(IPhdr)],uip_len-sizeof(IPhdr))));
  tcp->ip.chksum = HTONS(~(chksum(0,&uip_buf[sizeof(EtherNetII)],sizeof(IPhdr)-sizeof(EtherNetII))));
  MACWrite();
  //ack syn/ack
  GetPacket(TCPPROTOCOL);
  ackTcp(tcp);
  MACWrite();
  
  //Send the actual data
  //first we need change to a push
  tcp->PSH = 1;
  tcp->ip.ident = 0xDEED;
  //Now copy in the data
  int datalen = strlen(data);
  memcpy(&uip_buf[sizeof(TCPhdr)],data,datalen);
  uip_len = sizeof(TCPhdr) + datalen;
  tcp->ip.len = HTONS(uip_len-sizeof(EtherNetII));
  tcp->chksum = 0;
  tcp->ip.chksum = 0;
  pseudochksum = chksum(TCPPROTOCOL+uip_len-sizeof(IPhdr),&tcp->ip.source[0],8);
  tcp->chksum = HTONS(~(chksum(pseudochksum,&uip_buf[sizeof(IPhdr)],uip_len-sizeof(IPhdr))));
  tcp->ip.chksum = HTONS(~(chksum(0,&uip_buf[sizeof(EtherNetII)],sizeof(IPhdr)-sizeof(EtherNetII))));
  MACWrite();
  
  return 0;
}

int IPstackIdle()
{
  GetPacket(0);
  return 1;
}