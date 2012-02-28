#include "spi.h"
#include <msp430.h>

/* Modified by Duncan Frost
 *
 */

#define SCLK    BIT3
#define SDI     BIT2
#define SDO     BIT1
#define CS      BIT0

/***********************************************************************/
/** \brief Initilialise the SPI peripheral
 *
 * Description: init the SPI peripheral. 
 */
/**********************************************************************/
void initSPI(void)
{
  UCB0CTL1 = UCSWRST;
  // Most signigicant bit, Mode ( 0, 0 ), Master, Syncronous mode, two stop bits
  UCB0CTL0 = UCSYNC + UCSPB + UCMSB + UCCKPH;
  // Use CPU clk
  UCB0CTL1 |= UCSSEL_2;
  // No division of CPU clk
  UCB0BR0 |= 0x1;
  UCB0BR1 = 0;
  // Using these ports for SPI
  P3SEL |= SCLK + SDO + SDI;
  P3DIR |= SCLK + SDO;
  P2DIR |= CS;
  P3DIR &= ~SDI;
  // Start UCB0
  UCB0CTL1 &= ~UCSWRST;
}

/***********************************************************************/
/** \brief SPiWrite 
 *
 * Description: Writes bytes from buffer to SPI tx reg

 * \author Iain Derrington
 * \param ptrBuffer Pointer to buffer containing data.
 * \param ui_Len number of bytes to transmit.
 * \return uint Number of bytes transmitted.
 */
/**********************************************************************/
unsigned int SPIWrite(unsigned char * ptrBuffer, unsigned int ui_Len)
{
  unsigned int i;
  
  if (ui_Len == 0)
    return 0;

    for (i=0;i<ui_Len;i++)
    {
      UCB0TXBUF = *ptrBuffer++;
      while (! (IFG2 & UCB0TXIFG));
    }

    return i;
}

/***********************************************************************/
/** \brief SPIRead
 *
 * Description: Will read ui_Length bytes from SPI. Bytes placed into ptrbuffer
 *
 * \author Iain Derrington
 * \param ptrBuffer Buffer containing data.
 * \param ui_Len Number of bytes to read.
 * \return uint Number of bytes read.
 */
/**********************************************************************/
unsigned int SPIRead(unsigned char * ptrBuffer, unsigned int ui_Len)
{
  unsigned int i;

  for ( i=0;i<ui_Len;i++)
  {
      while (! (IFG2 & UCB0TXIFG));// Could remove this to speed up
      // Dummy byte sent to create read.
      UCB0TXBUF = 0x00;
      while (! (IFG2 & UCB0TXIFG));
      *ptrBuffer++ = UCB0RXBUF;
  }
  return i;
}
