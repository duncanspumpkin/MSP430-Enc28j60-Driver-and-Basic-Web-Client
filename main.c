#include <msp430.h>
#include "ipstack.h"
// Mac address of the enc28j60
const unsigned char bytMacAddress[6] = {0x00,0xa0,0xc9,0x14,0xc8,0x00};
char* url = "csr.stugo.co.uk";
char* data = "GET /admin/sensor/inuse/?id=5f3ee97228dab60a888b7128 HTTP/1.1\r\nHost: csr.stugo.co.uk\r\n\r\n";
char reply[] = "SUCCESS";

int main( void )
{
  // Stop watchdog timer to prevent time out reset
  WDTCTL = WDTPW + WDTHOLD;
  P1DIR = 0x01;
  P1OUT = 0x0;
  IPstackInit(&bytMacAddress[0]);
  IPstackHTMLPost(url,data,&reply[0]);
  while(1) IPstackIdle();
}
