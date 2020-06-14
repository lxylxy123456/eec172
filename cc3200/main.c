//*****************************************************************************
//
// Copyright (C) 2014 Texas Instruments Incorporated - http://www.ti.com/ 
// 
// 
//  Redistribution and use in source and binary forms, with or without 
//  modification, are permitted provided that the following conditions 
//  are met:
//
//    Redistributions of source code must retain the above copyright 
//    notice, this list of conditions and the following disclaimer.
//
//    Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the 
//    documentation and/or other materials provided with the   
//    distribution.
//
//    Neither the name of Texas Instruments Incorporated nor the names of
//    its contributors may be used to endorse or promote products derived
//    from this software without specific prior written permission.
//
//  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
//  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
//  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
//  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT 
//  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
//  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
//  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
//  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
//  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
//  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
//  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
//*****************************************************************************


//*****************************************************************************
//
// Application Name     -   SSL Demo
// Application Overview -   This is a sample application demonstrating the
//                          use of secure sockets on a CC3200 device.The
//                          application connects to an AP and
//                          tries to establish a secure connection to the
//                          Google server.
// Application Details  -
// docs\examples\CC32xx_SSL_Demo_Application.pdf
// or
// http://processors.wiki.ti.com/index.php/CC32xx_SSL_Demo_Application
//
//*****************************************************************************


//*****************************************************************************
//
//! \addtogroup ssl
//! @{
//
//*****************************************************************************

#include <string.h>

// Simplelink includes
#include "simplelink.h"

//Driverlib includes
#include "stdio.h"
#include "hw_types.h"
#include "hw_ints.h"
#include "rom.h"
#include "rom_map.h"
#include "interrupt.h"
#include "prcm.h"
#include "utils.h"
#include "uart.h"

#include "hw_memmap.h"
#include "hw_common_reg.h"
#include "spi.h"

#include "i2c_if.h"

//Common interface includes
// #include "pinmux.h"
#include "pin_mux_config.h"
#include "gpio_if.h"
#include "common.h"
#include "uart_if.h"

// lab3
#define SPI_IF_BIT_RATE  100000
#define TR_BUFF_SIZE     100

#define MAX_URI_SIZE 128
#define URI_SIZE MAX_URI_SIZE + 1


#define APPLICATION_NAME        "SSL"
#define APPLICATION_VERSION     "1.1.1.EEC.Spring2020"
// lab4
#define SERVER_NAME                "a2zaynoiruj7ol-ats.iot.us-east-1.amazonaws.com"
#define GOOGLE_DST_PORT             8443

// lab4
#define SL_SSL_CA_CERT "/cert/rootCA.der"
#define SL_SSL_PRIVATE "/cert/private.der"
#define SL_SSL_CLIENT  "/cert/client.der"

//NEED TO UPDATE THIS FOR IT TO WORK!
#define DATE                19    /* Current Date */
#define MONTH               5     /* Month 1-12 */
#define YEAR                2020  /* Current year */
#define HOUR                20    /* Time - hours */
#define MINUTE              34    /* Time - minutes */
#define SECOND              0     /* Time - seconds */

#define POSTHEADER "POST /things/EricLi_Spring2020_CC3200/shadow HTTP/1.1\r\n"
#define GETHEADER "GET /things/EricLi_Spring2020_CC3200/shadow HTTP/1.1\r\n"
#define HOSTHEADER "Host: a2zaynoiruj7ol-ats.iot.us-east-1.amazonaws.com\r\n"
#define CHEADER "Connection: Keep-Alive\r\n"
#define CTHEADER "Content-Type: application/json; charset=utf-8\r\n"
#define CLHEADER1 "Content-Length: "
#define CLHEADER2 "\r\n\r\n"

// lab4
#define DATA1 "{\"state\":{\"desired\": {\"state\":{\"desired\":{\"default\":\"CC3200: score %d! (max %d)\",\"sms\":\"CC3200: score %d! (max %d)\"}}} }}"

// #define KEYWD "\"messageagain\":\""
#define KEYWD "! (max "
// #define UART_PRNT_LXY Report
#define UART_PRINT_LXY(x,...)

// Application specific status/error codes
typedef enum{
    // Choosing -0x7D0 to avoid overlap w/ host-driver's error codes
    LAN_CONNECTION_FAILED = -0x7D0,
    INTERNET_CONNECTION_FAILED = LAN_CONNECTION_FAILED - 1,
    DEVICE_NOT_IN_STATION_MODE = INTERNET_CONNECTION_FAILED - 1,

    STATUS_CODE_MAX = -0xBB8
}e_AppStatusCodes;

typedef struct
{
   /* time */
   unsigned long tm_sec;
   unsigned long tm_min;
   unsigned long tm_hour;
   /* date */
   unsigned long tm_day;
   unsigned long tm_mon;
   unsigned long tm_year;
   unsigned long tm_week_day; //not required
   unsigned long tm_year_day; //not required
   unsigned long reserved[3];
}SlDateTime;


//*****************************************************************************
//                 GLOBAL VARIABLES -- Start
//*****************************************************************************
volatile unsigned long  g_ulStatus = 0;//SimpleLink Status
unsigned long  g_ulPingPacketsRecv = 0; //Number of Ping Packets received
unsigned long  g_ulGatewayIP = 0; //Network Gateway IP address
unsigned char  g_ucConnectionSSID[SSID_LEN_MAX+1]; //Connection SSID
unsigned char  g_ucConnectionBSSID[BSSID_LEN_MAX]; //Connection BSSID
signed char    *g_Host = SERVER_NAME;
SlDateTime g_time;
#if defined(ccs) || defined(gcc)
extern void (* const g_pfnVectors[])(void);
#endif
#if defined(ewarm)
extern uVectorEntry __vector_table;
#endif
//*****************************************************************************
//                 GLOBAL VARIABLES -- End
//*****************************************************************************


//****************************************************************************
//                      LOCAL FUNCTION PROTOTYPES
//****************************************************************************
static long WlanConnect();
static int set_time();
static void BoardInit(void);
static long InitializeAppVariables();
static int tls_connect();
static int connectToAccessPoint();
static int http_post(int, int, int);
static int http_get(int);
long printErrConvenience(char * msg, long retVal);
//*****************************************************************************
// SimpleLink Asynchronous Event Handlers -- Start
//*****************************************************************************


//*****************************************************************************
//
//! \brief The Function Handles WLAN Events
//!
//! \param[in]  pWlanEvent - Pointer to WLAN Event Info
//!
//! \return None
//!
//*****************************************************************************
void SimpleLinkWlanEventHandler(SlWlanEvent_t *pWlanEvent) {
    if(!pWlanEvent) {
        return;
    }

    switch(pWlanEvent->Event) {
        case SL_WLAN_CONNECT_EVENT: {
            SET_STATUS_BIT(g_ulStatus, STATUS_BIT_CONNECTION);

            //
            // Information about the connected AP (like name, MAC etc) will be
            // available in 'slWlanConnectAsyncResponse_t'.
            // Applications can use it if required
            //
            //  slWlanConnectAsyncResponse_t *pEventData = NULL;
            // pEventData = &pWlanEvent->EventData.STAandP2PModeWlanConnected;
            //

            // Copy new connection SSID and BSSID to global parameters
            memcpy(g_ucConnectionSSID,pWlanEvent->EventData.
                   STAandP2PModeWlanConnected.ssid_name,
                   pWlanEvent->EventData.STAandP2PModeWlanConnected.ssid_len);
            memcpy(g_ucConnectionBSSID,
                   pWlanEvent->EventData.STAandP2PModeWlanConnected.bssid,
                   SL_BSSID_LENGTH);

            UART_PRINT_LXY("[WLAN EVENT] STA Connected to the AP: %s , "
                       "BSSID: %x:%x:%x:%x:%x:%x\n\r",
                       g_ucConnectionSSID,g_ucConnectionBSSID[0],
                       g_ucConnectionBSSID[1],g_ucConnectionBSSID[2],
                       g_ucConnectionBSSID[3],g_ucConnectionBSSID[4],
                       g_ucConnectionBSSID[5]);
        }
        break;

        case SL_WLAN_DISCONNECT_EVENT: {
            slWlanConnectAsyncResponse_t*  pEventData = NULL;

            CLR_STATUS_BIT(g_ulStatus, STATUS_BIT_CONNECTION);
            CLR_STATUS_BIT(g_ulStatus, STATUS_BIT_IP_AQUIRED);

            pEventData = &pWlanEvent->EventData.STAandP2PModeDisconnected;

            // If the user has initiated 'Disconnect' request,
            //'reason_code' is SL_USER_INITIATED_DISCONNECTION
            if(SL_USER_INITIATED_DISCONNECTION == pEventData->reason_code) {
                UART_PRINT_LXY("[WLAN EVENT]Device disconnected from the AP: %s,"
                    "BSSID: %x:%x:%x:%x:%x:%x on application's request \n\r",
                           g_ucConnectionSSID,g_ucConnectionBSSID[0],
                           g_ucConnectionBSSID[1],g_ucConnectionBSSID[2],
                           g_ucConnectionBSSID[3],g_ucConnectionBSSID[4],
                           g_ucConnectionBSSID[5]);
            }
            else {
                UART_PRINT_LXY("[WLAN ERROR]Device disconnected from the AP AP: %s, "
                           "BSSID: %x:%x:%x:%x:%x:%x on an ERROR..!! \n\r",
                           g_ucConnectionSSID,g_ucConnectionBSSID[0],
                           g_ucConnectionBSSID[1],g_ucConnectionBSSID[2],
                           g_ucConnectionBSSID[3],g_ucConnectionBSSID[4],
                           g_ucConnectionBSSID[5]);
            }
            memset(g_ucConnectionSSID,0,sizeof(g_ucConnectionSSID));
            memset(g_ucConnectionBSSID,0,sizeof(g_ucConnectionBSSID));
        }
        break;

        default: {
            UART_PRINT_LXY("[WLAN EVENT] Unexpected event [0x%x]\n\r",
                       pWlanEvent->Event);
        }
        break;
    }
}

//*****************************************************************************
//
//! \brief This function handles network events such as IP acquisition, IP
//!           leased, IP released etc.
//!
//! \param[in]  pNetAppEvent - Pointer to NetApp Event Info
//!
//! \return None
//!
//*****************************************************************************
void SimpleLinkNetAppEventHandler(SlNetAppEvent_t *pNetAppEvent) {
    if(!pNetAppEvent) {
        return;
    }

    switch(pNetAppEvent->Event) {
        case SL_NETAPP_IPV4_IPACQUIRED_EVENT: {
            SlIpV4AcquiredAsync_t *pEventData = NULL;

            SET_STATUS_BIT(g_ulStatus, STATUS_BIT_IP_AQUIRED);

            //Ip Acquired Event Data
            pEventData = &pNetAppEvent->EventData.ipAcquiredV4;

            //Gateway IP address
            g_ulGatewayIP = pEventData->gateway;

            UART_PRINT_LXY("[NETAPP EVENT] IP Acquired: IP=%d.%d.%d.%d , "
                       "Gateway=%d.%d.%d.%d\n\r",
            SL_IPV4_BYTE(pNetAppEvent->EventData.ipAcquiredV4.ip,3),
            SL_IPV4_BYTE(pNetAppEvent->EventData.ipAcquiredV4.ip,2),
            SL_IPV4_BYTE(pNetAppEvent->EventData.ipAcquiredV4.ip,1),
            SL_IPV4_BYTE(pNetAppEvent->EventData.ipAcquiredV4.ip,0),
            SL_IPV4_BYTE(pNetAppEvent->EventData.ipAcquiredV4.gateway,3),
            SL_IPV4_BYTE(pNetAppEvent->EventData.ipAcquiredV4.gateway,2),
            SL_IPV4_BYTE(pNetAppEvent->EventData.ipAcquiredV4.gateway,1),
            SL_IPV4_BYTE(pNetAppEvent->EventData.ipAcquiredV4.gateway,0));
        }
        break;

        default: {
            UART_PRINT_LXY("[NETAPP EVENT] Unexpected event [0x%x] \n\r",
                       pNetAppEvent->Event);
        }
        break;
    }
}


//*****************************************************************************
//
//! \brief This function handles HTTP server events
//!
//! \param[in]  pServerEvent - Contains the relevant event information
//! \param[in]    pServerResponse - Should be filled by the user with the
//!                                      relevant response information
//!
//! \return None
//!
//****************************************************************************
void SimpleLinkHttpServerCallback(SlHttpServerEvent_t *pHttpEvent, SlHttpServerResponse_t *pHttpResponse) {
    // Unused in this application
}

//*****************************************************************************
//
//! \brief This function handles General Events
//!
//! \param[in]     pDevEvent - Pointer to General Event Info
//!
//! \return None
//!
//*****************************************************************************
void SimpleLinkGeneralEventHandler(SlDeviceEvent_t *pDevEvent) {
    if(!pDevEvent) {
        return;
    }

    //
    // Most of the general errors are not FATAL are are to be handled
    // appropriately by the application
    //
    UART_PRINT_LXY("[GENERAL EVENT] - ID=[%d] Sender=[%d]\n\n",
               pDevEvent->EventData.deviceEvent.status,
               pDevEvent->EventData.deviceEvent.sender);
}


//*****************************************************************************
//
//! This function handles socket events indication
//!
//! \param[in]      pSock - Pointer to Socket Event Info
//!
//! \return None
//!
//*****************************************************************************
void SimpleLinkSockEventHandler(SlSockEvent_t *pSock) {
    if(!pSock) {
        return;
    }

    switch( pSock->Event ) {
        case SL_SOCKET_TX_FAILED_EVENT:
            switch( pSock->socketAsyncEvent.SockTxFailData.status) {
                case SL_ECLOSE: 
                    UART_PRINT_LXY("[SOCK ERROR] - close socket (%d) operation "
                                "failed to transmit all queued packets\n\n", 
                                    pSock->socketAsyncEvent.SockTxFailData.sd);
                    break;
                default: 
                    UART_PRINT_LXY("[SOCK ERROR] - TX FAILED  :  socket %d , reason "
                                "(%d) \n\n",
                                pSock->socketAsyncEvent.SockTxFailData.sd, pSock->socketAsyncEvent.SockTxFailData.status);
                  break;
            }
            break;

        default:
            UART_PRINT_LXY("[SOCK EVENT] - Unexpected Event [%x0x]\n\n",pSock->Event);
          break;
    }
}


//*****************************************************************************
// SimpleLink Asynchronous Event Handlers -- End
//*****************************************************************************


//*****************************************************************************
//
//! \brief This function initializes the application variables
//!
//! \param    0 on success else error code
//!
//! \return None
//!
//*****************************************************************************
static long InitializeAppVariables() {
    g_ulStatus = 0;
    g_ulGatewayIP = 0;
    g_Host = SERVER_NAME;
    memset(g_ucConnectionSSID,0,sizeof(g_ucConnectionSSID));
    memset(g_ucConnectionBSSID,0,sizeof(g_ucConnectionBSSID));
    return SUCCESS;
}


//*****************************************************************************
//! \brief This function puts the device in its default state. It:
//!           - Set the mode to STATION
//!           - Configures connection policy to Auto and AutoSmartConfig
//!           - Deletes all the stored profiles
//!           - Enables DHCP
//!           - Disables Scan policy
//!           - Sets Tx power to maximum
//!           - Sets power policy to normal
//!           - Unregister mDNS services
//!           - Remove all filters
//!
//! \param   none
//! \return  On success, zero is returned. On error, negative is returned
//*****************************************************************************
static long ConfigureSimpleLinkToDefaultState() {
    SlVersionFull   ver = {0};
    _WlanRxFilterOperationCommandBuff_t  RxFilterIdMask = {0};

    unsigned char ucVal = 1;
    unsigned char ucConfigOpt = 0;
    unsigned char ucConfigLen = 0;
    unsigned char ucPower = 0;

    long lRetVal = -1;
    long lMode = -1;

    lMode = sl_Start(0, 0, 0);
    ASSERT_ON_ERROR(lMode);

    // If the device is not in station-mode, try configuring it in station-mode 
    if (ROLE_STA != lMode) {
        if (ROLE_AP == lMode) {
            // If the device is in AP mode, we need to wait for this event 
            // before doing anything 
            while(!IS_IP_ACQUIRED(g_ulStatus)) {
#ifndef SL_PLATFORM_MULTI_THREADED
              _SlNonOsMainLoopTask(); 
#endif
            }
        }

        // Switch to STA role and restart 
        lRetVal = sl_WlanSetMode(ROLE_STA);
        ASSERT_ON_ERROR(lRetVal);

        lRetVal = sl_Stop(0xFF);
        ASSERT_ON_ERROR(lRetVal);

        lRetVal = sl_Start(0, 0, 0);
        ASSERT_ON_ERROR(lRetVal);

        // Check if the device is in station again 
        if (ROLE_STA != lRetVal) {
            // We don't want to proceed if the device is not coming up in STA-mode 
            return DEVICE_NOT_IN_STATION_MODE;
        }
    }
    
    // Get the device's version-information
    ucConfigOpt = SL_DEVICE_GENERAL_VERSION;
    ucConfigLen = sizeof(ver);
    lRetVal = sl_DevGet(SL_DEVICE_GENERAL_CONFIGURATION, &ucConfigOpt, 
                                &ucConfigLen, (unsigned char *)(&ver));
    ASSERT_ON_ERROR(lRetVal);
    
    UART_PRINT_LXY("Host Driver Version: %s\n\r",SL_DRIVER_VERSION);
    UART_PRINT_LXY("Build Version %d.%d.%d.%d.31.%d.%d.%d.%d.%d.%d.%d.%d\n\r",
    ver.NwpVersion[0],ver.NwpVersion[1],ver.NwpVersion[2],ver.NwpVersion[3],
    ver.ChipFwAndPhyVersion.FwVersion[0],ver.ChipFwAndPhyVersion.FwVersion[1],
    ver.ChipFwAndPhyVersion.FwVersion[2],ver.ChipFwAndPhyVersion.FwVersion[3],
    ver.ChipFwAndPhyVersion.PhyVersion[0],ver.ChipFwAndPhyVersion.PhyVersion[1],
    ver.ChipFwAndPhyVersion.PhyVersion[2],ver.ChipFwAndPhyVersion.PhyVersion[3]);

    // Set connection policy to Auto + SmartConfig 
    //      (Device's default connection policy)
    lRetVal = sl_WlanPolicySet(SL_POLICY_CONNECTION, 
                                SL_CONNECTION_POLICY(1, 0, 0, 0, 1), NULL, 0);
    ASSERT_ON_ERROR(lRetVal);

    // Remove all profiles
    lRetVal = sl_WlanProfileDel(0xFF);
    ASSERT_ON_ERROR(lRetVal);

    

    //
    // Device in station-mode. Disconnect previous connection if any
    // The function returns 0 if 'Disconnected done', negative number if already
    // disconnected Wait for 'disconnection' event if 0 is returned, Ignore 
    // other return-codes
    //
    lRetVal = sl_WlanDisconnect();
    if(0 == lRetVal) {
        // Wait
        while(IS_CONNECTED(g_ulStatus)) {
#ifndef SL_PLATFORM_MULTI_THREADED
              _SlNonOsMainLoopTask(); 
#endif
        }
    }

    // Enable DHCP client
    lRetVal = sl_NetCfgSet(SL_IPV4_STA_P2P_CL_DHCP_ENABLE,1,1,&ucVal);
    ASSERT_ON_ERROR(lRetVal);

    // Disable scan
    ucConfigOpt = SL_SCAN_POLICY(0);
    lRetVal = sl_WlanPolicySet(SL_POLICY_SCAN , ucConfigOpt, NULL, 0);
    ASSERT_ON_ERROR(lRetVal);

    // Set Tx power level for station mode
    // Number between 0-15, as dB offset from max power - 0 will set max power
    ucPower = 0;
    lRetVal = sl_WlanSet(SL_WLAN_CFG_GENERAL_PARAM_ID, 
            WLAN_GENERAL_PARAM_OPT_STA_TX_POWER, 1, (unsigned char *)&ucPower);
    ASSERT_ON_ERROR(lRetVal);

    // Set PM policy to normal
    lRetVal = sl_WlanPolicySet(SL_POLICY_PM , SL_NORMAL_POLICY, NULL, 0);
    ASSERT_ON_ERROR(lRetVal);

    // Unregister mDNS services
    lRetVal = sl_NetAppMDNSUnRegisterService(0, 0);
    ASSERT_ON_ERROR(lRetVal);

    // Remove  all 64 filters (8*8)
    memset(RxFilterIdMask.FilterIdMask, 0xFF, 8);
    lRetVal = sl_WlanRxFilterSet(SL_REMOVE_RX_FILTER, (_u8 *)&RxFilterIdMask,
                       sizeof(_WlanRxFilterOperationCommandBuff_t));
    ASSERT_ON_ERROR(lRetVal);

    lRetVal = sl_Stop(SL_STOP_TIMEOUT);
    ASSERT_ON_ERROR(lRetVal);

    InitializeAppVariables();
    
    return lRetVal; // Success
}


//*****************************************************************************
//
//! Board Initialization & Configuration
//!
//! \param  None
//!
//! \return None
//
//*****************************************************************************
static void BoardInit(void) {
/* In case of TI-RTOS vector table is initialize by OS itself */
#ifndef USE_TIRTOS
  //
  // Set vector table base
  //
#if defined(ccs)
    MAP_IntVTableBaseSet((unsigned long)&g_pfnVectors[0]);
#endif
#if defined(ewarm)
    MAP_IntVTableBaseSet((unsigned long)&__vector_table);
#endif
#endif
    //
    // Enable Processor
    //
    MAP_IntMasterEnable();
    MAP_IntEnable(FAULT_SYSTICK);

    PRCMCC3200MCUInit();
}


//****************************************************************************
//
//! \brief Connecting to a WLAN Accesspoint
//!
//!  This function connects to the required AP (SSID_NAME) with Security
//!  parameters specified in te form of macros at the top of this file
//!
//! \param  None
//!
//! \return  0 on success else error code
//!
//! \warning    If the WLAN connection fails or we don't aquire an IP
//!            address, It will be stuck in this function forever.
//
//****************************************************************************
static long WlanConnect() {
    SlSecParams_t secParams = {0};
    long lRetVal = 0;

    secParams.Key = SECURITY_KEY;
    secParams.KeyLen = strlen(SECURITY_KEY);
    secParams.Type = SECURITY_TYPE;

    UART_PRINT_LXY("Attempting connection to access point: ");
    UART_PRINT_LXY(SSID_NAME);
    UART_PRINT_LXY("... ...");
    lRetVal = sl_WlanConnect(SSID_NAME, strlen(SSID_NAME), 0, &secParams, 0);
    ASSERT_ON_ERROR(lRetVal);

    UART_PRINT_LXY(" Connected!!!\n\r");


    // Wait for WLAN Event
    while((!IS_CONNECTED(g_ulStatus)) || (!IS_IP_ACQUIRED(g_ulStatus))) {
        // Toggle LEDs to Indicate Connection Progress
        _SlNonOsMainLoopTask();
        GPIO_IF_LedOff(MCU_IP_ALLOC_IND);
        MAP_UtilsDelay(800000);
        _SlNonOsMainLoopTask();
        GPIO_IF_LedOn(MCU_IP_ALLOC_IND);
        MAP_UtilsDelay(800000);
    }

    return SUCCESS;

}

//*****************************************************************************
//
//! This function updates the date and time of CC3200.
//!
//! \param None
//!
//! \return
//!     0 for success, negative otherwise
//!
//*****************************************************************************

static int set_time() {
    long retVal;

    g_time.tm_day = DATE;
    g_time.tm_mon = MONTH;
    g_time.tm_year = YEAR;
    g_time.tm_sec = HOUR;
    g_time.tm_hour = MINUTE;
    g_time.tm_min = SECOND;

    retVal = sl_DevSet(SL_DEVICE_GENERAL_CONFIGURATION,
                          SL_DEVICE_GENERAL_CONFIGURATION_DATE_TIME,
                          sizeof(SlDateTime),(unsigned char *)(&g_time));

    ASSERT_ON_ERROR(retVal);
    return SUCCESS;
}

//*****************************************************************************
//
//! This function demonstrates how certificate can be used with SSL.
//! The procedure includes the following steps:
//! 1) connect to an open AP
//! 2) get the server name via a DNS request
//! 3) define all socket options and point to the CA certificate
//! 4) connect to the server via TCP
//!
//! \param None
//!
//! \return  0 on success else error code
//! \return  LED1 is turned solid in case of success
//!    LED2 is turned solid in case of failure
//!
//*****************************************************************************
static int tls_connect() {
    SlSockAddrIn_t    Addr;
    int    iAddrSize;
    unsigned char    ucMethod = SL_SO_SEC_METHOD_TLSV1_2;
    unsigned int uiIP,uiCipher = SL_SEC_MASK_TLS_ECDHE_RSA_WITH_AES_256_CBC_SHA;
    long lRetVal = -1;
    int iSockID;

    lRetVal = sl_NetAppDnsGetHostByName(g_Host, strlen((const char *)g_Host),
                                    (unsigned long*)&uiIP, SL_AF_INET);

    if(lRetVal < 0) {
        return printErrConvenience("Device couldn't retrieve the host name \n\r", lRetVal);
    }

    Addr.sin_family = SL_AF_INET;
    Addr.sin_port = sl_Htons(GOOGLE_DST_PORT);
    Addr.sin_addr.s_addr = sl_Htonl(uiIP);
    iAddrSize = sizeof(SlSockAddrIn_t);
    //
    // opens a secure socket 
    //
    iSockID = sl_Socket(SL_AF_INET,SL_SOCK_STREAM, SL_SEC_SOCKET);
    if( iSockID < 0 ) {
        return printErrConvenience("Device unable to create secure socket \n\r", lRetVal);
    }

    //
    // configure the socket as TLS1.2
    //
    lRetVal = sl_SetSockOpt(iSockID, SL_SOL_SOCKET, SL_SO_SECMETHOD, &ucMethod,\
                               sizeof(ucMethod));
    if(lRetVal < 0) {
        return printErrConvenience("Device couldn't set socket options \n\r", lRetVal);
    }
    //
    //configure the socket as ECDHE RSA WITH AES256 CBC SHA
    //
    lRetVal = sl_SetSockOpt(iSockID, SL_SOL_SOCKET, SL_SO_SECURE_MASK, &uiCipher,\
                           sizeof(uiCipher));
    if(lRetVal < 0) {
        return printErrConvenience("Device couldn't set socket options \n\r", lRetVal);
    }

    //
    //configure the socket with CA certificate - for server verification
    //
    lRetVal = sl_SetSockOpt(iSockID, SL_SOL_SOCKET, \
                           SL_SO_SECURE_FILES_CA_FILE_NAME, \
                           SL_SSL_CA_CERT, \
                           strlen(SL_SSL_CA_CERT));

    if(lRetVal < 0) {
        return printErrConvenience("Device couldn't set socket options \n\r", lRetVal);
    }

    //configure the socket with Client Certificate - for server verification
    //
    lRetVal = sl_SetSockOpt(iSockID, SL_SOL_SOCKET, \
                SL_SO_SECURE_FILES_CERTIFICATE_FILE_NAME, \
                                    SL_SSL_CLIENT, \
                           strlen(SL_SSL_CLIENT));

    if(lRetVal < 0) {
        return printErrConvenience("Device couldn't set socket options \n\r", lRetVal);
    }

    //configure the socket with Private Key - for server verification
    //
    lRetVal = sl_SetSockOpt(iSockID, SL_SOL_SOCKET, \
            SL_SO_SECURE_FILES_PRIVATE_KEY_FILE_NAME, \
            SL_SSL_PRIVATE, \
                           strlen(SL_SSL_PRIVATE));

    if(lRetVal < 0) {
        return printErrConvenience("Device couldn't set socket options \n\r", lRetVal);
    }


    /* connect to the peer device - Google server */
    lRetVal = sl_Connect(iSockID, ( SlSockAddr_t *)&Addr, iAddrSize);

    if(lRetVal < 0) {
        UART_PRINT_LXY("Device couldn't connect to server:");
        UART_PRINT_LXY(SERVER_NAME);
        UART_PRINT_LXY("\n\r");
        return printErrConvenience("Device couldn't connect to server \n\r", lRetVal);
    }
    else {
        UART_PRINT_LXY("Device has connected to the website:");
        UART_PRINT_LXY(SERVER_NAME);
        UART_PRINT_LXY("\n\r");
    }

    GPIO_IF_LedOff(MCU_RED_LED_GPIO);
    GPIO_IF_LedOn(MCU_GREEN_LED_GPIO);
    return iSockID;
}



long printErrConvenience(char * msg, long retVal) {
    UART_PRINT_LXY(msg);
    GPIO_IF_LedOn(MCU_RED_LED_GPIO);
    return retVal;
}



int connectToAccessPoint() {
    long lRetVal = -1;
    GPIO_IF_LedConfigure(LED1|LED3);

    GPIO_IF_LedOff(MCU_RED_LED_GPIO);
    GPIO_IF_LedOff(MCU_GREEN_LED_GPIO);

    lRetVal = InitializeAppVariables();
    ASSERT_ON_ERROR(lRetVal);

    //
    // Following function configure the device to default state by cleaning
    // the persistent settings stored in NVMEM (viz. connection profiles &
    // policies, power policy etc)
    //
    // Applications may choose to skip this step if the developer is sure
    // that the device is in its default state at start of applicaton
    //
    // Note that all profiles and persistent settings that were done on the
    // device will be lost
    //
    lRetVal = ConfigureSimpleLinkToDefaultState();
    if(lRetVal < 0) {
      if (DEVICE_NOT_IN_STATION_MODE == lRetVal)
          UART_PRINT_LXY("Failed to configure the device in its default state \n\r");

      return lRetVal;
    }

    UART_PRINT_LXY("Device is configured in default state \n\r");

    CLR_STATUS_BIT_ALL(g_ulStatus);

    ///
    // Assumption is that the device is configured in station mode already
    // and it is in its default state
    //
    lRetVal = sl_Start(0, 0, 0);
    if (lRetVal < 0 || ROLE_STA != lRetVal) {
        UART_PRINT_LXY("Failed to start the device \n\r");
        return lRetVal;
    }

    UART_PRINT_LXY("Device started as STATION \n\r");

    //
    //Connecting to WLAN AP
    //
    lRetVal = WlanConnect();
    if(lRetVal < 0) {
        UART_PRINT_LXY("Failed to establish connection w/ an AP \n\r");
        GPIO_IF_LedOn(MCU_RED_LED_GPIO);
        return lRetVal;
    }

    UART_PRINT_LXY("Connection established w/ AP and IP is aquired \n\r");
    return 0;
}

// proj: GPIO accesses

void Write_DC(int val) {
    GPIOPinWrite(GPIOA3_BASE, 0x80, val ? 0x80 : 0x0);  // PIN_45
}

void Write_R(int val) {
    GPIOPinWrite(GPIOA3_BASE, 0x02, val ? 0x02 : 0x0);  // PIN_21
}

void Write_OC(int val) {
    GPIOPinWrite(GPIOA3_BASE, 0x10, val ? 0x10 : 0x0);  // PIN_18
}

int read_SW2() {
    return GPIOPinRead(GPIOA2_BASE, 0x40) ? 1 : 0;      // GPIO 22
}

int read_SW3() {
    return GPIOPinRead(GPIOA1_BASE, 0x20) ? 1 : 0;      // GPIO 13
}

// proj <- lab4 <- lab3
void I2C_read(char* ax, char* ay, char* az) {
    const unsigned char ucDevAddr = 0x18, ucRegOffset = 0x2;
    unsigned char result[6];
    I2C_IF_Write(ucDevAddr, &ucRegOffset, 1, 0);
    I2C_IF_Read(ucDevAddr, result, 6);
    if (ax) *ax = result[1];
    if (ay) *ay = result[3];
    if (az) *az = result[5];
}

// proj <- lab4 <- lab3
void main()
{
    BoardInit();
    PinMuxConfig();
    // Enable the SPI module clock
    MAP_PRCMPeripheralClkEnable(PRCM_GSPI,PRCM_RUN_MODE_CLK);

    InitTerm();
    ClearTerm();
    Message("\n\r");
    Message("\t*********************************\n\r");
    Message("\t  EEC 172 Final Project Eric Li  \n\r");
    Message("\t*********************************\n\r");
    Message("\n\r\n\r");
    Message("If you have trouble accessing AWS, press SW2 and SW3 will disable WiFi communication.\n\r\n\r");

    // Reset the peripheral
    MAP_PRCMPeripheralReset(PRCM_GSPI);

    // lxy
    MAP_SPIReset(GSPI_BASE);
    MAP_SPIConfigSetExpClk(GSPI_BASE,MAP_PRCMPeripheralClockGet(PRCM_GSPI),
                     SPI_IF_BIT_RATE,SPI_MODE_MASTER,SPI_SUB_MODE_0,
                     (SPI_SW_CTRL_CS | SPI_4PIN_MODE | SPI_TURBO_OFF | SPI_CS_ACTIVEHIGH | SPI_WL_8));
    MAP_SPIEnable(GSPI_BASE);

    Write_DC(0);
    Write_R(0);
    Write_OC(0);
    Adafruit_Init();

    I2C_IF_Open(I2C_MASTER_MODE_FST);

    project_main();
}

void main_post(int score, int max_score) {
    long lRetVal = -1;
    UART_PRINT_LXY("main_post\n\r");
    lRetVal = connectToAccessPoint();
    lRetVal = set_time();
    if(lRetVal < 0) {
        UART_PRINT_LXY("Unable to set time in the device");
        LOOP_FOREVER();
    }
    lRetVal = tls_connect();
    if(lRetVal < 0) {
        ERR_PRINT(lRetVal);
    }
    http_post(lRetVal, score, max_score);
    sl_Stop(SL_STOP_TIMEOUT);
    return 0;
}

static int http_post(int iTLSSockID, int score, int max_score){
    char acSendBuff[512];
    char acRecvbuff[1460];
    char cCLLength[200];
    char* pcBufHeaders;
    int lRetVal = 0;

    char DATAS[1000];
    sprintf(DATAS, DATA1, score, max_score, score, max_score);
    // Report("<%s>", DATAS);

    pcBufHeaders = acSendBuff;
    strcpy(pcBufHeaders, POSTHEADER);
    pcBufHeaders += strlen(POSTHEADER);
    strcpy(pcBufHeaders, HOSTHEADER);
    pcBufHeaders += strlen(HOSTHEADER);
    strcpy(pcBufHeaders, CHEADER);
    pcBufHeaders += strlen(CHEADER);
    strcpy(pcBufHeaders, "\r\n\r\n");

    int dataLength = strlen(DATAS);

    strcpy(pcBufHeaders, CTHEADER);
    pcBufHeaders += strlen(CTHEADER);
    strcpy(pcBufHeaders, CLHEADER1);

    pcBufHeaders += strlen(CLHEADER1);
    sprintf(cCLLength, "%d", dataLength);

    strcpy(pcBufHeaders, cCLLength);
    pcBufHeaders += strlen(cCLLength);
    strcpy(pcBufHeaders, CLHEADER2);
    pcBufHeaders += strlen(CLHEADER2);

    strcpy(pcBufHeaders, DATAS);
    pcBufHeaders += strlen(DATAS);

    int testDataLength = strlen(pcBufHeaders);

    UART_PRINT_LXY(acSendBuff);


    //
    // Send the packet to the server */
    //
    lRetVal = sl_Send(iTLSSockID, acSendBuff, strlen(acSendBuff), 0);
    if(lRetVal < 0) {
        UART_PRINT_LXY("POST failed. Error Number: %i\n\r",lRetVal);
        sl_Close(iTLSSockID);
        GPIO_IF_LedOn(MCU_RED_LED_GPIO);
        return lRetVal;
    }
    lRetVal = sl_Recv(iTLSSockID, &acRecvbuff[0], sizeof(acRecvbuff), 0);
    if(lRetVal < 0) {
        UART_PRINT_LXY("Received failed. Error Number: %i\n\r",lRetVal);
        //sl_Close(iSSLSockID);
        GPIO_IF_LedOn(MCU_RED_LED_GPIO);
           return lRetVal;
    }
    else {
        acRecvbuff[lRetVal+1] = '\0';
        UART_PRINT_LXY(acRecvbuff);
        UART_PRINT_LXY("\n\r\n\r");
    }
}

int main_get() {
    long lRetVal = -1;
    UART_PRINT_LXY("main_get\n\r");
    lRetVal = connectToAccessPoint();
    lRetVal = set_time();
    if(lRetVal < 0) {
        UART_PRINT_LXY("Unable to set time in the device");
        LOOP_FOREVER();
    }
    lRetVal = tls_connect();
    if(lRetVal < 0) {
        ERR_PRINT(lRetVal);
    }
    int ans = http_get(lRetVal);
    sl_Stop(SL_STOP_TIMEOUT);
    return ans;
}

static int http_get(int iTLSSockID){
    char acSendBuff[512];
    char acRecvbuff[1460];
    char cCLLength[200];
    char* pcBufHeaders;
    int lRetVal = 0;

    pcBufHeaders = acSendBuff;
    strcpy(pcBufHeaders, GETHEADER);
    pcBufHeaders += strlen(GETHEADER);
    strcpy(pcBufHeaders, HOSTHEADER);
    pcBufHeaders += strlen(HOSTHEADER);
    strcpy(pcBufHeaders, CHEADER);
    pcBufHeaders += strlen(CHEADER);
    strcpy(pcBufHeaders, "\r\n\r\n");

    int testDataLength = strlen(pcBufHeaders);

    UART_PRINT_LXY(acSendBuff);


    //
    // Send the packet to the server */
    //
    lRetVal = sl_Send(iTLSSockID, acSendBuff, strlen(acSendBuff), 0);
    if(lRetVal < 0) {
        UART_PRINT_LXY("POST failed. Error Number: %i\n\r",lRetVal);
        sl_Close(iTLSSockID);
        GPIO_IF_LedOn(MCU_RED_LED_GPIO);
        return lRetVal;
    }
    lRetVal = sl_Recv(iTLSSockID, &acRecvbuff[0], sizeof(acRecvbuff), 0);
    if(lRetVal < 0) {
        UART_PRINT_LXY("Received failed. Error Number: %i\n\r",lRetVal);
        //sl_Close(iSSLSockID);
        GPIO_IF_LedOn(MCU_RED_LED_GPIO);
           return lRetVal;
    }
    else {
        acRecvbuff[lRetVal+1] = '\0';
        UART_PRINT_LXY(acRecvbuff);
        // lab4: find keyword
        // strstr(acRecvbuff)
        if (strncmp(acRecvbuff, "HTTP/1.1 200 OK", 15)) {
            Report("GET != 200\r\n");
            return -1;
        }
        char* found = strstr(acRecvbuff, KEYWD);
        if (!found) {
            Report("GET KEYWD not found\r\n");
            return -1;
        }
        char* st = found + strlen(KEYWD);
        int ans;
        sscanf(st, "%d", &ans);
        return ans;
    }
}

// Note: this file is mixed with template code and lxylxy123456's
// code. Please use GNU Affero General Public License version 3 for the latter.
// Most of the program below are from lxylxy123456's.

//  
//  eec172 - EEC 172 Final Project
//  Copyright (C) 2020  lxylxy123456
//  
//  This program is free software: you can redistribute it and/or modify
//  it under the terms of the GNU Affero General Public License as
//  published by the Free Software Foundation, either version 3 of the
//  License, or (at your option) any later version.
//  
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU Affero General Public License for more details.
//  
//  You should have received a copy of the GNU Affero General Public License
//  along with this program.  If not, see <https://www.gnu.org/licenses/>.
//  

// proj: game logic

#include <stdlib.h>
#include <assert.h>
#include <math.h>

#define CHAR_W 6
#define CHAR_H 8

typedef float myfloat;

int clock_tick(int fps)
{
    static unsigned long long prev_time = 0;
    int interval = 32768 / fps;
    unsigned long long next_time = prev_time + interval;
    unsigned long long cur = PRCMSlowClkCtrGet();
    if (next_time > prev_time) {
        while (prev_time <= cur && cur < next_time)
            cur = PRCMSlowClkCtrGet();
    } else {
        while (prev_time <= cur || cur < next_time)
            cur = PRCMSlowClkCtrGet();
    }
    prev_time = cur;
}

int get_SW2() {
    // Require observing rising edge
    static int pressed = 0;
    int high = read_SW2();
    int ans = high && !pressed;
    pressed = high;
    return ans;
}

int get_SW3() {
    // Require observing rising edge
    static int pressed = 0;
    int high = read_SW3();
    int ans = high && !pressed;
    pressed = high;
    return ans;
}

typedef short color_t;
typedef short oled_t;

// proj: draw optimization

color_t virt_oled[128][128];

#include "Adafruit_SSD1351.h"

void writeCommandRender(unsigned char c) {
    unsigned char buf_in = c, buf_out;
    Write_DC(0);
    Write_OC(0);
    MAP_SPITransfer(GSPI_BASE, &buf_in, &buf_out, 1, SPI_CS_ENABLE|SPI_CS_DISABLE);
    Write_OC(1);
}
//*****************************************************************************

void writeDataRender(unsigned char c) {
    unsigned char buf_in = c, buf_out;
    Write_DC(1);
    Write_OC(0);
    MAP_SPITransfer(GSPI_BASE, &buf_in, &buf_out, 1, SPI_CS_ENABLE|SPI_CS_DISABLE);
    Write_OC(1);
}

// proj: rotation of screen
// 0:  -, +-
// 1: -+,  -
// 2:  +, -+
// 3: +-,  +
int rotation_stat = 0;

void render(oled_t x, oled_t y, oled_t w, oled_t h) {
    // From fillRect(x, y, w, h, ...)
    switch (rotation_stat) {
    case 0: {
        writeCommandRender(SSD1351_CMD_SETCOLUMN);
        writeDataRender(x);
        writeDataRender(x+w-1);
        writeCommandRender(SSD1351_CMD_SETROW);
        writeDataRender(y);
        writeDataRender(y+h-1);
        writeCommandRender(SSD1351_CMD_WRITERAM);
        oled_t i, j;
        for (j = 0; j < h; j++) {
            for (i = 0; i < w; i++) {
                color_t color = virt_oled[x+i][y+j];
                writeDataRender(color >> 8);
                writeDataRender(color);
            }
        }
        break;
    }
    case 1: {
        writeCommandRender(SSD1351_CMD_SETCOLUMN);
        writeDataRender(y);
        writeDataRender(y+h-1);
        writeCommandRender(SSD1351_CMD_SETROW);
        writeDataRender(127-(x+w-1));
        writeDataRender(127-x);
        writeCommandRender(SSD1351_CMD_WRITERAM);
        oled_t i, j;
        for (i = w-1; i >= 0; i--) {
            for (j = 0; j < h; j++) {
                color_t color = virt_oled[x+i][y+j];
                writeDataRender(color >> 8);
                writeDataRender(color);
            }
        }
        break;
    }
    case 2: {
        writeCommandRender(SSD1351_CMD_SETCOLUMN);
        writeDataRender(127-(x+w-1));
        writeDataRender(127-x);
        writeCommandRender(SSD1351_CMD_SETROW);
        writeDataRender(127-(y+h-1));
        writeDataRender(127-y);
        writeCommandRender(SSD1351_CMD_WRITERAM);
        oled_t i, j;
        for (j = h-1; j >= 0; j--) {
            for (i = w-1; i >= 0; i--) {
                color_t color = virt_oled[x+i][y+j];
                writeDataRender(color >> 8);
                writeDataRender(color);
            }
        }
        break;
    }
    case 3: {
        writeCommandRender(SSD1351_CMD_SETCOLUMN);
        writeDataRender(127-(y+h-1));
        writeDataRender(127-y);
        writeCommandRender(SSD1351_CMD_SETROW);
        writeDataRender(x);
        writeDataRender(x+w-1);
        writeCommandRender(SSD1351_CMD_WRITERAM);
        oled_t i, j;
        for (i = 0; i < w; i++) {
            for (j = h-1; j >= 0; j--) {
                color_t color = virt_oled[x+i][y+j];
                writeDataRender(color >> 8);
                writeDataRender(color);
            }
        }
        break;
    }
    }
}

void draw_point(oled_t x, oled_t y, color_t color) {
    virt_oled[x][y] = color;
    // render(x, y, 1, 1);
    // fillRect(x, y, 1, 1, color);
}

int no_auto_render = 0;

void draw_rect(oled_t x, oled_t y, oled_t w, oled_t h, color_t color) {
    //fillRect(x, y, w, h, color);
    oled_t i, j;
    for (i = 0; i < w; i++)
        for (j = 0; j < h; j++)
            virt_oled[x+i][y+j] = color;
    if (!no_auto_render)
        render(x, y, w, h);
}

signed char get_i2c() {
    unsigned char vx, vy, vz;
    I2C_read(&vx, &vy, &vz);
    Report("[i2c: %d %d %d]\r\n", (signed char) vx, (signed char) vy, (signed char) vz);
    switch (rotation_stat) {
    case 0:
        return -vy;
    case 1:
        return vx;
    case 2:
        return vy;
    case 3:
        return -vx;
    }
}

void set_rotation_stat() {
    // Determine rotation status based on current i2c
    unsigned char vx, vy, vz;
    I2C_read(&vx, &vy, &vz);
    signed char x = vx, y = vy, z = vz;
    if (abs(x) >= abs(y)) {
        if (x >= 0)
            rotation_stat = 2;
        else
            rotation_stat = 0;
    } else {
        if (y >= 0)
            rotation_stat = 3;
        else
            rotation_stat = 1;
    }
    Report("[rot: %d %d %d -> %d]\r\n", x, y, z, rotation_stat);
}

#define LONGER_RAND_MAX 0x7fffffff

int longer_rand() {
    assert(RAND_MAX == 0x7fff);
    return (int) ((((unsigned int) rand()) << 16) | (unsigned int) rand());
}

myfloat get_random_float() {
    // Return random number in [0, 1)
    return (myfloat) longer_rand() / ((myfloat) LONGER_RAND_MAX + 1);
}

int get_random_int(int upper_bound) {
    // Return random number in [0, upper bound)
    return longer_rand() % upper_bound;    // not perfect
}

#include "glcdfont.h"
#define BLACK   0x0000

void draw_char(oled_t x, oled_t y, char ch, color_t color) {
    color_t c;
    oled_t i, j;
    for (i = 0; i < CHAR_W; i++) {
        for (j = 0; j < CHAR_H; j++) {
            if (i == 5)
                c = BLACK;
            else
                c = (((font[ch * 5 + i]) >> (j)) & 1) ? color : BLACK;
            draw_point(x + i, y + j, c);
        }
    }
    if (!no_auto_render)
        render(x, y, CHAR_W, CHAR_H);
}

#define ONLINE
#ifdef ONLINE
int get_max_score() {
    if (read_SW2() && read_SW3())
        return 0;
    return main_get();
}

void send_score(int score, int max_score) {
    if (read_SW2() && read_SW3())
        return;
    main_post(score, max_score);
}
#else
int get_max_score() {
    return 9;
}

void send_score(int score, int max_score) {
    Report("Send score (%d, %d)\r\n", score, max_score);
}
#endif

// Theoreotically portable code starts

#define BLACK   0x0000
#define BLUE    0x001F
#define GREEN   0x07E0
#define CYAN    0x07FF
#define RED     0xF800
#define MAGENTA 0xF81F
#define YELLOW  0xFFE0
#define WHITE   0xFFFF
#define GRAY    0x7bef

#define CHAR_W 6
#define CHAR_H 8
#define TRAY_C WHITE
#define CHAR_C WHITE
#define HIGH_C RED
const color_t BRICK_COLORS[] = {RED, GREEN, YELLOW, CYAN, MAGENTA};
#define NBRICK_COLORS (sizeof(BRICK_COLORS) / sizeof(BRICK_COLORS[0]))

#define BOARD_L 10
#define BOARD_R 80
#define BOARD_B 10
#define BOARD_T 110
#define BOARD_W (BOARD_R - BOARD_L)
#define BOARD_H (BOARD_T - BOARD_B)
#define BRICK_W 10
#define BRICK_H 5
#define NBRICK_X (BOARD_W / BRICK_W)
#define NBRICK_Y (BOARD_H / BRICK_H)
#define BALL_R 3
#define BALL_C BLUE
#define SPEED 2
#define TRAY_W 30
#define TRAY_H 4
#define BORDER 4
#define BORDER_C GRAY
#define HSPEED_LIM 4
#define STR_XL 45       // mean(BOARD_L, BOARD_R)
#define STR_XR 106      // mean(BOARD_R + BORDER, 128)
#define STR_Y1 45
#define STR_Y2 55
#define STR_Y3 65
#define STR_Y4 75
#define TRAY_MASS 0.12
#define TRAY_FRICTION 0.1
#include "pattern.h"

#define COLL_LIST_N 10

void draw_brick(int x, int y, color_t color) {
    oled_t bx = BOARD_L + x * BRICK_W;
    oled_t by = BOARD_B + y * BRICK_H;
    draw_rect(bx, by, BRICK_W, BRICK_H, color);
}

void draw_tray(myfloat x, myfloat y, color_t color) {
    oled_t tx = BOARD_L + x - TRAY_W / 2;
    oled_t ty = BOARD_B + y - TRAY_H / 2;
    draw_rect(tx, ty, TRAY_W, TRAY_H, color);
}

void draw_str(oled_t x, oled_t y, char* s, color_t color, char* cache) {
    // draw n chars
    const int n = strlen(s);
    oled_t cx = x - n * CHAR_W / 2;
    oled_t cy = y - CHAR_H / 2;
    if (cache && strlen(cache) == n) {
        int i;
        for (i = 0; i < n; i++, cx += CHAR_W)
            if (s[i] != cache[i])
                draw_char(cx, cy, s[i], color);
    } else {
        int i;
        for (i = 0; i < n; i++, cx += CHAR_W)
            draw_char(cx, cy, s[i], color);
    }
    if (cache)
        strncpy(cache, s, n);
}

// Math functions
// lapacke.h
#define MAX(x,  y)   (((x) > (y)) ? (x) : (y))
#define MIN(x,  y)   (((x) < (y)) ? (x) : (y))

typedef struct { myfloat x; myfloat y; } vect_t;

vect_t normalize(vect_t xy) {
    myfloat n = sqrtf(xy.x*xy.x + xy.y*xy.y);
    return (vect_t){xy.x / n, xy.y / n};
}

myfloat dot_prod(vect_t xy1, vect_t xy2) {
    return xy1.x * xy2.x + xy1.y * xy2.y;
}

vect_t map_mult(myfloat c, vect_t xy) {
    return (vect_t){c * xy.x, c * xy.y};
}

vect_t map_plus(vect_t xy1, vect_t xy2) {
    return (vect_t){xy1.x + xy2.x, xy1.y + xy2.y};
}

vect_t map_minus(vect_t xy1, vect_t xy2) {
    return (vect_t){xy1.x - xy2.x, xy1.y - xy2.y};
}

vect_t map_neg(vect_t xy) {
    return (vect_t){-xy.x, -xy.y};
}

vect_t refl_diff(vect_t xy, vect_t nxy) {
    // nx and ny should be normalized before
    return map_mult(-2 * dot_prod(xy, nxy), nxy);
}

vect_t closest_corner(vect_t rxy, myfloat l, myfloat u, myfloat r, myfloat d) {
    assert(l < r);
    assert(u < d);
    return (vect_t){MIN(MAX(l, rxy.x), r) - rxy.x,
                    MIN(MAX(u, rxy.y), d) - rxy.y};
}

void get_random_perm(int n, int* ans) {
    // Rand permutation in range(1, n + 1)
    int i;
    for (i = 0; i < n; i++) {
        int cnt = get_random_int(n - i);
        int j;
        for (j = 0; j < n; j++) {
            int k;
            for (k = 0; k < i; k++) {
                if (ans[k] == j)
                    goto get_random_perm_found;
            }
            if (!(cnt--)) {
                ans[i] = j;
                goto get_random_perm_decided;
            }
            get_random_perm_found:;
        }
        assert(0);
        get_random_perm_decided:;
    }
}

int init_bricks(int nx, int ny, color_t bricks[nx][ny]) {
    // Initialize bricks, return number of bricks
    int nbricks = 0;
    if (get_random_int(10) < 8) {
        int choice = get_random_int(sizeof(PATTERNS) / sizeof(PATTERNS[0]));
        int perm[NBRICK_COLORS];
        get_random_perm(NBRICK_COLORS, perm);
        int j;
        for (j = 0; j < ny; j++) {
            int line = PATTERNS[choice][j];
            int i;
            for (i = 0; i < nx; i++) {
                int tmp;
                if (j >= 15)
                    bricks[i][j] = BLACK;
                else if (!(tmp = ((line >> (i * 4)) & 0xf)))
                    bricks[i][j] = BLACK;
                else {
                    bricks[i][j] = BRICK_COLORS[perm[tmp - 1]];
                    nbricks++;
                }
            }
        }
    } else {
        int i;
        for (i = 0; i < nx; i++) {
            int j;
            for (j = 0; j < ny; j++) {
                if (j < 15 && get_random_int(10) < 5) {
                    bricks[i][j] = BRICK_COLORS[get_random_int(NBRICK_COLORS)];
                    nbricks++;
                } else {
                    bricks[i][j] = BLACK;
                }
            }
        }
    }
    return nbricks;
}

int game(int prev_score, int max_score) {
    srand(rand() ^ PRCMSlowClkCtrGet());
    int score = prev_score;
    set_rotation_stat();
    vect_t txy = {BOARD_W / 2, BOARD_H};
    myfloat tx_old = txy.x;
    vect_t rxy = {txy.x, txy.y - BALL_R - TRAY_H / 2};
    vect_t vxy = map_mult(SPEED, normalize((vect_t){
                get_random_float() * 3 - 1.5, -1}));
    color_t bricks[NBRICK_X][NBRICK_Y];
    int nbricks = init_bricks(NBRICK_X, NBRICK_Y, bricks);
    no_auto_render = 1;
    draw_rect(0, 0, 128, 128, BLACK);
    draw_rect(BOARD_L - BORDER, BOARD_B, BORDER, BOARD_H + BORDER, BORDER_C);
    draw_rect(BOARD_R, BOARD_B, BORDER, BOARD_H + BORDER, BORDER_C);
    draw_rect(BOARD_L - BORDER, BOARD_B - BORDER, BOARD_W + 2 * BORDER, BORDER,
                BORDER_C);
    draw_tray(txy.x, txy.y, TRAY_C);
    int i, j;
    for (i = 0; i < NBRICK_X; i++)
        for (j = 0; j < NBRICK_Y; j++)
            if (bricks[i][j])
                draw_brick(i, j, bricks[i][j]);
    char score_str[20], score_cache[20] = "";
    draw_str(STR_XR, STR_Y1, "SCORE", CHAR_C, NULL);
    sprintf(score_str, "%d", score);
    draw_str(STR_XR, STR_Y2, score_str, CHAR_C, score_cache);
    draw_str(STR_XR, STR_Y3, "MAX", CHAR_C, NULL);
    sprintf(score_str, "%d", max_score);
    draw_str(STR_XR, STR_Y4, score_str, CHAR_C, NULL);
    render(0, 0, 128, 128);
    no_auto_render = 0;
    int paused = 0;
    int count_down = 49;
    while (1) {
        vect_t old_rxy = rxy;
        if (!paused && !count_down) {
            // Calculate new ball position and brick collision etc
            const int xs = floorf((rxy.x - BALL_R) / BRICK_W);
            const int xe = floorf((rxy.x + BALL_R) / BRICK_W);
            const int ys = floorf((rxy.y - BALL_R) / BRICK_H);
            const int ye = floorf((rxy.y + BALL_R) / BRICK_H);
            int refl_x = 0;
            int refl_y = 0;
            int refl_tray = 0;
            int coll_list_x[COLL_LIST_N], coll_list_y[COLL_LIST_N];
            int coll_list_n = 0;
            // Collision with tray
            // Requires |v| < TRAY_H / 2
            vect_t dxy = closest_corner(rxy, txy.x - TRAY_W/2, txy.y - TRAY_H/2,
                                            txy.x + TRAY_W/2, txy.y + TRAY_H/2);
            if (dxy.x * dxy.x + dxy.y * dxy.y < BALL_R * BALL_R) {
                refl_tray = 1;
                refl_y = 1;
            }
            // Collision with border and brick
            int i;
            for (i = xs; i <= xe; i++) {
                int j;
                for (j = ys; j <= ye; j++) {
                    if (i < 0 || i >= NBRICK_X) {
                        refl_x = 1;
                        continue;
                    }
                    if (j < 0) {
                        refl_y = 1;
                        continue;
                    }
                    if (j >= NBRICK_Y) {
                        if (refl_tray)
                            continue;
                        return score;
                    }
                    if (!bricks[i][j])
                        continue;
                    vect_t dxy = closest_corner(rxy, i * BRICK_W, j * BRICK_H,
                                            (i+1) * BRICK_W, (j+1) * BRICK_H);
                    if (dxy.x * dxy.x + dxy.y * dxy.y > BALL_R * BALL_R)
                        continue;
                    bricks[i][j] = 0;
                    nbricks--;
                    score++;
                    draw_brick(i, j, BLACK);
                    if (dxy.y == 0)
                        refl_x = 1;
                    else if (dxy.x == 0)
                        refl_y = 1;
                    else {
                        coll_list_x[coll_list_n] = i;
                        coll_list_y[coll_list_n++] = j;
                        assert(coll_list_n < COLL_LIST_N);
                    }
                }
            }
            if (refl_x)
                vxy.x = -vxy.x;
            if (refl_y)
                vxy.y = -vxy.y;
            if (!refl_x && !refl_y && coll_list_n) {
                // Require diameter of ball < brick
                vect_t total_diff = {0, 0};
                int index;
                for (index = 0; index < coll_list_n; index++) {
                    int i = coll_list_x[index], j = coll_list_y[index];
                    dxy = closest_corner(rxy, i * BRICK_W, j * BRICK_H,
                                        (i+1) * BRICK_W, (j+1) * BRICK_H);
                    dxy = normalize(dxy);
                    total_diff = map_plus(total_diff, refl_diff(vxy, dxy));
                }
                // print(total_diff, coll_list)
                vxy = map_mult(SPEED, normalize(map_plus(vxy, total_diff)));
            }
            if (refl_tray && !refl_x) {
                // Add tray velocity to vx
                vxy.x += (txy.x - tx_old - vxy.x) * TRAY_FRICTION;
                vxy = map_mult(SPEED, normalize(vxy));
            }
            if (fabsf(vxy.y) * HSPEED_LIM < fabsf(vxy.x)) {
                // Prevent just moving horizontally
                #define SGN(x) ((x) > 0 ? 1 : -1)
                vxy = map_mult(SPEED, normalize((vect_t){
                        SGN(vxy.x) * HSPEED_LIM, SGN(vxy.y)}));
            }
            if (refl_tray && vxy.y > 0) {
                // Prevent sticking onto the tray
                vxy.y = -vxy.y;
            }
            rxy = map_plus(rxy, vxy);
        }
        if (!paused && count_down) {
            // Count down
            switch(count_down) {
            case 49:
                draw_str(STR_XL, STR_Y2, "3", CHAR_C, NULL);
                break;
            case 33:
                draw_str(STR_XL, STR_Y2, "2", CHAR_C, NULL);
                break;
            case 17:
                draw_str(STR_XL, STR_Y2, "1", CHAR_C, NULL);
                break;
            case 1:
                draw_str(STR_XL, STR_Y2, "0", CHAR_C, NULL);
                break;
            // no default
            }
        }
        if (!paused) {
            // Draw update
            sprintf(score_str, "%d", score);
            draw_str(STR_XR, STR_Y2, score_str, CHAR_C, score_cache);
            int txy_x = txy.x, txy_y = txy.y, tx_old_i = tx_old;
            if (tx_old < txy.x) {
                draw_rect(BOARD_L + tx_old_i - TRAY_W / 2,
                        BOARD_B + txy.y - TRAY_H / 2, txy_x - tx_old_i,
                        TRAY_H, BLACK);
                draw_rect(BOARD_L + tx_old_i + TRAY_W / 2,
                        BOARD_B + txy.y - TRAY_H / 2, txy_x - tx_old_i,
                        TRAY_H, TRAY_C);
            }
            if (tx_old > txy.x) {
                draw_rect(BOARD_L + txy_x + TRAY_W / 2,
                        BOARD_B + txy.y - TRAY_H / 2, tx_old_i - txy_x,
                        TRAY_H, BLACK);
                draw_rect(BOARD_L + txy_x - TRAY_W / 2,
                        BOARD_B + txy.y - TRAY_H / 2, tx_old_i - txy_x,
                        TRAY_H, TRAY_C);
            }
            oled_t ux1, ux2, uy1, uy2;
            if (count_down == 1) {
                ux1 = STR_XL - BOARD_L - CHAR_W / 2;
                ux2 = STR_XL - BOARD_L + CHAR_W / 2;
                uy1 = STR_Y2 - BOARD_B - CHAR_H / 2;
                uy2 = STR_Y2 - BOARD_B + CHAR_H / 2;
            } else {
                ux1 = (oled_t) MIN(old_rxy.x, rxy.x) - BALL_R;
                ux2 = (oled_t) MAX(old_rxy.x, rxy.x) + BALL_R;
                uy1 = (oled_t) MIN(old_rxy.y, rxy.y) - BALL_R;
                uy2 = (oled_t) MAX(old_rxy.y, rxy.y) + BALL_R;
            }
            oled_t x;
            for (x = ux1; x <= ux2; x++) {
                oled_t y;
                for (y = uy1; y <= uy2; y++) {
                    int i = x / BRICK_W;
                    int j = y / BRICK_H;
                    color_t color;
                    if (txy_x - TRAY_W / 2 <= x && txy_x + TRAY_W / 2 > x &&
                        txy_y - TRAY_H / 2 <= y && txy_y + TRAY_H / 2 > y) {
                        color = TRAY_C;
                    } else if (x < 0 || x >= BOARD_W || y < 0) {
                        color = BORDER_C;
                    } else if ((x - rxy.x) * (x - rxy.x) + \
                                (y - rxy.y) * (y - rxy.y) < BALL_R * BALL_R) {
                        color = BALL_C;
                    } else if (y >= BOARD_H) {
                        color = BLACK;
                    } else {
                        color = bricks[i][j];
                    }
                    draw_point(BOARD_L + x, BOARD_B + y, color);
                }
            }
            render(BOARD_L + ux1, BOARD_B + uy1, ux2 - ux1 + 1, uy2 - uy1 + 1);
            if (nbricks == 0)
                return -score;
            myfloat tv;
            if (count_down)
                tv = 0;
            else
                tv = get_i2c() * TRAY_MASS;
            tx_old = txy.x;
            txy.x = MIN(MAX(txy.x + tv, TRAY_W/2), BOARD_W - TRAY_W/2);
        }
        if (!paused && count_down)
            count_down--;
        if (get_SW2()) {
            paused = !paused;
        }
        if (get_SW3()) {
            return score;
        }
        // Report("[SW2=%d SW3=%d]", read_SW2(), read_SW3());
        clock_tick(16);
    }
    return prev_score;
}

void project_main() {
    int max_score = get_max_score();
    while (1) {
        int score = 0;
        do {
            score = game(-score, max_score);
        } while(score < 0);
        draw_str(STR_XL, STR_Y1, "Game Over", CHAR_C, NULL);
        draw_str(STR_XL, STR_Y3, "Sending", CHAR_C, NULL);
        draw_str(STR_XL, STR_Y4, "score", CHAR_C, NULL);
        if (score > max_score) {
            draw_str(STR_XL, STR_Y2, "HIGH SCORE!", HIGH_C, NULL);
            max_score = score;
        }
        send_score(score, max_score);
        draw_str(STR_XL, STR_Y3, "Press SW2/SW3", CHAR_C, NULL);
        draw_str(STR_XL, STR_Y4, "to restart", CHAR_C, NULL);
        while (!get_SW2() && !get_SW3());
    }
}
