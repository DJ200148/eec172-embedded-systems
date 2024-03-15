#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
// #include "cJSON.h"
// // #include "simplelink.h"

// #define SL_STOP_TIMEOUT        0xFF
// #define BUF_SIZE               1024
// #define HOST_NAME              "192.168.137.102" // The server's IP address
// #define HOST_PORT              5000              // The server's port

// typedef struct Point {
//     int8_t x;
//     int8_t y;
// } Point;

// typedef struct Mapping {
//     bool map[128][128]; // Assuming this is manually handled due to complexity
//     Point start;
//     Point end;
// } Mapping;

// int connectToServer(const char* host, int port) {
//     _i16 sockfd;
//     SlSockAddrIn_t servaddr = {0};

//     // Create socket
//     sockfd = sl_Socket(SL_AF_INET, SL_SOCK_STREAM, 0);
//     if (sockfd < 0) {
//         return -1; // Error creating socket
//     }

//     // Set server address
//     servaddr.sin_family = SL_AF_INET;
//     servaddr.sin_port = sl_Htons((unsigned short)port);
//     servaddr.sin_addr.s_addr = sl_Htonl(SL_IPV4_VAL(192,168,137,102)); // IP converted to correct format

//     // Connect to server
//     if (sl_Connect(sockfd, (SlSockAddr_t*)&servaddr, sizeof(servaddr)) < 0) {
//         sl_Close(sockfd);
//         return -1; // Error connecting to server
//     }

//     return sockfd; // Return the socket descriptor
// }
// int closeConnection(int sockfd) {
//     return sl_Close(sockfd);
// }
// int sendData(int sockfd, const char* httpRequest) {
//     return sl_Send(sockfd, httpRequest, strlen(httpRequest), 0);
// }
// int receiveData(int sockfd, char* buffer, int bufferSize) {
//     int bytesRead, totalBytesRead = 0;
//     do {
//         bytesRead = sl_Recv(sockfd, buffer + totalBytesRead, bufferSize - totalBytesRead, 0);
//         if (bytesRead <= 0) {
//             break; // Connection closed or error
//         }
//         totalBytesRead += bytesRead;
//     } while (totalBytesRead < bufferSize - 1);
//     buffer[totalBytesRead] = '\0'; // Ensure null-termination

//     return totalBytesRead;
// }
// void parseJSONWithCJSON(const char* jsonData, Mapping* outMapping) {
//     cJSON *root = cJSON_Parse(jsonData);
//     if (root == NULL) {
//         fprintf(stderr, "Error before: %s\n", cJSON_GetErrorPtr());
//         return;
//     }

//     // Parsing "start"
//     cJSON *start = cJSON_GetObjectItemCaseSensitive(root, "start");
//     if (cJSON_IsObject(start)) {
//         outMapping->start.x = cJSON_GetObjectItemCaseSensitive(start, "x")->valueint;
//         outMapping->start.y = cJSON_GetObjectItemCaseSensitive(start, "y")->valueint;
//     }

//     // Parsing "end"
//     cJSON *end = cJSON_GetObjectItemCaseSensitive(root, "end");
//     if (cJSON_IsObject(end)) {
//         outMapping->end.x = cJSON_GetObjectItemCaseSensitive(end, "x")->valueint;
//         outMapping->end.y = cJSON_GetObjectItemCaseSensitive(end, "y")->valueint;
//     }

//     // Parsing "map"
//     cJSON *map = cJSON_GetObjectItemCaseSensitive(root, "map");
//     if (cJSON_IsArray(map)) {
//         for (int i = 0; i < cJSON_GetArraySize(map); ++i) {
//             cJSON *row = cJSON_GetArrayItem(map, i);
//             if (cJSON_IsArray(row)) {
//                 for (int j = 0; j < cJSON_GetArraySize(row); ++j) {
//                     cJSON *cell = cJSON_GetArrayItem(row, j);
//                     outMapping->map[i][j] = cell->valueint ? true : false;
//                 }
//             }
//         }
//     }

//     cJSON_Delete(root);
// }
// int fetchAndParseData() {
//      Mapping mapData = {0};
//     char recvBuf[BUF_SIZE];
//     memset(recvBuf, 0, BUF_SIZE);

//     // Connect to the server
//     int sockfd = connectToServer(HOST_NAME, HOST_PORT);
//     if (sockfd < 0) {
//         // Error handling
//         return -1;
//     }

//     // Send HTTP GET request
//     const char* getRequest = "GET /GetMap HTTP/1.1\r\nHost: 192.168.137.102\r\nConnection: close\r\n\r\n";
//     if (sendData(sockfd, getRequest) < 0) {
//         // Error handling
//         closeConnection(sockfd);
//         return -1;
//     }

//     // Receive response
//     if (receiveData(sockfd, recvBuf, BUF_SIZE) < 0) {
//         // Error handling
//         closeConnection(sockfd);
//         return -1;
//     }

//     // Close the connection
//     closeConnection(sockfd);

//     // Parse JSON data (Assuming the JSON starts at the first '{' character for simplicity)
//     char* jsonData = strchr(recvBuf, '{');
//     if (jsonData != NULL) {
//         parseJSONWithCJSON(jsonData, &mapData);
//     } else {
//         // Error handling
//         return -1;
//     }

//     return 0;
// }


// // Implementations of connectToServer, closeConnection, sendData, receiveData, and parseJsonData go here
// int main() {
//     // Example JSON data (simplified for demonstration purposes)
//     Mapping mapData = {0};

//     parseJSONWithCJSON(jsonExample, &mapData);

//     // Example output for demonstration purposes
//     printf("Start: (%d, %d)\n", mapData.start.x, mapData.start.y);
//     printf("End: (%d, %d)\n", mapData.end.x, mapData.end.y);
//     printf("Map:\n");
//     for (int i = 0; i < 128; ++i) { // Adjust for actual size
//         for (int j = 0; j < 128; ++j) { // Adjust for actual size
//             printf("%d ", mapData.map[i][j]);
//         }
//         printf("\n");
//     }

//     return 0;
// }




#include <stdint.h>

typedef struct Point {
    int x;
    int y;
} Point;

typedef struct Mapping {
    // 0 is path, 1 is OOB (Out Of Bounds)
    uint64_t map[128][2];  // 128 rows, 2 uint64_t values per row
    Point start;
    Point end;
} Mapping;

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


int main() {
    // Create a test mapping
    Mapping testMapping = {0};  // Initialize all to 0 (path)

    // Set some bits to 1 (OOB) for testing
    // For example, setting bit at (0, 64) and (1, 65)
    testMapping.map[0][1] = 1ULL << 0; // Sets the 64th bit in the first row
    testMapping.map[1][1] = 1ULL << 1; // Sets the 65th bit in the second row

    // Test cases
    printf("Testing getMapValue...\n");

    int result = getMapValue(&testMapping, 0, 64);
    printf("Value at (0, 64): Expected 1, Got %d\n", result);

    result = getMapValue(&testMapping, 1, 65);
    printf("Value at (1, 65): Expected 1, Got %d\n", result);

    result = getMapValue(&testMapping, 0, 0);
    printf("Value at (0, 0): Expected 0, Got %d\n", result);

    result = getMapValue(&testMapping, 1, 0);
    printf("Value at (1, 0): Expected 0, Got %d\n", result);

    // Test out of bounds
    result = getMapValue(&testMapping, -1, 0);
    printf("Value at (-1, 0): Expected -1, Got %d\n", result);

    result = getMapValue(&testMapping, 128, 0);
    printf("Value at (128, 0): Expected -1, Got %d\n", result);

    return 0;
}