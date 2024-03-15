#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>

#define ROWS 128
#define COLS 128

void pack_array_to_uint64(int arr[ROWS][COLS], uint64_t packed_array[ROWS][COLS/64]) {
    for (int i = 0; i < ROWS; i++) {
        for (int j = 0; j < COLS; j += 64) {
            uint64_t packed_int = 0;
            for (int k = 0; k < 64; k++) {
                packed_int |= (uint64_t)arr[i][j + k] << k;
            }
            packed_array[i][j / 64] = packed_int;
        }
    }
}

void unpack_uint64_to_array(uint64_t packed_array[ROWS][COLS/64], int arr[ROWS][COLS]) {
    for (int i = 0; i < ROWS; i++) {
        for (int j = 0; j < COLS; j += 64) {
            uint64_t packed_int = packed_array[i][j / 64];
            for (int k = 0; k < 64; k++) {
                arr[i][j + k] = (packed_int >> k) & 1;
            }
        }
    }
}

void randomize_array(int arr[ROWS][COLS]) {
    for (int i = 0; i < ROWS; i++) {
        for (int j = 0; j < COLS; j++) {
            arr[i][j] = rand() % 2; // Randomly assign 0 or 1
        }
    }
}

int main() {
    int original_array[ROWS][COLS];
    uint64_t packed_array[ROWS][COLS/64];
    
    // Seed the random number generator
    srand(time(NULL));
    
    // Randomize the original array
    randomize_array(original_array);
    
    // Pack array into an array of uint64_t
    pack_array_to_uint64(original_array, packed_array);
    
    // Print packed array (just for verification)
    printf("Packed array:\n");
    for (int i = 0; i < ROWS; i++) {
        for (int j = 0; j < COLS/64; j++) {
            printf("%llu ", packed_array[i][j]);
        }
        printf("\n");
    }
    
    // Unpack array of uint64_t into the original array
    int unpacked_array[ROWS][COLS];
    unpack_uint64_to_array(packed_array, unpacked_array);
    
    // Print unpacked array (just for verification)
    printf("\nUnpacked array:\n");
    for (int i = 0; i < ROWS; i++) {
        for (int j = 0; j < COLS; j++) {
            printf("%d ", unpacked_array[i][j]);
        }
        printf("\n");
    }
    
    return 0;
}
