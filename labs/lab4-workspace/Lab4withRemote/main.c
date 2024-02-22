// Standard includes
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

// Simplelink includes
#include "simplelink.h"

// Driverlib includes
#include "hw_types.h"
#include "hw_ints.h"
#include "hw_memmap.h"
#include "hw_common_reg.h"
#include "pin.h"
#include "interrupt.h"
#include "hw_apps_rcm.h"
#include "spi.h"
#include "uart.h"
#include "prcm.h"
#include "rom.h"
#include "rom_map.h"
#include "prcm.h"
#include "gpio.h"
#include "utils.h"
#include "timer.h"
#include "Adafruit_GFX.h"
#include "Adafruit_SSD1351.h"
#include "glcdfont.h"
#include "common.h"

// Common interface includes
#include "uart_if.h"
#include "i2c_if.h"
#include "gpio_if.h"
#include "timer_if.h"

#include "pin_mux_config.h"

//*****************************************************************************
//                      MACRO DEFINITIONS
//*****************************************************************************
#define APPLICATION_VERSION "1.4.0"
#define APP_NAME "I2C Demo"
#define UART_PRINT Report
#define FOREVER 1
#define CONSOLE UARTA0_BASE
#define FAILURE -1
#define SUCCESS 0
#define RETERR_IF_TRUE(condition) \
    {                             \
        if (condition)            \
            return FAILURE;       \
    }
#define RET_IF_ERR(Func)        \
    {                           \
        int iRetVal = (Func);   \
        if (SUCCESS != iRetVal) \
            return iRetVal;     \
    }

#define APPLICATION_NAME        "SSL"
#define APPLICATION_VERSION     "1.1.1.EEC.Winter2023-2024"
#define SERVER_NAME             "a7n35thb5klp9-ats.iot.us-east-2.amazonaws.com"
#define GOOGLE_DST_PORT         8443
#define SL_SSL_CA_CERT "/cert/rootCA.der" //starfield class2 rootca (from firefox) // <-- this one works
#define SL_SSL_PRIVATE "/cert/private.der"
#define SL_SSL_CLIENT  "/cert/client.der"

//NEED TO UPDATE THIS FOR IT TO WORK!
#define DATE                20    /* Current Date */
#define MONTH               2     /* Month 1-12 */
#define YEAR                2024  /* Current year */
#define HOUR                9    /* Time - hours */
#define MINUTE              52    /* Time - minutes */
#define SECOND              0     /* Time - seconds */

#define POSTHEADER "POST /things/CC3200_Thing/shadow HTTP/1.1\n\r"
#define HOSTHEADER "Host: a7n35thb5klp9-ats.iot.us-east-2.amazonaws.com\r\n"
#define CHEADER "Connection: Keep-Alive\r\n"
#define CTHEADER "Content-Type: application/json; charset=utf-8\r\n"
#define CLHEADER1 "Content-Length: "
#define CLHEADER2 "\r\n\r\n"
#define DATA1 "{\"state\": {\r\n\"desired\" : {\r\n\"var\" : \"Hello phone, message from CC3200 via AWS IoT!\"\r\n}}}\r\n\r\n"

#define BMA222_ADDRESS 0x18
#define BASE_OFFSET 0x2
#define X_OFFSET 0x3
#define Y_OFFSET 0x5

#define ONE   0x00FF
#define TWO   0x807F
#define THREE 0x40BF
#define FOUR  0xC03F
#define FIVE  0x20DF
#define SIX   0xA05F
#define SEVEN 0x609F
#define EIGHT 0xE01F
#define NINE  0x10EF
#define ZERO  0x906F
#define MUTE  0x708F
#define LAST  0x08F7

// Color definitions
#define	BLACK           0x0000
#define	BLUE            0x001F
#define	GREEN           0x07E0
#define CYAN            0x07FF
#define	RED             0xF800
#define MAGENTA         0xF81F
#define YELLOW          0xFFE0
#define WHITE           0xFFFF

typedef struct Letter {
    unsigned int x;
    unsigned int y;
    char letter;
} Letter;

Letter compMessage[100];
Letter reciMessage[100];
Letter prevMessage[100];

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

typedef enum{
    // Choosing -0x7D0 to avoid overlap w/ host-driver's error codes
    LAN_CONNECTION_FAILED = -0x7D0,
    INTERNET_CONNECTION_FAILED = LAN_CONNECTION_FAILED - 1,
    DEVICE_NOT_IN_STATION_MODE = INTERNET_CONNECTION_FAILED - 1,

    STATUS_CODE_MAX = -0xBB8
}e_AppStatusCodes;

//*****************************************************************************
//                 GLOBAL VARIABLES -- Start
//*****************************************************************************
#if defined(ccs)
    extern void (*const g_pfnVectors[])(void);
#endif
#if defined(ewarm)
    extern uVectorEntry __vector_table;
#endif

#define SYSCLKFREQ 80000000ULL

#define TICKS_TO_US(ticks) \
    ((((ticks) / SYSCLKFREQ) * 1000000ULL) + \
    ((((ticks) % SYSCLKFREQ) * 1000000ULL) / SYSCLKFREQ))\

#define US_TO_TICKS(us) ((SYSCLKFREQ / 1000000ULL) * (us))

#define SYSTICK_RELOAD_VAL 3200000UL

volatile int count = 0;
volatile int flag = 0;
volatile unsigned long temp = 0;
volatile int current = 0;
volatile int previous = 0;
volatile int same = 0;
volatile int pressed = 0;
unsigned long buffer[100];
volatile int butPressedCount = 0;
volatile int msgReadyFlag = 0;

volatile int prevSize = 0;
volatile int compSize = 0;
volatile int reciSize = 0;

volatile int comp_x = 5, comp_y = 68;
volatile int reci_x = 5, reci_y = 4;

volatile unsigned long  g_ulStatus = 0;//SimpleLink Status
unsigned long  g_ulPingPacketsRecv = 0; //Number of Ping Packets received
unsigned long  g_ulGatewayIP = 0; //Network Gateway IP address
unsigned char  g_ucConnectionSSID[SSID_LEN_MAX+1]; //Connection SSID
unsigned char  g_ucConnectionBSSID[BSSID_LEN_MAX]; //Connection BSSID
signed char    *g_Host = SERVER_NAME;
SlDateTime g_time;

extern void (* const g_pfnVectors[])(void);
//*****************************************************************************
//                  GLOBAL VARIABLES -- End
//*****************************************************************************

//****************************************************************************
//                   SIMPLELINK FUNCTION DEFINITIONS
//****************************************************************************
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

            UART_PRINT("[WLAN EVENT] STA Connected to the AP: %s , "
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
                UART_PRINT("[WLAN EVENT]Device disconnected from the AP: %s,"
                    "BSSID: %x:%x:%x:%x:%x:%x on application's request \n\r",
                           g_ucConnectionSSID,g_ucConnectionBSSID[0],
                           g_ucConnectionBSSID[1],g_ucConnectionBSSID[2],
                           g_ucConnectionBSSID[3],g_ucConnectionBSSID[4],
                           g_ucConnectionBSSID[5]);
            }
            else {
                UART_PRINT("[WLAN ERROR]Device disconnected from the AP AP: %s, "
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
            UART_PRINT("[WLAN EVENT] Unexpected event [0x%x]\n\r",
                       pWlanEvent->Event);
        }
        break;
    }
}

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

            UART_PRINT("[NETAPP EVENT] IP Acquired: IP=%d.%d.%d.%d , "
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
            UART_PRINT("[NETAPP EVENT] Unexpected event [0x%x] \n\r",
                       pNetAppEvent->Event);
        }
        break;
    }
}

void SimpleLinkGeneralEventHandler(SlDeviceEvent_t *pDevEvent) {
    if(!pDevEvent) {
        return;
    }

    //
    // Most of the general errors are not FATAL are are to be handled
    // appropriately by the application
    //
    UART_PRINT("[GENERAL EVENT] - ID=[%d] Sender=[%d]\n\n",
               pDevEvent->EventData.deviceEvent.status,
               pDevEvent->EventData.deviceEvent.sender);
}

void SimpleLinkSockEventHandler(SlSockEvent_t *pSock) {
    if(!pSock) {
        return;
    }

    switch( pSock->Event ) {
        case SL_SOCKET_TX_FAILED_EVENT:
            switch( pSock->socketAsyncEvent.SockTxFailData.status) {
                case SL_ECLOSE:
                    UART_PRINT("[SOCK ERROR] - close socket (%d) operation "
                                "failed to transmit all queued packets\n\n",
                                    pSock->socketAsyncEvent.SockTxFailData.sd);
                    break;
                default:
                    UART_PRINT("[SOCK ERROR] - TX FAILED  :  socket %d , reason "
                                "(%d) \n\n",
                                pSock->socketAsyncEvent.SockTxFailData.sd, pSock->socketAsyncEvent.SockTxFailData.status);
                  break;
            }
            break;

        default:
            UART_PRINT("[SOCK EVENT] - Unexpected Event [%x0x]\n\n",pSock->Event);
          break;
    }
}

static long InitializeAppVariables() {
    g_ulStatus = 0;
    g_ulGatewayIP = 0;
    g_Host = SERVER_NAME;
    memset(g_ucConnectionSSID,0,sizeof(g_ucConnectionSSID));
    memset(g_ucConnectionBSSID,0,sizeof(g_ucConnectionBSSID));
    return SUCCESS;
}

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

    UART_PRINT("Host Driver Version: %s\n\r",SL_DRIVER_VERSION);
    UART_PRINT("Build Version %d.%d.%d.%d.31.%d.%d.%d.%d.%d.%d.%d.%d\n\r",
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

static long WlanConnect() {
    SlSecParams_t secParams = {0};
    long lRetVal = 0;

    secParams.Key = SECURITY_KEY;
    secParams.KeyLen = strlen(SECURITY_KEY);
    secParams.Type = SECURITY_TYPE;

    UART_PRINT("Attempting connection to access point: ");
    UART_PRINT(SSID_NAME);
    UART_PRINT("... ...");
    lRetVal = sl_WlanConnect(SSID_NAME, strlen(SSID_NAME), 0, &secParams, 0);
    ASSERT_ON_ERROR(lRetVal);

    UART_PRINT(" Connected!!!\n\r");


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

long printErrConvenience(char * msg, long retVal) {
    UART_PRINT(msg);
    GPIO_IF_LedOn(MCU_RED_LED_GPIO);
    return retVal;
}

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

static int tls_connect() {
    SlSockAddrIn_t    Addr;
    int    iAddrSize;
    unsigned char    ucMethod = SL_SO_SEC_METHOD_TLSV1_2;
    unsigned int uiIP;
//    unsigned int uiCipher = SL_SEC_MASK_TLS_ECDHE_RSA_WITH_AES_256_CBC_SHA;
    unsigned int uiCipher = SL_SEC_MASK_TLS_ECDHE_RSA_WITH_AES_128_CBC_SHA256;
// SL_SEC_MASK_SSL_RSA_WITH_RC4_128_SHA
// SL_SEC_MASK_SSL_RSA_WITH_RC4_128_MD5
// SL_SEC_MASK_TLS_RSA_WITH_AES_256_CBC_SHA
// SL_SEC_MASK_TLS_DHE_RSA_WITH_AES_256_CBC_SHA
// SL_SEC_MASK_TLS_ECDHE_RSA_WITH_AES_256_CBC_SHA
// SL_SEC_MASK_TLS_ECDHE_RSA_WITH_RC4_128_SHA
// SL_SEC_MASK_TLS_RSA_WITH_AES_128_CBC_SHA256
// SL_SEC_MASK_TLS_RSA_WITH_AES_256_CBC_SHA256
// SL_SEC_MASK_TLS_ECDHE_RSA_WITH_AES_128_CBC_SHA256
// SL_SEC_MASK_TLS_ECDHE_ECDSA_WITH_AES_128_CBC_SHA256 // does not work (-340, handshake fails)
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



/////////////////////////////////
// START: COMMENT THIS OUT IF DISABLING SERVER VERIFICATION
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
// END: COMMENT THIS OUT IF DISABLING SERVER VERIFICATION
/////////////////////////////////


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

    if(lRetVal >= 0) {
        UART_PRINT("Device has connected to the website:");
        UART_PRINT(SERVER_NAME);
        UART_PRINT("\n\r");
    }
    else if(lRetVal == SL_ESECSNOVERIFY) {
        UART_PRINT("Device has connected to the website (UNVERIFIED):");
        UART_PRINT(SERVER_NAME);
        UART_PRINT("\n\r");
    }
    else if(lRetVal < 0) {
        UART_PRINT("Device couldn't connect to server:");
        UART_PRINT(SERVER_NAME);
        UART_PRINT("\n\r");
        return printErrConvenience("Device couldn't connect to server \n\r", lRetVal);
    }

    GPIO_IF_LedOff(MCU_RED_LED_GPIO);
    GPIO_IF_LedOn(MCU_GREEN_LED_GPIO);
    return iSockID;
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
          UART_PRINT("Failed to configure the device in its default state \n\r");

      return lRetVal;
    }

    UART_PRINT("Device is configured in default state \n\r");

    CLR_STATUS_BIT_ALL(g_ulStatus);

    ///
    // Assumption is that the device is configured in station mode already
    // and it is in its default state
    //
    UART_PRINT("Opening sl_start\n\r");
    lRetVal = sl_Start(0, 0, 0);
    if (lRetVal < 0 || ROLE_STA != lRetVal) {
        UART_PRINT("Failed to start the device \n\r");
        return lRetVal;
    }

    UART_PRINT("Device started as STATION \n\r");

    //
    //Connecting to WLAN AP
    //
    lRetVal = WlanConnect();
    if(lRetVal < 0) {
        UART_PRINT("Failed to establish connection w/ an AP \n\r");
        GPIO_IF_LedOn(MCU_RED_LED_GPIO);
        return lRetVal;
    }

    UART_PRINT("Connection established w/ AP and IP is aquired \n\r");
    return 0;
}

//****************************************************************************
//                      LOCAL FUNCTION DEFINITIONS
//****************************************************************************

void IRHandler();
void Display(unsigned long value);
unsigned long Decode(unsigned long* buffer);
char LetterCalc(unsigned long value);
char AdvanceLetter(char letter, unsigned long value);

static void BoardInit(void) {
    #ifndef USE_TIRTOS
        #if defined(ccs)
            MAP_IntVTableBaseSet((unsigned long)&g_pfnVectors[0]);
        #endif
        #if defined(ewarm)
            MAP_IntVTableBaseSet((unsigned long)&__vector_table);
        #endif
    #endif
    MAP_IntMasterEnable();
    MAP_IntEnable(FAULT_SYSTICK);
    PRCMCC3200MCUInit();
}

static void GPIOIntHandler(void) {
    unsigned long ulStatus;
    ulStatus = GPIOIntStatus(GPIOA0_BASE, true);
    GPIOIntClear(GPIOA0_BASE, ulStatus);
    count++;
    if(count == 35) {
        flag = 1;
        count = 0;
        Timer_IF_Start(TIMERA2_BASE, TIMER_A, 1000);
    }
    temp = TimerValueGet(TIMERA0_BASE, TIMER_A) >> 17;
    if(temp == 58 || temp == 59) {
        flag = 0;
        count = -2;
        Timer_IF_Start(TIMERA1_BASE, TIMER_A, 200);
    }
    buffer[count] = temp;
    if (count == 2 && buffer[count] != 5) {
        count = 0;
    }
    TimerValueSet(TIMERA0_BASE, TIMER_A, 0);
}

static void RepeatHandler(void)
{
    Timer_IF_InterruptClear(TIMERA1_BASE);
}

static void TimeoutHandler(void)
{
    Timer_IF_InterruptClear(TIMERA2_BASE);
    previous = -1;
}

void SendMessage(void) {
    //Send Message
    if(compSize > 0) {
        int i;
        for(i = 0; i < compSize; i++) {
            while(UARTBusy(UARTA1_BASE));
            UARTCharPut(UARTA1_BASE, compMessage[i].letter);
        }
        UARTCharPut(UARTA1_BASE,'\0');
        for(i = 0; i < compSize; i++) {
            drawChar(compMessage[i].x, compMessage[i].y, compMessage[i].letter, BLACK, BLACK, 1);
        }
        compSize = 0;
        comp_x = 5;
        comp_y = 68;
    }
}

void IRHandler(void) {
    if (flag == 1) {
        flag = 0;
        current = Decode(buffer + 19);
        if(previous == current) {
           same = 1;
        }
        else {
           same = 0;
        }
        previous = current;

        if(LetterCalc(current) == '-') {
            if(compSize > 0) {
                compSize--;
                Report("%c", compMessage[compSize]);
                drawChar(compMessage[compSize].x, compMessage[compSize].y, compMessage[compSize].letter, BLACK, BLACK, 1);
            }
            if(comp_x >= 12) {
                comp_x -= 7;
            }
            else if(comp_x == 5) {
                if(comp_y >= 78) {
                    comp_y -= 10;
                    comp_x = 117;
                }
            }
        }

        else if(LetterCalc(current) == '+') {
            SendMessage();
        }
        else {
            char letter;
            letter = LetterCalc(current);
            if(compSize < 100) {
                if(same == 1) {
                    int index = compSize - 1;
                    char let = compMessage[index].letter;

                    drawChar(compMessage[index].x, compMessage[index].y, let, BLACK, BLACK, 1);

                    compMessage[index].letter = AdvanceLetter(let, current);
                    drawChar(compMessage[index].x, compMessage[index].y, compMessage[index].letter, BLUE, BLUE, 1);
                }

                else {
                    Letter newLet;
                    newLet.x = comp_x;
                    newLet.y = comp_y;
                    newLet.letter = letter;
                    compMessage[compSize] = newLet;

                    drawChar(comp_x, comp_y, letter, BLUE, BLUE, 1);

                    comp_x += 7;
                    if(comp_x >= 124) {
                        comp_x = 5;
                        comp_y += 10;
                    }

                    compSize++;
                }
            }
        }
    }
}

void UARTIntHandler(void) {
//    unsigned long ulStatus;
    UARTIntStatus(UARTA1_BASE, true);
//    UARTIntClear(UARTA1_BASE, ulStatus);
    while(UARTCharsAvail(UARTA1_BASE))
    {
        char c = UARTCharGet(UARTA1_BASE);
        if(c == '\0') {
            msgReadyFlag = 1;
        }
        else {
            reciMessage[reciSize].letter = c;
            reciMessage[reciSize].x = reci_x;
            reciMessage[reciSize].y = reci_y;
            reciSize++;
            reci_x += 7;
            if(reci_x >= 124) {
                reci_x = 5;
                reci_y += 10;
            }
        }
    }
}

char LetterCalc(unsigned long value) {
    char letter;
    switch(value) {
        case ZERO:
            letter = ' ';
            break;
        case ONE:
            letter = '?';
            break;
        case TWO:
            letter = 'a';
            break;
        case THREE:
            letter = 'd';
            break;
        case FOUR:
            letter = 'g';
            break;
        case FIVE:
            letter = 'j';
            break;
        case SIX:
            letter = 'm';
            break;
        case SEVEN:
            letter = 'p';
            break;
        case EIGHT:
            letter = 't';
            break;
        case NINE:
            letter = 'w';
            break;
        case MUTE:
            letter = '-';
            break;
        case LAST:
            letter = '+';
            break;
        default:
            letter = ' ';
            break;
    }
    return letter;
}

char AdvanceLetter(char letter, unsigned long value) {
    char newLetter;
    switch(value) {
        case ONE:
            if(letter == '?') { newLetter = '.'; }
            else if(letter == '.') { newLetter = ','; }
            else { newLetter = '?'; }
            break;
        case TWO:
            if(letter == 'c') { newLetter = 'a'; }
            else { newLetter = letter + 1; }
            break;
        case THREE:
            if(letter == 'f') { newLetter = 'd'; }
            else { newLetter = letter + 1; }
            break;
        case FOUR:
            if(letter == 'i') { newLetter = 'g'; }
            else {newLetter = letter + 1; }
            break;
        case FIVE:
            if(letter == 'l') { newLetter = 'j'; }
            else { newLetter = letter + 1; }
            break;
        case SIX:
            if(letter == 'o') { newLetter = 'm'; }
            else { newLetter = letter + 1; }
            break;
        case SEVEN:
            if(letter == 's') { newLetter = 'p'; }
            else { newLetter = letter + 1; }
            break;
        case EIGHT:
            if(letter == 'v') { newLetter = 't'; }
            else { newLetter = letter + 1; }
            break;
        case NINE:
            if(letter == 'z') { newLetter = 'w'; }
            else { newLetter = letter + 1; }
            break;
        default:
            newLetter = letter;
            break;
    }
    return newLetter;
}

void DisplayMessage() {
    if(msgReadyFlag) {
        msgReadyFlag = 0;
        int i;
        for(i = 0; i < prevSize; i++) {
            drawChar(prevMessage[i].x, prevMessage[i].y, prevMessage[i].letter, BLACK, BLACK, 1);
        }
        for(i = 0; i < reciSize; i++) {
            drawChar(reciMessage[i].x, reciMessage[i].y, reciMessage[i].letter, RED, RED, 1);
            prevMessage[i] = reciMessage[i];
        }
        prevSize = reciSize;
        reci_x = 5;
        reci_y = 4;
        reciSize = 0;
    }
}

unsigned long Decode(unsigned long* buffer) {
    unsigned long value = 0;
    int i;
    for(i = 0; i < 16; i++) {
        value += *(buffer + i) << (15 - i);
    }
    return value;
}


// UART APIs to start with
// MAP_UARTConfigSetExpClk(CONSOLE, MAP_PRCMPeripheralClockGet(CONSOLE),
//                         UART_BAUD_RATE, (UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE |
//                         UART_CONFIG_PAR_NONE));
// MAP_UARTIntRegister(CONSOLE, UARTIntHandler);
// // Clear interupts by getting status
// MAP_UARTIntStatus(CONSOLE, true);
// MAP_UARTIntEnable(CONSOLE, UART_INT_RX | UART_INT_RT);
void InitBoardUART(){
    MAP_UARTConfigSetExpClk(UARTA1_BASE,MAP_PRCMPeripheralClockGet(UARTA1_BASE),
                            UART_BAUD_RATE, (UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE |
                            UART_CONFIG_PAR_NONE));
    
    // MAP_UARTIntRegister(UARTA1_BASE, UARTIntHandler);
    // MAP_UARTIntStatus(UARTA1_BASE, true);
    // MAP_UARTIntEnable(UARTA1_BASE, UART_INT_RX | UART_INT_RT);
}
void testhelloworld() {
    fillScreen(BLACK);
    drawChar(5, 64, 'H', RED, BLUE, 2);
    drawChar(15, 64, 'e', RED, BLUE, 2);
    drawChar(25, 64, 'l', RED, BLUE, 2);
    drawChar(35, 64, 'l', RED, BLUE, 2);
    drawChar(45, 64, 'o', RED, BLUE, 2);
    drawChar(65, 64, 'W', RED, BLUE, 2);
    drawChar(75, 64, 'o', RED, BLUE, 2);
    drawChar(85, 64, 'r', RED, BLUE, 2);
    drawChar(95, 64, 'l', RED, BLUE, 2);
    drawChar(105, 64, 'd', RED, BLUE, 2);
    drawChar(115, 64, '!', RED, BLUE, 2);
}

void MasterMain()
{
    int SPI_IF_BIT_RATE = 100000;
    MAP_SPIReset(GSPI_BASE);
    MAP_SPIConfigSetExpClk(GSPI_BASE,MAP_PRCMPeripheralClockGet(PRCM_GSPI),
                      SPI_IF_BIT_RATE,SPI_MODE_MASTER,SPI_SUB_MODE_0,
                      (SPI_SW_CTRL_CS |
                      SPI_4PIN_MODE |
                      SPI_TURBO_OFF |
                      SPI_CS_ACTIVEHIGH |
                      SPI_WL_8));
    MAP_SPIEnable(GSPI_BASE);
    Adafruit_Init();
}

void main()
{
    unsigned long ulStatus;
    count = 0;
    flag = 0;
    current = 0;
    previous = 0;

    // Init the board
    BoardInit();
    PinMuxConfig();

    // Init the UART for Console
    InitTerm();
    ClearTerm();

    // Init the UART for the board
    InitBoardUART();

    // Init the OLED
    MasterMain();
    fillScreen(BLACK);
//    testhelloworld();
    
    // Init the GPIO for the IR sensor
    GPIOIntRegister(GPIOA0_BASE, GPIOIntHandler);
    GPIOIntTypeSet(GPIOA0_BASE, 0x80, GPIO_FALLING_EDGE);
    ulStatus = GPIOIntStatus (GPIOA0_BASE, false);
    GPIOIntClear(GPIOA0_BASE, ulStatus);
    GPIOIntEnable(GPIOA0_BASE, 0x80);

    // Init the Interupt for baord to baord UART
    UARTIntRegister(UARTA1_BASE, UARTIntHandler);
    MAP_UARTIntEnable(UARTA1_BASE, UART_INT_RX | UART_INT_RT);

    Timer_IF_Init(PRCM_TIMERA2, TIMERA2_BASE, TIMER_CFG_ONE_SHOT, TIMER_A, 0);
    Timer_IF_IntSetup(TIMERA2_BASE, TIMER_A, TimeoutHandler);

    Timer_IF_Init(PRCM_TIMERA1, TIMERA1_BASE, TIMER_CFG_ONE_SHOT, TIMER_A, 0);
    Timer_IF_IntSetup(TIMERA1_BASE, TIMER_A, RepeatHandler);

    // Init the Timer for the IR sensor input
    Timer_IF_Init(PRCM_TIMERA0, TIMERA0_BASE, TIMER_CFG_PERIODIC_UP, TIMER_A, 0);
    TimerEnable(TIMERA0_BASE, TIMER_A);
    TimerValueSet(TIMERA0_BASE, TIMER_A, 0);

    while (1)
    {
        IRHandler();
        DisplayMessage();
    }
}
