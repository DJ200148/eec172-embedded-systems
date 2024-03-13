#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <string.h>
#include <limits.h>
#include <stdbool.h>

#define SIZE 10

// Structs
typedef struct
{
    int x;
    int y;
} Point;

typedef struct
{
    int fScore; // For sorting
    Point point;
} Node;

// Function to add a node to the priority queue (simplified insertion sort for demonstration purposes)
void push(Node *heap, int *heapSize, Node node)
{
    heap[(*heapSize)++] = node;
    // Simple insertion sort to keep the queue sorted by fScore
    for (int i = *heapSize - 1; i > 0 && heap[i].fScore < heap[i - 1].fScore; i--)
    {
        Node temp = heap[i];
        heap[i] = heap[i - 1];
        heap[i - 1] = temp;
    }
}

// Function to pop the node with the lowest fScore from the priority queue
Node pop(Node *heap, int *heapSize)
{
    Node node = heap[0];
    memmove(&heap[0], &heap[1], (--(*heapSize)) * sizeof(Node)); // Shift elements down
    return node;
}

int inSet(Point *set, int setSize, Point point)
{
    for (int i = 0; i < setSize; i++)
    {
        if (set[i].x == point.x && set[i].y == point.y)
            return 1;
    }
    return 0;
}

// Check if a point is in the set
int inNodeSet(Node *set, int setSize, Point point)
{
    for (int i = 0; i < setSize; i++)
    {
        if (set[i].point.x == point.x && set[i].point.y == point.y)
            return 1;
    }
    return 0;
}

// Function to initialize the map array
void init_map(int **map, int size)
{
    for (int i = 0; i < size; i++)
    {
        for (int j = 0; j < size; j++)
        {
            map[i][j] = 0; // Initialize all cells to 0
        }
    }
}

// Function to print the map
void print_map(int **map, int size)
{
    for (int i = 0; i < size; i++)
    {
        for (int j = 0; j < size; j++)
        {
            printf("%d ", map[i][j]);
        }
        printf("\n");
    }
}

// Initialize random seed
void init_random()
{
    srand((unsigned)time(NULL));
}

// Generate a random integer between min and max (inclusive)
int random_int(int min, int max)
{
    return min + rand() % (max - min + 1);
}

// Generate a random float between 0.0 and 1.0
float random_float()
{
    return (float)rand() / (float)RAND_MAX;
}

//  Heuristic for A* algorithm
int heuristic(Point a, Point b)
{
    return abs(a.x - b.x) + abs(a.y - b.y);
}

// Function to mark the path on the map for visualization
void mark_path(int **map, Point *path, int pathSize)
{
    for (int i = 0; i < pathSize; i++)
    {
        map[path[i].x][path[i].y] = 2; // Marking the path with '2'
    }
}

// Functions
void clear_area_around_point(int **map_array, int x_center, int y_center, int radius, int map_size)
{
    for (int x = fmax(0, x_center - radius); x <= fmin(map_size - 1, x_center + radius); ++x)
    {
        for (int y = fmax(0, y_center - radius); y <= fmin(map_size - 1, y_center + radius); ++y)
        {
            if (sqrt((x - x_center) * (x - x_center) + (y - y_center) * (y - y_center)) <= radius)
            {
                map_array[x][y] = 0; // Clear the area
            }
        }
    }
}
void grow_obstacles(int **map_array, Point *seed_points, int num_seeds, int min_growth_steps, int max_growth_steps, float growth_chance, int map_size)
{
    int max_growth_steps_range = random_int(min_growth_steps, max_growth_steps);
    Point directions[4] = {{-1, 0}, {1, 0}, {0, -1}, {0, 1}};
    for (int i = 0; i < num_seeds; i++)
    {
        int x = seed_points[i].x;
        int y = seed_points[i].y;
        map_array[x][y] = 1; // Mark the seed as an obstacle
        int steps = random_int(1, max_growth_steps_range);
        for (int step = 0; step < steps; step++)
        {
            if (random_float() < growth_chance)
            {
                int dir_index = random_int(0, 3);
                int dx = directions[dir_index].x;
                int dy = directions[dir_index].y;
                int new_x = x + dx;
                int new_y = y + dy;
                if (0 <= new_x && new_x < map_size && 0 <= new_y && new_y < map_size)
                {
                    map_array[new_x][new_y] = 1;
                    x = new_x;
                    y = new_y; // Update the current position to new growth
                }
            }
        }
    }
}
int astar(int **map_array, Point start, Point end, Point *path, int map_size)
{
    Node heap[map_size * map_size]; // Priority queue
    int heapSize = 0;

    Point closeSet[map_size * map_size]; // Closed set
    int closeSetSize = 0;

    Point cameFrom[map_size][map_size];     // Track path
    memset(cameFrom, -1, sizeof(cameFrom)); // Initialize with -1

    int gScore[map_size][map_size];          // Cost from start to each position
    memset(gScore, INT_MAX, sizeof(gScore)); // Initialize with max int
    gScore[start.x][start.y] = 0;

    int fScore[map_size][map_size];          // Total cost of getting from the start node to the goal
    memset(fScore, INT_MAX, sizeof(fScore)); // Initialize with max int
    fScore[start.x][start.y] = heuristic(start, end);

    push(heap, &heapSize, (Node){fScore[start.x][start.y], start});

    Point neighbors[4] = {{-1, 0}, {1, 0}, {0, -1}, {0, 1}}; // Neighbor directions
    int pathSize = 0;

    while (heapSize > 0)
    {
        Node current = pop(heap, &heapSize);

        if (current.point.x == end.x && current.point.y == end.y)
        {
            // Reconstruct path
            while (!(current.point.x == start.x && current.point.y == start.y))
            {
                path[pathSize++] = current.point;
                current.point = cameFrom[current.point.x][current.point.y];
            }
            path[pathSize++] = start; // Add start to the path
            return pathSize;          // Path found
        }

        closeSet[closeSetSize++] = current.point;

        for (int i = 0; i < 4; i++)
        { // For each neighbor
            Point neighbor;
            neighbor.x = current.point.x + neighbors[i].x;
            neighbor.y = current.point.y + neighbors[i].y;

            if (neighbor.x >= 0 && neighbor.x < map_size && neighbor.y >= 0 && neighbor.y < map_size)
            { // If within bounds
                if (map_array[neighbor.x][neighbor.y] == 1 || inSet(closeSet, closeSetSize, neighbor))
                    continue; // Check if walkable or in closed set

                int tentative_gScore = gScore[current.point.x][current.point.y] + heuristic(current.point, neighbor);

                if (tentative_gScore < gScore[neighbor.x][neighbor.y])
                {
                    cameFrom[neighbor.x][neighbor.y] = current.point;
                    gScore[neighbor.x][neighbor.y] = tentative_gScore;
                    fScore[neighbor.x][neighbor.y] = gScore[neighbor.x][neighbor.y] + heuristic(neighbor, end);
                    if (!inNodeSet(heap, heapSize, neighbor))
                    {
                        push(heap, &heapSize, (Node){fScore[neighbor.x][neighbor.y], neighbor});
                    }
                }
            }
        }
    }
    return 0; // Path not found
}

// Test functions
void clear_area_around_point_test()
{
    int map_size = 10;              // Example map size
    int radius = 3;                 // Example radius
    int x_center = 5, y_center = 5; // Example center point

    // Allocate memory for the map array
    int **map_array = (int **)malloc(map_size * sizeof(int *));
    for (int i = 0; i < map_size; i++)
    {
        map_array[i] = (int *)malloc(map_size * sizeof(int));
        for (int j = 0; j < map_size; j++)
        {
            map_array[i][j] = 1; // Initialize map with non-zero values to see the clearing effect
        }
    }

    // Clear area around point
    clear_area_around_point(map_array, x_center, y_center, radius, map_size);

    // Print the map array to verify the clearing
    for (int i = 0; i < map_size; i++)
    {
        for (int j = 0; j < map_size; j++)
        {
            printf("%d ", map_array[i][j]);
        }
        printf("\n");
    }

    // Free the allocated memory
    for (int i = 0; i < map_size; i++)
    {
        free(map_array[i]);
    }
    free(map_array);
    return;
}
void grow_obstacles_test()
{
    int MAP_SIZE = 20;
    init_random(); // Initialize the random number generator

    // Allocate memory for the map
    int **map = (int **)malloc(MAP_SIZE * sizeof(int *));
    for (int i = 0; i < MAP_SIZE; i++)
    {
        map[i] = (int *)malloc(MAP_SIZE * sizeof(int));
    }

    init_map(map, MAP_SIZE); // Initialize map

    // Define seed points
    Point seed_points[] = {{5, 5}, {10, 10}, {15, 15}};
    int num_seeds = sizeof(seed_points) / sizeof(seed_points[0]);

    // Call the grow_obstacles function
    grow_obstacles(map, seed_points, num_seeds, 50, 75, 0.75, MAP_SIZE);

    // Print the map to see the obstacles
    print_map(map, MAP_SIZE);

    // Free allocated memory
    for (int i = 0; i < MAP_SIZE; i++)
    {
        free(map[i]);
    }
    free(map);

    return;
}
void astar_test()
{
    const int MAP_SIZE = 10; // Example map size
    int **map = (int **)malloc(MAP_SIZE * sizeof(int *));
    for (int i = 0; i < MAP_SIZE; i++)
    {
        map[i] = (int *)malloc(MAP_SIZE * sizeof(int));
        for (int j = 0; j < MAP_SIZE; j++)
        {
            map[i][j] = 0; // Initialize all cells as walkable
        }
    }

    // Setting up obstacles
    map[3][4] = 1;
    map[3][5] = 1;
    map[3][6] = 1;
    map[4][6] = 1;
    map[5][6] = 1;

    // Corrected start and end points, ensuring they are not on obstacles
    Point start = {2, 3};            // Adjusted start point, not on an obstacle
    Point end = {7, 8};              // Adjusted end point, not on an obstacle
    Point path[MAP_SIZE * MAP_SIZE]; // Allocate maximum possible path size
    int pathSize;

    // Assuming astar function signature is:
    // int astar(int** map, Point start, Point end, Point* path, int map_size);
    pathSize = astar(map, start, end, path, MAP_SIZE);

    if (pathSize > 0)
    {
        printf("Path found with size %d:\n", pathSize);
        mark_path(map, path, pathSize);
    }
    else
    {
        printf("No path found.\n");
    }

    // Print the map with the path
    print_map(map, MAP_SIZE);

    // Cleanup
    for (int i = 0; i < MAP_SIZE; i++)
    {
        free(map[i]);
    }
    free(map);

    return;
}

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

int main()
{
    astar_test();
    return 0;
}
