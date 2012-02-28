Enc28j60 driver for the msp430 modified from driver by Iain Derrington (www.kandi-electronics.com)

Implements a basic webclient that will send messages to a server. Will be able to read basic messages back.

Webclient part not working yet.

See http://mostlyprog.wordpress.com/2011/12/01/msp430-enc28j60-ethernet/ for progress.


Parts to do:

1. SPI link reading and writing.
2. ENC28J60 driver.
3. EthernetII Header.
4. IP Header.
5. TCP Header.
6. HTTP Header.

Parts complete:

1. Checked by simple reads and writes to enc28j60. Examples to be added.
2. Checked by sending ARP and recieving reply. Example to be added.
3. Same as 2.

Not Complete:

4. Checked by sending and recieving Ping. Current Focus.
5. Checked by handshake with a server.
6. Checked by downloading basic web page.