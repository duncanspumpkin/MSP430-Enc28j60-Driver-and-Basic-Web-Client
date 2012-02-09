#include "spi.h"
#include "io430.h"

#define SCLK    BIT3
#define SDI     BIT2
#define SDO     BIT1
#define CS      BIT0

/***********************************************************************/
/** \brief Initilialise the SPI peripheral
 *
 * Description: init the SPI peripheral. Simple non flexible version

 */
/**********************************************************************/
void initSPI(void)
{
  UCB0CTL1 = UCSWRST;
  UCB0CTL0 = UCSYNC + UCSPB + UCMSB + UCCKPH;
  UCB0CTL1 |= UCSSEL_2;
  UCB0BR0 |= 0x01;
  UCB0BR1 = 0;
  P3SEL |= SCLK + SDO + SDI;
  P3DIR |= SCLK + SDO;
  P2DIR |= CS;
  P3DIR &= ~SDI;
  UCB0CTL1 &= ~UCSWRST;
    
//    USICTL0 |= USIPE7 +  USIPE6 + USIPE5 + USIMST + USIOE; // Port, SPI master
//  
//    //USICTL1 |= USIIE;                     // Counter interrupt, flag remains set
//  
//    USICKCTL = USISSEL0 + USISSEL1;      // /2 SMCLK
////    // enable SDI, SDO, SCLK, master mode, MSB, output enabled, hold in reset
////    USICTL0 = USIPE7 | USIPE6 | USIPE5 | USIMST | USIOE | USISWRST;
////
////    // SMCLK / 128
////    USICKCTL = USIDIV_7 + USISSEL_2;
////
////    // clock phase
////    USICTL1 |= USICKPH;
//
//    // release from reset
//    USICTL0 &= ~USISWRST;

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
  unsigned int i;//,stat;
  
  //stat= S0SPSR;
  
  if (ui_Len == 0)           // no data no send
    return 0;

    for (i=0;i<ui_Len;i++)
    {
      UCB0TXBUF = *ptrBuffer++;
      while (! (IFG2 & UCB0TXIFG));
      //while(UCB0STAT & UCBUSY);
//      USISR = *ptrBuffer++;
//      USICNT = 8;
//      //UCB0TXBUF = *ptrBuffer++;     // load spi tx reg
//      while((USICTL1 & USIIFG) != 0x01);//while(!(UCB0TXBUF)){}   // wait for transmission to complete
    }

    return i;
}

/***********************************************************************/
/** \brief SPIRead
 *
 * Description: Will read ui_Length bytes from SPI. Bytes placed into ptrbuffer

 * \author Iain Derrington
 * \param ptrBuffer Buffer containing data.
 * \param ui_Len Number of bytes to read.
 * \return uint Number of bytes read.
 */
/**********************************************************************/
unsigned int SPIRead(unsigned char * ptrBuffer, unsigned int ui_Len)
{
  //uint16_t stat;
  unsigned int i;
  
  //i= UCB0RXBUF;

  for ( i=0;i<ui_Len;i++)
  {
      while (! (IFG2 & UCB0TXIFG));
      UCB0TXBUF = 0x00;
      while (! (IFG2 & UCB0TXIFG));
      *ptrBuffer++ = UCB0RXBUF;
//    //S0SPDR= 0xff;                  // dummy transmit byte
//    USISR = 0xff;
//    USICNT = 8;
//    while((USICTL1&USIIFG)!=0x01);// wait for transmission to complete
//    *ptrBuffer++ = USISRL;         
//    //*ptrBuffer++ = UCB0RXBUF;              // read data from SPI data reg, place into buffer
  }
  return i;
}
