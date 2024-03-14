#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdbool.h>

#define SIZE 10
#define WIDTH 5
#define NUM_SEEDS 5
#define MAX_GROWTH_STEPS_RANGE 10
#define GROWTH_CHANCE 0.5
#define PATH_WIDTH 3
#define ATTEMPTS 25

// Structs
typedef struct
{
    int x, y;
} Point;

// Function to check if index is within bounds
int is_valid(int x, int y, int map_size) {
    return x >= 0 && x < map_size && y >= 0 && y < map_size;
}

// Function to perform flood fill
void flood_fill(int map_array[SIZE][SIZE], bool visited[SIZE][SIZE], int x, int y) {
    if (!is_valid(x, y, SIZE) || visited[x][y] || map_array[x][y] == 1) {
        return;
    }
    visited[x][y] = true;
    int directions[4][2] = {{-1, 0}, {1, 0}, {0, -1}, {0, 1}};
    for (int i = 0; i < 4; i++) {
        int dx = directions[i][0];
        int dy = directions[i][1];
        flood_fill(map_array, visited, x + dx, y + dy);
    }
}

// Function to fill enclosed areas
void fill_enclosed_areas(int map_array[SIZE][SIZE]) {
    bool visited[SIZE][SIZE];
    memset(visited, 0, sizeof(visited));

    // Start flood fill from all border cells that are empty
    for (int x = 0; x < SIZE; x++) {
        if (!visited[x][0] && map_array[x][0] == 0) {
            flood_fill(map_array, visited, x, 0);
        }
        if (!visited[x][SIZE - 1] && map_array[x][SIZE - 1] == 0) {
            flood_fill(map_array, visited, x, SIZE - 1);
        }
    }
    for (int y = 0; y < SIZE; y++) {
        if (!visited[0][y] && map_array[0][y] == 0) {
            flood_fill(map_array, visited, 0, y);
        }
        if (!visited[SIZE - 1][y] && map_array[SIZE - 1][y] == 0) {
            flood_fill(map_array, visited, SIZE - 1, y);
        }
    }

    // Fill all unvisited, empty cells
    for (int x = 0; x < SIZE; x++) {
        for (int y = 0; y < SIZE; y++) {
            if (map_array[x][y] == 0 && !visited[x][y]) {
                map_array[x][y] = 1;
            }
        }
    }
}

// Function to make path wide
void make_path_wide(int map_array[SIZE][SIZE], int path[][2], int width, int map_size) {
    int radius = width / 2;

    // Clear the area around each point in the path
    for (int i = 0; i < sizeof(path) / sizeof(path[0]); i++) {
        int point[2] = {path[i][0], path[i][1]};
        clear_area_around_point(map_array, point, radius, map_size);
    }
}

// Function to calculate the distance between two points
double calculate_distance(int point1[2], int point2[2]) {
    double dx = point2[0] - point1[0];
    double dy = point2[1] - point1[1];
    return sqrt(dx * dx + dy * dy);
}

// Function to generate a map with random shapes
void generate_map_with_random_shapes(int map_array[SIZE][SIZE], Point start_point, Point end_point, int path[SIZE][SIZE]) {
    srand(time(NULL));

    // Initialize the map with all cells as free spaces
    for (int i = 0; i < SIZE; i++) {
        for (int j = 0; j < SIZE; j++) {
            map_array[i][j] = 0;
        }
    }

    // Generate seed points
    Point seed_points[NUM_SEEDS];
    for (int i = 0; i < NUM_SEEDS; i++) {
        seed_points[i].x = rand() % SIZE;
        seed_points[i].y = rand() % SIZE;
    }

    // Grow obstacles from seeds
    grow_obstacles(map_array, seed_points, MAX_GROWTH_STEPS_RANGE, GROWTH_CHANCE, SIZE);

    // Fill enclosed areas
    fill_enclosed_areas(map_array);

    // Clear areas around start and end points before growing obstacles
    clear_area_around_point(map_array, start_point, 8, SIZE);
    clear_area_around_point(map_array, end_point, 8, SIZE);

    // Ensure the start and end points are not obstacles
    map_array[start_point.x][start_point.y] = 0;
    map_array[end_point.x][end_point.y] = 0;

    // Find a path between start and end points
    bool path_found = astar(map_array, start_point, end_point, path);

    if (path_found) {
        make_path_wide(map_array, path, PATH_WIDTH, SIZE);
    }
}

// Function to invert the map
void invert_map(int map_array[SIZE][SIZE]) {
    // Invert the map: obstacles become free spaces (1 becomes 0, 0 becomes 1)
    for (int i = 0; i < SIZE; i++) {
        for (int j = 0; j < SIZE; j++) {
            map_array[i][j] = 1 - map_array[i][j];
        }
    }
}