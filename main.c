/*****************************************************************************
  FileName:        main.c
  Processor:       PIC32MM0256GPM064
  Compiler:        XC32 ver 2.30
  IDE :            MPLABX-IDE 5.25
  Created by:      http://strefapic.blogspot.com
 ******************************************************************************/
/*----------------------------------------------------------------------------*/
/* Wiznet W5500 Ethernet Module connect                                       */
/*----------------------------------------------------------------------------*/
// ** Sample application for PIC32MM **
//    
//    W5500      --->    MCU PIC32MM
//     RST                     -->  RC14
//     MOSI                    -->  RA7 (MISO) 
//     MISO                    -->  RA8 (MOSI) 
//     SCK                     -->  RA10 
//     CS                      -->  RC11
//     INT                     -->  RD2  (not used))
/*----------------------------------------------------------------------------*/
/* UART for Printf() function                                                 */
/*----------------------------------------------------------------------------*/ 
//    Required connections for UART:
//    
//    CP210x Module     ---> MCU PIC32MM
//    TX                     --> RA6 (not used))
//    RX                     --> RC12 (TX)
//******************************************************************************
#include "mcc_generated_files/system.h"
#include "mcc_generated_files/spi3.h"
#include "mcc_generated_files/pin_manager.h"
#include <stdio.h>
#include <string.h>
#include "delay.h"
#include "wizchip_conf.h"
#include "w5500.h"
#include "socket.h"

#define Wizchip_Reset PORTCbits.RC14

// Socket & Port number definition
#define SOCK_ID_TCP       0
#define SOCK_ID_UDP       1
#define PORT_TCP          5000
#define PORT_UDP          10001
#define DATA_BUF_SIZE     1024

wiz_NetInfo pnetinfo;
uint8_t gDATABUF[DATA_BUF_SIZE]; 
uint8_t tmp;
uint8_t HTTP_ok[] = {"HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n"};
uint8_t HTML_web[] = {"<html><body><p style=\"color: green; font-size: 2.0em;\">hello world</p></body></html>"};

/*Call back function for W5500 SPI - Theses used as parametr of reg_wizchip_spi_cbfunc()
 Should be implemented by WIZCHIP users because host is dependent it*/
uint8_t CB_SpiRead(void);
void CB_ChipSelect(void);
void CB_ChipDeselect(void);
void CB_SpiWrite(uint8_t wb);

void TCP_Server(void);
void UDP_Server(void);
void W5500_Init(void);
void network_init(void);


/*definition of network parameters*/
 wiz_NetInfo gWIZNETINFO =
{
  .mac = {0xba, 0x5a, 0x4d, 0xb4, 0xc3, 0xe0},    // Source Mac Address 
  .ip = {192, 168,  8, 220},                     // Source IP Address
  .sn = {255, 255, 255, 0},                      // Subnet Mask
  .gw = {192, 168,  8, 1},                       // Gateway IP Address
  .dns = {192, 168, 8, 1},                     // DNS server IP Address
  .dhcp = NETINFO_STATIC
 
 };

int main(void)
{
    // initialize the device
    SYSTEM_Initialize();
    printf("\r\n***** W5500_TEST_START *****\r\n");
    W5500_Init();
    printf("\r\n***** W5500 successfully connect to Router *****\r\n"); // must be connect cable W5500 to Router
    
//open_socket();    
    while (1)
    {
        /*TCP Server*/
        TCP_Server();
        /*UDP Server*/
        UDP_Server();
                       
    }
}





/*Call back function for W5500 SPI*/

void CB_ChipSelect(void) // Call back function for WIZCHIP select.
{
    CS_SetLow();
}

void CB_ChipDeselect(void) // Call back function for WIZCHIP deselect.
{
    CS_SetHigh();
}

uint8_t CB_SpiRead(void) // Callback function to read byte usig SPI.
{
    return SPI3_Exchange8bit(0xFF);
}

void CB_SpiWrite(uint8_t wb) // Callback function to write byte usig SPI.
{
    SPI3_Exchange8bit(wb);
    
}


/* Handle TCP socket state.*/
void TCP_Server(void)
{
    int32_t ret;
    uint16_t size = 0, sentsize = 0;

    // Get status of socket
    switch(getSn_SR(SOCK_ID_TCP))
    {
        // Connection established
        case SOCK_ESTABLISHED :
        {
            // Check interrupt: connection with peer is successful
            if(getSn_IR(SOCK_ID_TCP) & Sn_IR_CON)
            {
                // Clear corresponding bit
                setSn_IR(SOCK_ID_TCP,Sn_IR_CON);
            }

            // Get received data size
            if((size = getSn_RX_RSR(SOCK_ID_TCP)) > 0)
            {
                printf("\r\n***** TCP Packet Received *****\r\n");
                // Cut data to size of data buffer
                if(size > DATA_BUF_SIZE)
                {
                    size = DATA_BUF_SIZE;
                }

                // Get received data
                ret = recv(SOCK_ID_TCP, gDATABUF, size);

                // Check for error
                if(ret <= 0)
                {
                    return;
                }
                printf("\r\n");
                UART1_WriteBuffer(gDATABUF, size); //show the contents of the buffer 
                memset(gDATABUF, 0, sizeof(gDATABUF)); //clear gDATABUF
                                
                /*Send confirmation and web page back to the web client */
                ret = send(SOCK_ID_TCP, HTTP_ok, sizeof(HTTP_ok));
                ret = send(SOCK_ID_TCP, HTML_web, sizeof(HTML_web));
                    
                // Check if remote close socket
                
                if(ret < 0)
                    {
                        close(SOCK_ID_TCP);
                        return;
                    }
                  
            }
            break;
        }

        // Socket received the disconnect-request (FIN packet) from the connected peer
        case SOCK_CLOSE_WAIT :
        {
            // Disconnect socket
            if((ret = disconnect(SOCK_ID_TCP)) != SOCK_OK)
            {
                return;
            }

            break;
        }

        // Socket is opened with TCP mode
        case SOCK_INIT :
        {
            // Listen to connection request
            if( (ret = listen(SOCK_ID_TCP)) != SOCK_OK)
            {
                return;
            }
                           
            break;
        }

        // Socket is released
        case SOCK_CLOSED:
        {
            // Open TCP socket
            if((ret = socket(SOCK_ID_TCP, Sn_MR_TCP, PORT_TCP, 0x00)) != SOCK_ID_TCP)
            {
                printf("\r\n***** socket is not opened *****\r\n");
                return;
            }
           printf("\r\n***** socket is no. %d opened success *****\r\n", ret);
           break;
        }

        default:
        {
            break;
        }
    }
}

/* Handle UDP socket state.*/
void UDP_Server(void)
{
    int32_t  ret;
    uint16_t size, sentsize;
    uint8_t  destip[4];
    uint16_t destport;

    // Get status of socket
    switch(getSn_SR(SOCK_ID_UDP))
    {
        // Socket is opened in UDP mode
        case SOCK_UDP:
        {
            // Get received data size
            if((size = getSn_RX_RSR(SOCK_ID_UDP)) > 0)
            {
                printf("\r\n***** UDP Packet Received *****\r\n");
                // Cut data to size of data buffer
                if(size > DATA_BUF_SIZE)
                {
                    size = DATA_BUF_SIZE;
                }
                // Get received data
                ret = recvfrom(SOCK_ID_UDP, gDATABUF, size, destip, (uint16_t*)&destport);
                printf("\r\nIP client : %d.%d.%d.%d\r\n",destip[0],destip[1],destip[2],destip[3]);
                // Check for error
                if(ret <= 0)
                {
                    return;
                }
                printf("\r\n");
                UART1_WriteBuffer(gDATABUF, size); //show the contents of the buffer 
                memset(gDATABUF, 0, sizeof(gDATABUF)); //clear gDATABUF
                               
                // Send echo to remote
                size = (uint16_t) ret;
                sentsize = 0;
                
                while(sentsize != size)
                {
                    ret = sendto(SOCK_ID_UDP, gDATABUF + sentsize, size - sentsize, destip, destport);
                    if(ret < 0)
                    {
                        return;
                    }
                    // Update number of sent bytes
                    sentsize += ret;
                }
            }
            break;
        }

        // Socket is not opened
        case SOCK_CLOSED:
        {
            // Open UDP socket
            if((ret=socket(SOCK_ID_UDP, Sn_MR_UDP, PORT_UDP, 0x00)) != SOCK_ID_UDP)
            {
                printf("r\n***** socket is not opened *****\r\n");
                return;
            }
            printf("\r\n***** socket is no. %d opened success *****\r\n", ret);
            break;
        }

        default :
        {
           break;
        }
    }
}

/* Initialize modules */
 void W5500_Init(void)
{
    // Set Tx and Rx buffer size to 2KB
    uint8_t txsize[8] = {2,2,2,2,2,2,2,2};
    uint8_t rxsize[8] = {2,2,2,2,2,2,2,2};
       
    CB_ChipDeselect();  // Deselect module

    // Reset module
    Wizchip_Reset = 0;
    delayMs(1);
    Wizchip_Reset = 1;
    delayMs(1);

     /* Registration call back */
     //_WIZCHIP_IO_MODE_ = _WIZCHIP_IO_MODE_SPI_VDM_
    reg_wizchip_cs_cbfunc(CB_ChipSelect, CB_ChipDeselect);

    /* SPI Read & Write callback function */
    reg_wizchip_spi_cbfunc(CB_SpiRead, CB_SpiWrite);
     
    /* WIZCHIP SOCKET Buffer initialize */
       wizchip_init(txsize,rxsize);
    
    /* PHY link status check */
    do
    {
       if(!(ctlwizchip(CW_GET_PHYLINK, (void*)&tmp)))
          printf("\r\nUnknown PHY Link status.\r\n");
       delayMs(500);
    }  while(tmp == PHY_LINK_OFF); 
    printf("\r\nPHY Link status OK.\r\n");
    
    /*Intialize the network information to be used in WIZCHIP*/
    network_init();
}

void network_init(void)
{
  uint8_t tmpstr[6];
  ctlnetwork(CN_SET_NETINFO, (void*)&gWIZNETINFO); // Write Network Settings to W5500 registers
        
  // Display Network Information 
  ctlnetwork(CN_GET_NETINFO, (void*)&pnetinfo);    // Read registers value , W5500 Network Settings    
  ctlwizchip(CW_GET_ID,(void*)tmpstr); //GET ID WIZNET like this : W5500
  /*Send the data to the Uart*/
  printf("\r\n=== %s NET CONF ===\r\n",(char*)tmpstr);
  printf("\r\nMAC: %02X:%02X:%02X:%02X:%02X:%02X\r\n", pnetinfo.mac[0],pnetinfo.mac[1],pnetinfo.mac[2],
		   pnetinfo.mac[3],pnetinfo.mac[4],pnetinfo.mac[5]);
  printf("SIP: %d.%d.%d.%d\r\n", pnetinfo.ip[0],pnetinfo.ip[1],pnetinfo.ip[2],pnetinfo.ip[3]);
  printf("GAR: %d.%d.%d.%d\r\n", pnetinfo.gw[0],pnetinfo.gw[1],pnetinfo.gw[2],pnetinfo.gw[3]);
  printf("SUB: %d.%d.%d.%d\r\n", pnetinfo.sn[0],pnetinfo.sn[1],pnetinfo.sn[2],pnetinfo.sn[3]);
  printf("DNS: %d.%d.%d.%d\r\n", pnetinfo.dns[0],pnetinfo.dns[1],pnetinfo.dns[2],pnetinfo.dns[3]);
  printf("======================\r\n");

}


 
