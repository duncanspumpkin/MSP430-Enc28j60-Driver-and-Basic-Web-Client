Enc28j60 driver for the msp430 modified from driver by Iain Derrington (www.kandi-electronics.com)

Implements a basic webclient that will send messages to a server. Will be able to read basic messages back.

Webclient part now working. 

.:Edit:. Webclient somewhat works. Note will often crash!

See http://mostlyprog.wordpress.com/2011/12/01/msp430-enc28j60-ethernet/ for progress.


Parts to do:

1. SPI link reading and writing.
2. ENC28J60 driver.
3. EthernetII Header.
4. IP Header.
5. TCP Header.
6. HTTP Header.

Parts complete:

1. Checked by simple reads and writes to enc28j60. See spi_test.c for example.
2. Checked by sending ARP and recieving reply. See arp_test.c for example.
3. Same as 2.
4. Checked by sending and recieving Ping. Proper Example to be added.
5. Checked by handshake with a server. Proper Example to be added.
6. Checked by downloading basic web page. Note currently will not download but most of the code is there.

Now that the basic client is working focus is on finishing TCP cleanly and general tidy up of the code base.
