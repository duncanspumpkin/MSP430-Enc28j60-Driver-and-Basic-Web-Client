#include <msp430.h>
#include "spi.h"
#include "enc28j60.h"
#define TRUE 1
#define FALSE 0
#define CS BIT0
/** MACRO for selecting or deselecting chip select for the ENC28J60.**/
#define SEL_MAC(x)  (x==TRUE) ? (P2OUT&=(~CS)) : (P2OUT|=CS)  
/* Duncan Frost - 17/06/2012 */

/*  Tests SPI link for sending messages.

This program will test the SPI link by sending the ENC28J60 a message to turn on
the two LEDs that can be connected as outputs to the ENC28J60.

To turn on the LEDs:
1. Device is reset.
2. The PHLCON register is updated to force on LEDs.

To write to the PHLCON register:
1. MIRREGADR register is changed to point to the PHLCON register
2. Lower byte of message is written to MIWRL register
3. Upper byte of message is written to MIWRH register

As the MIREGADR, MIWRL and MIWRH registers are located in bank 2 ECON1 will have
to be updated to select the second bank.

ECON1 Register
--7------6------5------4-------3------2------1-----0--
TXRST--RXRST--DMAST--CSUMEN--TXRTS--RXEN--BSEL1--BSEL0

BSELX = Bank select bit X
All other bits can be looked up in data sheet.


MIREGADR Register
bit 4-0 = Phy register address


MIWRL & MIWRH Registers
All bits are message sent to Phy register.


PHLCON Register
Bits 15-12 and 0 are reserved set as 0.
--11-----10-----9------8-------7-----6------5------4------3-----2------1-----0--
LACFG3-LACFG2-LACFG1-LACFG0-LBCFG3-LBCFG2-LBCFG1-LBCFG0-LFRQ1-LFRQ0-STRCH-Reseverd

LACFGX = LED A configuration bit X
LBCFGX = LED B configuration bit X
LFRQX = LED pulse stretch
STRCH = Pulse stretch enable


LYCFGX configuration registers can be set to 0b1000 to turn on the LED.

0xFF is will reset the enc28j60
0x40 is the write operation
ECON1 addr is 0x1F
MIREGADR addr is 0x14
MIWRL addr is 0x16
MIWRH addr is 0x17
PHLCON addr is 0x14
*/
int main( void )
{
  // Stop watchdog timer to prevent time out reset
  WDTCTL = WDTPW + WDTHOLD;
  initSPI(); // initialise the SPI
  unsigned char op = RESET_OP; //Reset
  
  SEL_MAC(TRUE);
  SPIWrite(&op,1);
  SEL_MAC(FALSE);
  
  __delay_cycles(1600); 
  //Wait for enc28j60 to reset. 50us requried but best give it extra.
  op = ECON1 | WCR_OP; 
  //Write next message to ECON1 register.
  SEL_MAC(TRUE);
  SPIWrite(&op,1);
  
  op = 0x02; 
  //0x2 in ECON1 selects bank 2 registers.
  SPIWrite(&op,1);
  SEL_MAC(FALSE);
  
  
  op = MIREGADR | WCR_OP; 
  //Write next message to MIREGADR register.
  SEL_MAC(TRUE);
  SPIWrite(&op,1);
  
  op = 0x14; 
  //0x14 in MIREGADR selects the PHLCON register.
  SPIWrite(&op,1);
  SEL_MAC(FALSE);
  
  op = MIWRL | WCR_OP; 
  //Write next message to MIWRL register.
  SEL_MAC(TRUE);
  SPIWrite(&op,1);
  op = 0x80; 
  //0x80 in MIWRL will turn on LEDB.
  SPIWrite(&op,1);
  SEL_MAC(FALSE);
  
  op = MIWRH | WCR_OP; 
  //Write next message to MIWRH register.
  SEL_MAC(TRUE);
  SPIWrite(&op,1);
  op = 0x38; 
  //0x38 in MIWRH will turn on LEDA.
  SPIWrite(&op,1);
  SEL_MAC(FALSE);
  
}
