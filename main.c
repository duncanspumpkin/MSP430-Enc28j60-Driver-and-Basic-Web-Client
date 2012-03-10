#include <msp430.h>
#include "ipstack.h"
// Mac address of the enc28j60
const unsigned char bytMacAddress[6] = {0x00,0xa0,0xc9,0x14,0xc8,0x00};
char* url = "csr.stugo.co.uk\\machine";
char* data = "machine=1&state=1";

int main( void )
{
  // Stop watchdog timer to prevent time out reset
  WDTCTL = WDTPW + WDTHOLD;
  P1DIR = 0x01;
  P1OUT = 0x0;
  
  IPstackInit(&bytMacAddress[0]);
  IPstackHTMLPost(url,data);
  while(1) IPstackIdle();
}
