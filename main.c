#include <msp430.h>
#include "ipstack.h"

const char* url = "csr.stugo.co.uk";
const char* data = "GET /admin/sensor/inuse/?id=5f3ee97228dab60a888b7128 HTTP/1.1\r\nHost: csr.stugo.co.uk\r\n\r\n";
char reply[] = "SUCCESS";

int main( void )
{
  // Stop watchdog timer to prevent time out reset
  WDTCTL = WDTPW + WDTHOLD;
  P1DIR = 0x01;
  P1OUT = 0x0;
  IPstackInit();
  IPstackHTMLPost(url, data, reply );

  while(1) IPstackIdle();
}
