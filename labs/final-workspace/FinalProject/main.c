#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <cJSON.h>
#include <math.h>
#include <time.h>
#include <limits.h>

// Simplelink includes
#include "simplelink.h"

//Driverlib includes
#include "hw_types.h"
#include "hw_ints.h"
#include "rom.h"
#include "rom_map.h"
#include "interrupt.h"
#include "prcm.h"
#include "utils.h"
#include "uart.h"
#include "common.h"
#include "hw_memmap.h"
#include "hw_common_reg.h"
#include "pin.h"
#include "hw_apps_rcm.h"
#include "spi.h"
#include "gpio.h"
#include "timer.h"
#include "Adafruit_GFX.h"
#include "Adafruit_SSD1351.h"
#include "glcdfont.h"
#include "i2c_if.h"

//Common interface includes
#include "pin_mux_config.h"
#include "gpio_if.h"
#include "common.h"
#include "uart_if.h"
#include "timer_if.h"

#define MAX_URI_SIZE 128
#define URI_SIZE MAX_URI_SIZE + 1

#define APPLICATION_NAME        "SSL"
#define APPLICATION_VERSION     "1.1.1.EEC.Winter2024"
#define SERVER_NAME_POST        "a7n35thb5klp9-ats.iot.us-east-2.amazonaws.com"
#define SERVER_NAME_GET         "xcxzt43up4.execute-api.us-east-2.amazonaws.com"
#define GOOGLE_DST_PORT         8443

#define SL_SSL_CA_CERT "/cert/rootCA.der" //starfield class2 rootca (from firefox) // <-- this one works
#define SL_SSL_CA_CERT2 "/cert/awsCA.der"
#define SL_SSL_PRIVATE "/cert/private.der"
#define SL_SSL_CLIENT  "/cert/client.der"


//NEED TO UPDATE THIS FOR IT TO WORK!
#define DATE                22    /* Current Date */
#define MONTH               2     /* Month 1-12 */
#define YEAR                2024  /* Current year */
#define HOUR                12    /* Time - hours */
#define MINUTE              3    /* Time - minutes */
#define SECOND              0     /* Time - seconds */

#define POSTHEADER "POST /things/dillon_CC3200_Board/shadow HTTP/1.1\r\n"
#define GETHEADER "GET /GetMap HTTP/1.1\r\n"
#define HOSTHEADER "Host: a7n35thb5klp9-ats.iot.us-east-2.amazonaws.com\r\n"
#define GETHOSTHEADER "Host: xcxzt43up4.execute-api.us-east-2.amazonaws.com\r\r"
#define CHEADER "Connection: Keep-Alive\r\n"
#define CTHEADER "Content-Type: application/json; charset=utf-8\r\n"
#define CLHEADER1 "Content-Length: "
#define CLHEADER2 "\r\n\r\n"
#define MAPGET "{\"state\": {\"map\": {\"newMap\": '1'}}}"

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

#define BLACK           0x0000
#define BLUE            0x001F
#define GREEN           0x07E0
#define CYAN            0x07FF
#define RED             0xF800
#define MAGENTA         0xF81F
#define YELLOW          0xFFE0
#define WHITE           0xFFFF

// Application specific status/error codes
typedef enum {
    // Choosing -0x7D0 to avoid overlap w/ host-driver's error codes
    LAN_CONNECTION_FAILED = -0x7D0,
    INTERNET_CONNECTION_FAILED = LAN_CONNECTION_FAILED - 1,
    DEVICE_NOT_IN_STATION_MODE = INTERNET_CONNECTION_FAILED - 1,

    STATUS_CODE_MAX = -0xBB8
}e_AppStatusCodes;

typedef struct {
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

typedef struct Point {
    int x;
    int y;
} Point;

typedef struct
{
    Point point;
    int8_t gScore;
    int8_t fScore;
    Point cameFrom;
} Node;

typedef struct Letter {
    unsigned int x;
    unsigned int y;
    char letter;
} Letter;

typedef struct Mapping {
    //0 is path, 1 is OOB
    uint64_t map[128][2];
    Point start;
    Point end;
} Mapping;

Letter compMessage[100];
Letter reciMessage[100];
Letter prevMessage[100];
// Mapping mapData;

//*****************************************************************************
//                 GLOBAL VARIABLES -- Start
//*****************************************************************************
volatile unsigned long  g_ulStatus = 0;//SimpleLink Status
//unsigned char  *g_caCert = SL_SSL_CA_CERT;
unsigned long  g_ulPingPacketsRecv = 0; //Number of Ping Packets received
unsigned long  g_ulGatewayIP = 0; //Network Gateway IP address
unsigned char  g_ucConnectionSSID[SSID_LEN_MAX+1]; //Connection SSID
unsigned char  g_ucConnectionBSSID[BSSID_LEN_MAX]; //Connection BSSID
signed char    *g_Host = SERVER_NAME_POST;
volatile long lRetVal = -1;
char compString[200];
// char reciBuffer[1460];
SlDateTime g_time;

volatile int count = 0;
volatile int flag = 0;
volatile unsigned long temp = 0;
volatile int current = 0;
volatile int previous = 0;
volatile int same = 0;
volatile int pressed = 0;
unsigned long buffer[100];
volatile int butPressedCount = 0;
volatile int userObtained = 0;
volatile int msgReadyFlag = 0;
volatile int prevSize = 0;
volatile int compSize = 0;
volatile int reciSize = 0;
volatile int comp_x = 5, comp_y = 68;
volatile int reci_x = 5, reci_y = 4;
volatile float score = 0.0;
//volatile int xPos = 64, yPos = 64;
volatile int goalXPos, goalYPos;
volatile int startXPos, startYPos;
volatile bool getProcess = 0;

#if defined(ccs) || defined(gcc)
    extern void (* const g_pfnVectors[])(void);
#endif
#if defined(ewarm)
    extern uVectorEntry __vector_table;
#endif
//*****************************************************************************
//                 GLOBAL VARIABLES -- End: df
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
static int http_post(int);
//static int http_get(int);
void IRHandler();
void Display(unsigned long value);
unsigned long Decode(unsigned long* buffer);
char LetterCalc(unsigned long value);
char AdvanceLetter(char letter, unsigned long value);

//*****************************************************************************
// SimpleLink Asynchronous Event Handlers -- Start
//*****************************************************************************

void SimpleLinkWlanEventHandler(SlWlanEvent_t *pWlanEvent) {
    if(!pWlanEvent) {
        return;
    }

    switch(pWlanEvent->Event) {
        case SL_WLAN_CONNECT_EVENT: {
            SET_STATUS_BIT(g_ulStatus, STATUS_BIT_CONNECTION);

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
                UART_PRINT("Device disconnected from the AP AP: %s, "
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


void SimpleLinkHttpServerCallback(SlHttpServerEvent_t *pHttpEvent, SlHttpServerResponse_t *pHttpResponse) {
    // Unused in this application
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


//*****************************************************************************
// SimpleLink Asynchronous Event Handlers -- End breadcrumb: s18_df
//*****************************************************************************

static long InitializeAppVariables() {
    g_ulStatus = 0;
    g_ulGatewayIP = 0;
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

static void BoardInit(void) {
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
    unsigned int uiCipher = SL_SEC_MASK_TLS_ECDHE_RSA_WITH_AES_128_CBC_SHA256;
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

//    const char* g_Host_UART = g_Host;

    if(lRetVal >= 0) {
        UART_PRINT("Device has connected to the website:");
        UART_PRINT(SERVER_NAME_POST);
        UART_PRINT("\n\r");
    }
    else if(lRetVal == SL_ESECSNOVERIFY) {
        UART_PRINT("Device has connected to the website (UNVERIFIED):");
        UART_PRINT(SERVER_NAME_POST);
        UART_PRINT("\n\r");
    }
    else if(lRetVal < 0) {
        UART_PRINT("Device couldn't connect to server:");
        UART_PRINT(SERVER_NAME_POST);
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

    UART_PRINT("Connection established w/ AP and IP is acquired \n\r");
    return 0;
}

//Start of IR Functions

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
        //Timer_IF_Start(TIMERA1_BASE, TIMER_A, 200);
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
            if(compSize > 0) {
                int i;
                char name[20];
                char scoreStr[20];
                sprintf(scoreStr, "%f", score);
                char header1[] = "{\"state\": {\"desired\": {\"scoreData\": {\"username\": \"";
                char header2[] = "\", \"score\": ";
                char header3[] = "}}}}";
                for(i = 0; i < 20; i++) {
                    name[i] = '\0';
                }
                for(i = 0; i < compSize; i++) {
                    name[i] = compMessage[i].letter;
                    drawChar(compMessage[i].x, compMessage[i].y, compMessage[i].letter, BLACK, BLACK, 1);
                }
                strcpy(compString, header1);
                strcat(compString, name);
                strcat(compString, header2);
                strcat(compString, scoreStr);
                strcat(compString, header3);
                compSize = 0;
                comp_x = 5;
                comp_y = 68;
            }
            //http_post(lRetVal);
            userObtained = 1;
        }
        else if(LetterCalc(current) == '\0') {
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
            letter = 'A';
            break;
        case THREE:
            letter = 'D';
            break;
        case FOUR:
            letter = 'G';
            break;
        case FIVE:
            letter = 'J';
            break;
        case SIX:
            letter = 'M';
            break;
        case SEVEN:
            letter = 'P';
            break;
        case EIGHT:
            letter = 'T';
            break;
        case NINE:
            letter = 'W';
            break;
        case MUTE:
            letter = '-';
            break;
        case LAST:
            letter = '+';
            break;
        default:
            letter = '\0';
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
            if(letter == 'C') { newLetter = 'A'; }
            else { newLetter = letter + 1; }
            break;
        case THREE:
            if(letter == 'F') { newLetter = 'D'; }
            else { newLetter = letter + 1; }
            break;
        case FOUR:
            if(letter == 'I') { newLetter = 'G'; }
            else {newLetter = letter + 1; }
            break;
        case FIVE:
            if(letter == 'L') { newLetter = 'J'; }
            else { newLetter = letter + 1; }
            break;
        case SIX:
            if(letter == 'O') { newLetter = 'M'; }
            else { newLetter = letter + 1; }
            break;
        case SEVEN:
            if(letter == 'S') { newLetter = 'P'; }
            else { newLetter = letter + 1; }
            break;
        case EIGHT:
            if(letter == 'V') { newLetter = 'T'; }
            else { newLetter = letter + 1; }
            break;
        case NINE:
            if(letter == 'Z') { newLetter = 'W'; }
            else { newLetter = letter + 1; }
            break;
        default:
            newLetter = letter;
            break;
    }
    return newLetter;
}

unsigned long Decode(unsigned long* buffer) {
    unsigned long value = 0;
    int i;
    for(i = 0; i < 16; i++) {
        value += *(buffer + i) << (15 - i);
    }
    return value;
}

//Start of SPI functions
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

#define BUF_SIZE               6500
#define HOST_NAME              "192.168.137.102" // The server's IP address
#define HOST_NAME1 192
#define HOST_NAME2 168
#define HOST_NAME3 137
#define HOST_NAME4 102
#define HOST_PORT              5000              // The server's port

int connectToServer(const char* host, int port) {
    _i16 sockfd;
    SlSockAddrIn_t servaddr = {0};

    // Create socket
    sockfd = sl_Socket(SL_AF_INET, SL_SOCK_STREAM, 0);
    if (sockfd < 0) {
        return -1; // Error creating socket
    }

    // Set server address
    servaddr.sin_family = SL_AF_INET;
    servaddr.sin_port = sl_Htons((unsigned short)port);
    servaddr.sin_addr.s_addr = sl_Htonl(SL_IPV4_VAL(HOST_NAME1,HOST_NAME2,HOST_NAME3,HOST_NAME4)); // IP converted to correct format

    // Connect to server
    if (sl_Connect(sockfd, (SlSockAddr_t*)&servaddr, sizeof(servaddr)) < 0) {
        sl_Close(sockfd);
        return -1; // Error connecting to server
    }

    return sockfd; // Return the socket descriptor
}

int closeConnection(int sockfd) {
    return sl_Close(sockfd);
}

int sendData(int sockfd, const char* httpRequest) {
    return sl_Send(sockfd, httpRequest, strlen(httpRequest), 0);
}
int receiveData(int sockfd, char* buffer, int bufferSize) {
    int bytesRead, totalBytesRead = 0;
    do {
        bytesRead = sl_Recv(sockfd, buffer + totalBytesRead, bufferSize - totalBytesRead, 0);
        if (bytesRead <= 0) {
            break; // Connection closed or error
        }
        totalBytesRead += bytesRead;
    } while (totalBytesRead < bufferSize - 1);
    buffer[totalBytesRead] = '\0'; // Ensure null-termination

    return totalBytesRead;
}

int parseJSONWithCJSON(const char* jsonData, Mapping* outMapping) {
    cJSON *root = cJSON_Parse(jsonData);
    if (root == NULL) {
        fprintf(stderr, "Error before: %s\n", cJSON_GetErrorPtr());
        return -1;
    }

    // Parsing "start"
    cJSON *start = cJSON_GetObjectItemCaseSensitive(root, "start");
    if (cJSON_IsObject(start)) {
        cJSON *xStr = cJSON_GetObjectItemCaseSensitive(start, "x");
        cJSON *yStr = cJSON_GetObjectItemCaseSensitive(start, "y");
        if (cJSON_IsString(xStr) && xStr->valuestring != NULL && cJSON_IsString(yStr) && yStr->valuestring != NULL) {
            outMapping->start.x = (int)atoi(xStr->valuestring);
            outMapping->start.y = (int)atoi(yStr->valuestring);

            Report("Start Point: x = %u, y = %u\n", (unsigned)outMapping->start.x, (unsigned)outMapping->start.y);
        }
    }

    // Parsing "end" with string to uint8_t conversion
    cJSON *end = cJSON_GetObjectItemCaseSensitive(root, "end");
    if (cJSON_IsObject(end)) {
        cJSON *xStr = cJSON_GetObjectItemCaseSensitive(end, "x");
        cJSON *yStr = cJSON_GetObjectItemCaseSensitive(end, "y");
        if (cJSON_IsString(xStr) && xStr->valuestring != NULL && cJSON_IsString(yStr) && yStr->valuestring != NULL) {
            outMapping->end.x = (int)atoi(xStr->valuestring);
            outMapping->end.y = (int)atoi(yStr->valuestring);
        }
    }

    // Parsing "map"
    cJSON *map = cJSON_GetObjectItemCaseSensitive(root, "map");
    if (cJSON_IsArray(map)) {
        int i,j;
        for (i = 0; i < cJSON_GetArraySize(map) && i < 128; ++i) { // Assuming there are 2 rows
            cJSON *row = cJSON_GetArrayItem(map, i);
            if (cJSON_IsArray(row)) {
                for (j = 0; j < cJSON_GetArraySize(row) && j < 2; ++j) { // Assuming each row has 128 elements
                    cJSON *cell = cJSON_GetArrayItem(row, j);
                    if (cJSON_IsString(cell) && cell->valuestring != NULL) {
                        // Convert the string to a uint64_t value
                        uint64_t value = strtoull(cell->valuestring, NULL, 10);
                        outMapping->map[i][j] = value;
                    }
                }
            }
        }
    }

    cJSON_Delete(root);
    return 0;
}

int fetchAndParseData(Mapping *mapData) {
//    Mapping mapData = {0};
    char recvBuf[BUF_SIZE];
    //char recvBuf[4000];

    // Connect to the server
    int sockfd = connectToServer(HOST_NAME, HOST_PORT);
    if (sockfd < 0) {
        // Error handling
        return -1;
    }

    // Send HTTP GET request
    const char* getRequest = "GET /GetMap HTTP/1.1\r\nHost: 192.168.137.102\r\nConnection: close\r\n\r\n";
    if (sendData(sockfd, getRequest) < 0) {
        // Error handling
        closeConnection(sockfd);
        return -1;
    }

    // Receive response
    if (receiveData(sockfd, recvBuf, BUF_SIZE) < 0) {
        // Error handling
        closeConnection(sockfd);
        return -1;
    }

    // Close the connection
    closeConnection(sockfd);


    // Parse JSON data (Assuming the JSON starts at the first '{' character for simplicity)
    char* jsonData = strchr(recvBuf, '{');
    int ret;
    if (jsonData != NULL) {
        ret = parseJSONWithCJSON(jsonData, mapData);
        if (ret == -1) return -1;
    } else {
        // Error handling
        return -1;
    }


    return 0;
}

bool getMapValue(const Mapping* mapping, int x, int y) {
    if (x < 0 || x >= 128 || y < 0 || y >= 128) {
        // Coordinate out of bounds
        return -1;  // Or handle as per your error handling strategy
    }

    int uint64_index = y / 64;  // Determine which uint64_t to access (0 or 1)
    int bit_index = y % 64;     // Determine the bit position within the uint64_t

    uint64_t bit_mask = 1ULL << bit_index;  // Create a mask to isolate the desired bit
    int value = (mapping->map[x][uint64_index] & bit_mask) ? 1 : 0;

    return value;
}

void main() {
    unsigned long ulStatus;
    count = 0;
    flag = 0;
    current = 0;
    previous = 0;
    int lapFlag = 1;
    int laps = 0;
    int finishFlag = 0;
    int xSpeed = 0, ySpeed = 0;
    int ballSize = 4;
    int xPos = 64, yPos = 64;
    int i = 0, j = 0;
    char cTemp;
    unsigned char ucRegOffset_base = BASE_OFFSET; // set register offset
    unsigned char aucRdDataBuf[256]; // data buffer

    //Init Board
    BoardInit();
    PinMuxConfig();

    //Init UART
    InitTerm();
    ClearTerm();

    // Init the OLED
    MasterMain();
    fillScreen(BLACK);

    // I2C Init
    I2C_IF_Open(I2C_MASTER_MODE_FST);

    // Init the GPIO for the IR sensor
    GPIOIntRegister(GPIOA0_BASE, GPIOIntHandler);
    GPIOIntTypeSet(GPIOA0_BASE, 0x80, GPIO_FALLING_EDGE);
    ulStatus = GPIOIntStatus (GPIOA0_BASE, false);
    GPIOIntClear(GPIOA0_BASE, ulStatus);

    // Init the Timers for the IR sensor input
    Timer_IF_Init(PRCM_TIMERA3, TIMERA3_BASE, TIMER_CFG_PERIODIC_UP, TIMER_A, 0);

    Timer_IF_Init(PRCM_TIMERA2, TIMERA2_BASE, TIMER_CFG_ONE_SHOT, TIMER_A, 0);
    Timer_IF_IntSetup(TIMERA2_BASE, TIMER_A, TimeoutHandler);

    Timer_IF_Init(PRCM_TIMERA1, TIMERA1_BASE, TIMER_CFG_ONE_SHOT, TIMER_A, 0);
    Timer_IF_IntSetup(TIMERA1_BASE, TIMER_A, RepeatHandler);

    Timer_IF_Init(PRCM_TIMERA0, TIMERA0_BASE, TIMER_CFG_PERIODIC_UP, TIMER_A, 0);
    TimerEnable(TIMERA0_BASE, TIMER_A);
    TimerValueSet(TIMERA0_BASE, TIMER_A, 0);

    //Connect the CC3200 to the local access point
    lRetVal = connectToAccessPoint();

    //Set time so that encryption can be used
    lRetVal = set_time();
    if(lRetVal < 0) {
        UART_PRINT("Unable to set time in the device");
        LOOP_FOREVER();
    }

    //Connect to the website with TLS encryption
    lRetVal = tls_connect();
    if(lRetVal < 0) {
        ERR_PRINT(lRetVal);
    }

    Mapping map = {0};
    int test = -1;

    xPos = 64;
    yPos = 64;

    while(1){
        fillScreen(BLACK);
        drawChar( 5, 20, 'P', RED, BLACK, 2);
        drawChar(15, 20, 'r', RED, BLACK, 2);
        drawChar(25, 20, 'e', RED, BLACK, 2);
        drawChar(35, 20, 's', RED, BLACK, 2);
        drawChar(45, 20, 's', RED, BLACK, 2);
        drawChar(65, 20, 'S', RED, BLACK, 2);
        drawChar(75, 20, 'W', RED, BLACK, 2);
        drawChar(85, 20, '3', RED, BLACK, 2);
        drawChar( 5, 40, 't', RED, BLACK, 2);
        drawChar(15, 40, 'o', RED, BLACK, 2);
        drawChar(35, 40, 's', RED, BLACK, 2);
        drawChar(45, 40, 't', RED, BLACK, 2);
        drawChar(55, 40, 'a', RED, BLACK, 2);
        drawChar(65, 40, 'r', RED, BLACK, 2);
        drawChar(75, 40, 't', RED, BLACK, 2);
        while (GPIOPinRead(GPIOA1_BASE, 0x20) == 0) { ; }
        fillScreen(BLACK);
        fillCircle(xPos, yPos, ballSize, 0xF800);
        TimerValueSet(TIMERA3_BASE, TIMER_A, 0);
        while(finishFlag == 0){
            if (lapFlag == 1){
                while (test == -1) {test = fetchAndParseData(&map);}
                fillScreen(BLACK);
                for (i = 0; i < 128; i++) {
                    for (j = 0; j < 128; j++) {
                        if (getMapValue(&map, i, j)) {
                            drawPixel(i, j, GREEN);
                        }
                    }
                }
                TimerEnable(TIMERA3_BASE, TIMER_A);
                lapFlag = 0;
                startXPos = map.start.x;
                startYPos = map.start.y;
                printf("Start: %d %d", map.start.x, map.start.y);
                xPos = startXPos;
                yPos = startYPos;
                goalXPos = map.end.x;
                goalYPos = map.end.y;
                printf("End: %d %d", map.end.x, map.end.y);
                drawCircle(goalXPos, goalYPos, 7, BLUE);
            }

            //Get x and y accel values
            I2C_IF_Write(BMA222_ADDRESS, &ucRegOffset_base,1,0);
            I2C_IF_Read(BMA222_ADDRESS, &aucRdDataBuf[0], 4);

            // Get X acceleration value, decoding two's complement
            cTemp = (char)(aucRdDataBuf[1]);
            xSpeed = (int)cTemp;
            if(xSpeed > 127)
                xSpeed = xSpeed-256;

            // Get Y acceleration value, decoding two's complement
            cTemp = (char)(aucRdDataBuf[3]);
            ySpeed = (int)cTemp;
            if(ySpeed > 127)
                ySpeed = ySpeed-256;

            fillCircle(xPos, yPos, ballSize, 0x0000);

            //Move ball based on speed
            yPos += xSpeed/5;
            xPos += ySpeed/5;
            //Wall bounds
            if (xPos <= ballSize){
                xPos = ballSize;
            }
            if(xPos >= 127 - ballSize){
                xPos = 127 - ballSize;
            }
            if(yPos <= ballSize){
                yPos = ballSize;
            }
            if(yPos >= 127 - ballSize){
                yPos = 127 - ballSize;
            }

            //Reset to start if ball hovers over 0 and redraw section where fallen
            if(getMapValue(&map, xPos, yPos) == 1) {
                fillCircle(xPos, yPos, ballSize, 0x0000);
                for (i = xPos - 5; i <= xPos + 5; i++) {
                    for (j = yPos - 5; j <= yPos + 5; j++) {
                        if (getMapValue(&map, i, j) == 1) {
                            drawPixel(i, j, GREEN);
                        }
                        else {
                            drawPixel(i, j, BLACK);
                        }
                    }
                }
                xPos = startXPos;
                yPos = startYPos;
            }

            //if in goal, increment lap by 1 and raise lapFlag
            if((xPos + ballSize > goalXPos && xPos - ballSize < goalXPos) && (yPos + ballSize > goalYPos && yPos - ballSize < goalYPos)) {
                fillCircle(goalXPos, goalYPos, 7, BLACK);
                lapFlag = 1;
                laps++;
                test = -1;
//                strcpy(compString, MAPGET);
//                http_post(lRetVal);
                if (laps == 5) {
                    laps = 0;
                    finishFlag = 1;
                }
            }
            else if((xPos + ballSize > goalXPos - 7 && xPos - ballSize < goalXPos + 7) && (yPos + ballSize > goalYPos - 7 && yPos - ballSize < goalYPos + 7)) {
//                Timer_IF_Start(TIMERA1_BASE, TIMER_A, 10);
                drawCircle(goalXPos, goalYPos, 7, BLUE);
            }

            fillCircle(xPos, yPos, ballSize, 0xF800);
        }
        //Calculate Score
        score = TimerValueGet(TIMERA3_BASE, TIMER_A)/80000000.0;
        TimerDisable(TIMERA3_BASE, TIMER_A);
        TimerValueSet(TIMERA3_BASE, TIMER_A, 0);
        GPIOIntEnable(GPIOA0_BASE, 0x80);
        //Get Username
        fillScreen(BLACK);
        drawChar(5, 20, 'E', RED, BLACK, 2);
        drawChar(15, 20, 'n', RED, BLACK, 2);
        drawChar(25, 20, 't', RED, BLACK, 2);
        drawChar(35, 20, 'e', RED, BLACK, 2);
        drawChar(45, 20, 'r', RED, BLACK, 2);
        drawChar(65, 20, 'U', RED, BLACK, 2);
        drawChar(75, 20, 's', RED, BLACK, 2);
        drawChar(85, 20, 'e', RED, BLACK, 2);
        drawChar(95, 20, 'r', RED, BLACK, 2);
        drawChar(105, 20, ':', RED, BLACK, 2);
        while (userObtained == 0) {
            IRHandler();
        }
        if (userObtained == 1) {
            userObtained = 0;
            test = -1;
            GPIOIntDisable(GPIOA0_BASE, 0x80);
            //Combine Strings
            http_post(lRetVal);
            finishFlag = 0;
            fillScreen(BLACK);
        }
    }
}
//*****************************************************************************
//
// Close the Doxygen group.
//! @}
//
//*****************************************************************************

static int http_post(int iTLSSockID){
    char acSendBuff[512];
    char acRecvbuff[1460];
    char cCLLength[200];
    char* pcBufHeaders;
    int lRetVal = 0;

    pcBufHeaders = acSendBuff;
    strcpy(pcBufHeaders, POSTHEADER);
    pcBufHeaders += strlen(POSTHEADER);
    strcpy(pcBufHeaders, HOSTHEADER);
    pcBufHeaders += strlen(HOSTHEADER);
    strcpy(pcBufHeaders, CHEADER);
    pcBufHeaders += strlen(CHEADER);
    strcpy(pcBufHeaders, "\r\n\r\n");

    int dataLength = strlen(compString);

    strcpy(pcBufHeaders, CTHEADER);
    pcBufHeaders += strlen(CTHEADER);
    strcpy(pcBufHeaders, CLHEADER1);

    pcBufHeaders += strlen(CLHEADER1);
    sprintf(cCLLength, "%d", dataLength);

    strcpy(pcBufHeaders, cCLLength);
    pcBufHeaders += strlen(cCLLength);
    strcpy(pcBufHeaders, CLHEADER2);
    pcBufHeaders += strlen(CLHEADER2);

    strcpy(pcBufHeaders, compString);
    pcBufHeaders += strlen(compString);

    int testDataLength = strlen(pcBufHeaders);

    UART_PRINT(acSendBuff);


    //
    // Send the packet to the server */
    //
    lRetVal = sl_Send(iTLSSockID, acSendBuff, strlen(acSendBuff), 0);
    if(lRetVal < 0) {
        UART_PRINT("POST failed. Error Number: %i\n\r",lRetVal);
        sl_Close(iTLSSockID);
        GPIO_IF_LedOn(MCU_RED_LED_GPIO);
        return lRetVal;
    }
    lRetVal = sl_Recv(iTLSSockID, &acRecvbuff[0], sizeof(acRecvbuff), 0);
    if(lRetVal < 0) {
        UART_PRINT("Received failed. Error Number: %i\n\r",lRetVal);
        //sl_Close(iSSLSockID);
        GPIO_IF_LedOn(MCU_RED_LED_GPIO);
           return lRetVal;
    }
    else {
        acRecvbuff[lRetVal+1] = '\0';
        UART_PRINT(acRecvbuff);
        UART_PRINT("\n\r\n\r");
    }

    return 0;
}
