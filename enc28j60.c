#include <msp430.h>
#include "enc28j60.h"
#include "spi.h"
/******************************************************************************/
/** \file enc28j60.c
 *  \brief Driver code for enc28j60.
 *  \author Iain Derrington (www.kandi-electronics.com)
 *          Modified by Duncan Frost
 *  \date  0.1 20/06/07 First Draft \n
 *         0.2 11/07/07 Removed CS check macros. Fixed bug in writePhy
 *         0.3 12.07/07 Altered for uIP 1.0 
 */
/*******************************************************************************/

#define TRUE  1
#define FALSE 0

TXSTATUS TxStatus;
RXSTATUS ptrRxStatus;
// define private functions
static unsigned char ReadETHReg(unsigned char);         // read a ETX reg
static unsigned char ReadMacReg(unsigned char);         // read a MAC reg
static unsigned int ReadPhyReg(unsigned char);         // read a PHY reg
static unsigned int ReadMacBuffer(unsigned char * ,unsigned int);    //read the mac buffer (ptrBuffer, no. of bytes)
static unsigned char WriteCtrReg(unsigned char,unsigned char);               // write to control reg
static unsigned char WritePhyReg(unsigned char,unsigned int);               // write to a phy reg
static unsigned int WriteMacBuffer(unsigned char *,unsigned int);    // write to mac buffer
static void ResetMac(void);

static unsigned char SetBitField(unsigned char, unsigned char);
static unsigned char ClrBitField(unsigned char, unsigned char);
static void BankSel(unsigned char);

//define usefull macros
#define CS BIT0

/** MACRO for selecting or deselecting chip select for the ENC28J60. Some HW dependancy.*/
#define SEL_MAC(x)  (x==TRUE) ? (P2OUT&=(~CS)) : (P2OUT|=CS)   
/** MACRO for rev B5 fix.*/
#define ERRATAFIX   SetBitField(ECON1, ECON1_TXRST);ClrBitField(ECON1, ECON1_TXRST);ClrBitField(EIR, EIR_TXERIF | EIR_TXIF)


/***********************************************************************/
/** \brief Initialise the MAC.
 *
 * Description: \n
 * a) Setup SPI device. Assume Reb B7 for sub 8MHz operation \n
 * b) Setup buffer ptrs to devide memory in In and Out mem \n
 * c) Setup receive filters (accept only unicast).\n
 * d) Setup MACON registers (MAC control registers)\n
 * e) Setup MAC address
 * f) Setup Phy registers
 * \author Iain Derrington
 * \date 0.1 20/06/07 First draft
 */
/**********************************************************************/
void initMAC(const unsigned char* deviceMAC)
{
  initSPI();        // initialise the SPI
  
  ResetMac();       // erm. Resets the MAC.
  
                    
  BankSel(0); 
  while ( !(ReadETHReg(ESTAT)&ESTAT_CLKRDY) );
  
  //---Setup Recieve Buffer---
  //Start of buffer
  WriteCtrReg(ERXSTL,(unsigned char)( RXSTART & 0x00ff));    
  WriteCtrReg(ERXSTH,(unsigned char)((RXSTART & 0xff00)>> 8));
  //End of buffer
  WriteCtrReg(ERXNDL,(unsigned char)( RXEND   & 0x00ff));
  WriteCtrReg(ERXNDH,(unsigned char)((RXEND   & 0xff00)>>8));
  //RX read ptr placed at start of buffer
  WriteCtrReg(ERXRDPTL, (unsigned char)( RXSTART & 0x00ff));
  WriteCtrReg(ERXRDPTH, (unsigned char)((RXSTART & 0xff00)>> 8));
  //---Setup Transmit Buffer---
  //Start of buffer
  WriteCtrReg(ETXSTL,(unsigned char)( TXSTART & 0x00ff)); 
  WriteCtrReg(ETXSTH,(unsigned char)((TXSTART & 0xff00)>>8));
  //End buffer not set as will be packet dependant
  
  
  BankSel(1);   
  
  //---Setup packet filter---
  //This is pinched from Guido Socher AVR enc28j60 driver
  //For broadcast packets we allow only ARP packtets
  //All other packets should be unicast only for our mac (MAADR)
  //The pattern to match on is therefore
  //Type     ETH.DST
  //ARP      BROADCAST
  //06 08 -- ff ff ff ff ff ff -> ip checksum for theses bytes=f7f9
  //in binary these poitions are:11 0000 0011 1111
  //This is hex 303F->EPMM0=0x3f,EPMM1=0x30
  WriteCtrReg(ERXFCON,( ERXFCON_UCEN + ERXFCON_CRCEN + ERXFCON_PMEN));
  WriteCtrReg(EPMM0, 0x3f);
  WriteCtrReg(EPMM1, 0x30);
  WriteCtrReg(EPMCSL,0x39);
  WriteCtrReg(EPMCSH,0xf7);
  
 
  BankSel(2);
  
  //---Setup MAC registers---                          
  WriteCtrReg(MACON1, MACON1_MARXEN|MACON1_TXPAUS|MACON1_RXPAUS);     // Enable reception of frames
  WriteCtrReg(MACON2, 0x00);
  SetBitField(MACON3, MACON3_FRMLNEN +    // Type / len field will be checked
                      MACON3_TXCRCEN +    // MAC will append valid CRC
                      MACON3_PADCFG0);    // All small packets will be padded
  WriteCtrReg(MAIPGL , 0x12);             // non back to back interpacket gap. set as per data sheet
  WriteCtrReg(MAIPGH , 0x0C); 
  WriteCtrReg(MABBIPG, 0x12);             // back to back interpacket gap. set as per data sheet
  WriteCtrReg(MAMXFLL, (unsigned char)( MAXFRAMELEN & 0x00ff));     // set max frame len
  WriteCtrReg(MAMXFLH, (unsigned char)((MAXFRAMELEN & 0xff00)>>8));
  
  //Not sure what these were for
  //WriteCtrReg(MACLCON2, 63);
  //SetBitField(MACON4, MACON4_DEFER);      
  
  
  BankSel(3);   
  
  //---Setup MAC address---
  WriteCtrReg(MAADR1,deviceMAC[0]);   
  WriteCtrReg(MAADR2,deviceMAC[1]);  
  WriteCtrReg(MAADR3,deviceMAC[2]);
  WriteCtrReg(MAADR4,deviceMAC[3]);
  WriteCtrReg(MAADR5,deviceMAC[4]);
  WriteCtrReg(MAADR6,deviceMAC[5]);

  // Initialise the PHY registes
  //WritePhyReg(PHCON1, 0x000);
  WritePhyReg(PHCON2, PHCON2_HDLDIS);
  WriteCtrReg(ECON1,  ECON1_RXEN);     //Enable the chip for reception of packets
}

/***********************************************************************/
/** \brief Writes a packet to the ENC28J60.
 *
 * Description: Writes ui_len bytes of data from ptrBufffer into ENC28J60.
 *              puts the necessary padding around the packet to make it a legit
                MAC packet.\n \n 
                1) Program ETXST.   \n
                2) Write per packet control byte.\n
                3) Program ETXND.\n
                4) Set ECON1.TXRTS.\n
                5) Check ESTAT.TXABRT. \n

 * \author Iain Derrington
 * \param ptrBuffer ptr to byte buffer. 
 * \param ui_Len Number of bytes to write from buffer. 
 * \return uint True or false. 
 */
/**********************************************************************/
unsigned int MACWrite(unsigned char* packet, unsigned int len)
{
  volatile unsigned int i;
  //Control byte meaning use settings in MACON3
  unsigned char  bytControl = 0x00;
  
  BankSel(0);             
  // Set write buffer to point to start of Tx Buffer
  WriteCtrReg(EWRPTL,(unsigned char)( TXSTART & 0x00ff));        
  WriteCtrReg(EWRPTH,(unsigned char)((TXSTART & 0xff00)>>8));
  // Tell MAC when the end of the packet is
  WriteCtrReg(ETXNDL, (unsigned char)( (len+TXSTART) & 0x00ff));       
  WriteCtrReg(ETXNDH, (unsigned char)(((len+TXSTART) & 0xff00)>>8));
  
  // write per packet control byte
  WriteMacBuffer(&bytControl,1);  
  WriteMacBuffer(packet, len);  
  
  ClrBitField(EIR,EIR_TXIF);
  SetBitField(EIE, EIE_TXIE |EIE_INTIE);

  ERRATAFIX;    
  SetBitField(ECON1, ECON1_TXRTS);                     // begin transmitting;

  do
  {      
  }while (!(ReadETHReg(EIR) & (EIR_TXIF)));             // kill some time. Note: Nice place to block?             // kill some time. Note: Nice place to block?

  ClrBitField(ECON1, ECON1_TXRTS);
  
  len++; //Adds on control byte
  WriteCtrReg(ERDPTL, (unsigned char)( len & 0x00ff));      // Setup the buffer read ptr to read status struc
  WriteCtrReg(ERDPTH, (unsigned char)((len & 0xff00)>>8));
  ReadMacBuffer(&TxStatus.v[0],7);

  if (ReadETHReg(ESTAT) & ESTAT_TXABRT)                // did transmission get interrupted?
  {
    if (TxStatus.bits.LateCollision)
    {
      ClrBitField(ECON1, ECON1_TXRTS);
      SetBitField(ECON1, ECON1_TXRTS);
      
      ClrBitField(ESTAT,ESTAT_TXABRT | ESTAT_LATECOL);
    }
    ClrBitField(EIR, EIR_TXERIF | EIR_TXIF);
    ClrBitField(ESTAT,ESTAT_TXABRT);

    return FALSE;                                          // packet transmit failed. Inform calling function
  }                                                        // calling function may inquire why packet failed by calling [TO DO] function
  else
  {
    return TRUE;                                           // all fan dabby dozy
  }
}

/***********************************************************************/
/** \brief Tries to read a packet from the ENC28J60. 
 *
 * Description: If a valid packet is available in the ENC28J60 this function reads the packet into a
 *              buffer. The memory within the ENC28J60 will then be released. This version of the
                driver does not use interrupts so this function needs to be polled.\n \n
 * 
 * 1) Read packet count register. If >0 then continue else return. \n
 * 2) Read the current ERXRDPTR value. \n           
 * 3) Write this value into ERDPT.     \n
 * 4) First two bytes contain the ptr to the start of next packet. Read this value in. \n
 * 5) Calculate length of packet. \n
 * 6) Read in status byte into private variable. \n
 * 7) Read in packet and place into buffer. \n
 * 8) Free up memory in the ENC. \n
 *
 * \author Iain Derrington
 * \param ptrBuffer ptr to buffer of bytes where the packet should be read into. 
 * \return unsigned int, the number of complete packets in the buffer -1.

 */
/**********************************************************************/
unsigned int MACRead(unsigned char* packet, unsigned int maxLen)
{
  volatile unsigned int pckLen;
  static unsigned int nextpckptr = RXSTART;
  //volatile RXSTATUS ptrRxStatus;
  volatile unsigned char numPackets;

  BankSel(1);
  // How many packets have been received
  numPackets = ReadETHReg(EPKTCNT);          
  if(numPackets == 0)
    return numPackets;    // No full packets received
  
  BankSel(0);
  //Set read pointer to start of packet
  WriteCtrReg(ERDPTL,(unsigned char)( nextpckptr & 0x00ff));   
  WriteCtrReg(ERDPTH,(unsigned char)((nextpckptr & 0xff00)>>8));
  // read next packet ptr + 4 status bytes
  ReadMacBuffer((unsigned char*)&ptrRxStatus.v[0],6);
  //Set nextpckptr for next call of ReadMAC
  nextpckptr = ptrRxStatus.bits.NextPacket;
  
  //We take away 4 as that is the CRC checksum and we do not need it
  pckLen=ptrRxStatus.bits.ByteCount - 4; 
  if( pckLen > (maxLen-1) ) pckLen = maxLen-1;
  //read the packet
  ReadMacBuffer(packet,pckLen);   
  
  //free up ENC memory my adjustng the Rx Read ptr                                      
  //See errata this fixes ERXRDPT as it has to always be odd.
  if ( ((nextpckptr - 1) < RXSTART) || ((nextpckptr-1) > RXEND))
  {
    WriteCtrReg(ERXRDPTL, (RXEND & 0x00ff));  
    WriteCtrReg(ERXRDPTH, ((RXEND & 0xff00) >> 8));
  }
  else
  {
    WriteCtrReg(ERXRDPTL, (( nextpckptr - 1 ) & 0x00ff ));  
    WriteCtrReg(ERXRDPTH, ((( nextpckptr - 1 ) & 0xff00 ) >> 8 ));
  }
  // decrement packet counter
  SetBitField(ECON2, ECON2_PKTDEC);
 
  return pckLen;
}

/*------------------------Private Functions-----------------------------*/

/***********************************************************************/
/** \brief ReadETHReg.
 *
 * Description: Reads contents of the addressed ETH reg over SPI bus. Assumes correct bank selected.
 *              
 *              
 * \author Iain Derrington
 * \param bytAddress Address of register to be read
 * \return byte Value of register.
 */
/**********************************************************************/
static unsigned char ReadETHReg(unsigned char bytAddress)
{
  unsigned char bytData;
  
  if (bytAddress > 0x1F)    
    return FALSE;                 // address invalid, [TO DO] 

  SEL_MAC(TRUE);                 // ENC CS low
  SPIWrite(&bytAddress,1);       // write opcode
  SPIRead(&bytData, 1);          // read value
  SEL_MAC(FALSE);
  
  return bytData;

}

/***********************************************************************/
/** \brief ReadMacReg.
 *
 * Description: Read contents of addressed MAC register over SPI bus. Assumes correct bank selected.
 *                    
 * \author Iain Derrington
 * \param bytAddress Address of register to read.
 * \return byte Contens of register just read.
 */
/**********************************************************************/
static unsigned char ReadMacReg(unsigned char bytAddress)
{
  unsigned char bytData;

  if (bytAddress > 0x1F)    
    return FALSE;                 // address invalid, [TO DO] 

  SEL_MAC(TRUE);                 // ENC CS low
 
  SPIWrite(&bytAddress,1);    // write opcode
  SPIRead(&bytData, 1);       // read dummy byte
  SPIRead(&bytData, 1);       // read value
  SEL_MAC(FALSE);
 

  return bytData;
}

/***********************************************************************/
/** \brief Write to phy Reg. 
 *
 * Description:  Writing to PHY registers is different to writing the other regeisters in that
                the registers can not be accessed directly. This function wraps up the requirements
                for writing to the PHY reg. \n \n
                  
                1) Write address of phy reg to MIREGADR. \n
                2) Write lower 8 bits of data to MIWRL. \n
                3) Write upper 8 bits of data to MIWRL. \n \n            
 *              
 *              
 * \author Iain Derrington
 * \param address
 * \param data
 * \return byte
 */
/**********************************************************************/
static unsigned char WritePhyReg(unsigned char address, unsigned int data)
{ 
  if (address > 0x14)
    return FALSE;
  
  BankSel(2);

  WriteCtrReg(MIREGADR,address);        // Write address of Phy reg 
  WriteCtrReg(MIWRL,(unsigned char)data);        // lower phy data 
  WriteCtrReg(MIWRH,((unsigned char)(data >>8)));    // Upper phydata

  return TRUE;
}

/***********************************************************************/
/** \brief Read from PHY reg.
 *
 * Description: No direct access allowed to phy registers so the folling process must take place. \n \n
 *              1) Write address of phy reg to read from into MIREGADR. \n
 *              2) Set MICMD.MIIRD bit and wait 10.4uS. \n
 *              3) Clear MICMD.MIIRD bit. \n
 *              4) Read data from MIRDL and MIRDH reg. \n
 * \author Iain Derrington
 * \param address
 * \return uint
 */
/**********************************************************************/
static unsigned int ReadPhyReg(unsigned char address)
{
 volatile unsigned int uiData;
 volatile unsigned char bytStat;

  BankSel(2);
  WriteCtrReg(MIREGADR,address);    // Write address of phy register to read
  SetBitField(MICMD, MICMD_MIIRD);  // Set rd bit
  do                                  
  {
    bytStat = ReadMacReg(MISTAT);
  }while(bytStat & MISTAT_BUSY);

  ClrBitField(MICMD,MICMD_MIIRD);   // Clear rd bit
  uiData = (unsigned int)ReadMacReg(MIRDL);       // Read low data byte
  uiData |=((unsigned int)ReadMacReg(MIRDH)<<8); // Read high data byte

  return uiData;
}

/***********************************************************************/
/** \brief Write to a control reg .
 *
 * Description: Writes a byte to the address register. Assumes that correct bank has
 *              all ready been selected
 *              
 * \author Iain Derrington
 * \param bytAddress Address of register to be written to. 
 * \param bytData    Data to be written. 
 * \returns byte
 */
/**********************************************************************/
static unsigned char WriteCtrReg(unsigned char bytAddress,unsigned char bytData)
{
  if (bytAddress > 0x1f)
  {
    return FALSE;
  }

  bytAddress |= WCR_OP;       // Set opcode
  SEL_MAC(TRUE);              // ENC CS low
  SPIWrite(&bytAddress,1);    // Tx opcode and address
  SPIWrite(&bytData,1);       // Tx data
  SEL_MAC(FALSE);
  
  return TRUE;
}

/***********************************************************************/
/** \brief Read bytes from MAC data buffer.
 *
 * Description: Reads a number of bytes from the ENC28J60 internal memory. Assumes auto increment
 *              is on. 
 *              
 * \author Iain Derrington
 * \param bytBuffer  Buffer to place data in. 
 * \param byt_length Number of bytes to read.
 * \return uint  Number of bytes read.
 */
/**********************************************************************/
static unsigned int ReadMacBuffer(unsigned char * bytBuffer,unsigned int byt_length)
{
  unsigned char bytOpcode;
  volatile unsigned int len;

  bytOpcode = RBM_OP;
  SEL_MAC(TRUE);            // ENC CS low
  
  SPIWrite(&bytOpcode,1);   // Tx opcode 
  len = SPIRead(bytBuffer, byt_length);   // read bytes into buffer
  SEL_MAC(FALSE);           // release CS
  

  return len;

}
/***********************************************************************/
/** \brief Write bytes to MAC data buffer.
 *
 * Description: Writes a number of bytes to the ENC28J60 internal memory. Assumes auto increment
 *              is on.
 *             
 * \author Iain Derrington
 * \param bytBuffer
 * \param ui_len
 * \return uint
 * \date WIP
 */
/**********************************************************************/
static unsigned int WriteMacBuffer(unsigned char * bytBuffer,unsigned int ui_len)
{
  unsigned char bytOpcode;
  volatile unsigned int len;

  bytOpcode = WBM_OP;
  SEL_MAC(TRUE);            // ENC CS low
  
  SPIWrite(&bytOpcode,1);   // Tx opcode 
  len = SPIWrite(bytBuffer, ui_len);   // read bytes into buffer
  SEL_MAC(FALSE);           // release CS
  

  return len;

}

/***********************************************************************/
/** \brief Set bit field. 
 *
 * Description: Sets the bit/s at the address register. Assumes correct bank has been selected.
 *                           
 * \author Iain Derrington
 * \param bytAddress Address of registed where bit is to be set
 * \param bytData    Sets all the bits high.
 * \return byte      True or false
 */
/**********************************************************************/
static unsigned char SetBitField(unsigned char bytAddress, unsigned char bytData)
{
  if (bytAddress > 0x1f)
  {
    return FALSE;
  }

  bytAddress |= BFS_OP;       // Set opcode
  SEL_MAC(TRUE);              // ENC CS low
  
  SPIWrite(&bytAddress,1);    // Tx opcode and address
  SPIWrite(&bytData,1);       // Tx data
  SEL_MAC(FALSE);
  
  return TRUE;
}

/***********************************************************************/
/** \brief Clear bit field on ctr registers.
 *
 * Description: Clears the bit/s at the address register. Assumes correct bank has been selected.
 *             
 * \author Iain Derrington
 * \param bytAddress Address of registed where bit is to be set
 * \param bytData    Sets all the bits high.
 * \return byte      True or false
 */
/**********************************************************************/
static unsigned char ClrBitField(unsigned char bytAddress, unsigned char bytData)
{
 if (bytAddress > 0x1f)
  {
    return FALSE;
  }

  bytAddress |= BFC_OP;       // Set opcode
  SEL_MAC(TRUE);              // ENC CS low
  
  SPIWrite(&bytAddress,1);    // Tx opcode and address
  SPIWrite(&bytData,1);       // Tx data
  SEL_MAC(FALSE);
  
  return TRUE;
}

/***********************************************************************/
/** \brief Bank Select.
 *
 * Description: Select the required bank within the ENC28J60
 *              
 *              
 * \author Iain Derrington
 * \param bank Value between 0 and 3.
 * \date 0.1 09/06/07 First draft
 */
/**********************************************************************/
static void BankSel(unsigned char bank)
{
  volatile unsigned char temp;

  if (bank >3)
    return;
  
  ClrBitField( ECON1, ECON1_BSEL );
  SetBitField( ECON1, bank );
}
/***********************************************************************/
/** \brief ResetMac.
 *
 * Description: Sends command to reset the MAC.
 *              
 *              
 * \author Iain Derrington
 */
/**********************************************************************/
static void ResetMac(void)
{
  unsigned char bytOpcode = RESET_OP;
  SEL_MAC(TRUE);              // ENC CS low
  
  SPIWrite(&bytOpcode,1);     // Tx opcode and address
  SEL_MAC(FALSE);
  
  __delay_cycles(1600000);
}


