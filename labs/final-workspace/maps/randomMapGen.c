#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <string.h>
#include <limits.h>
#include <stdbool.h>

// Structs
typedef struct
{
    int x, y;
} Point;

typedef struct
{
    Point point;
    int gScore;
    int fScore;
    Point cameFrom;
} Node;

int find_index_in_open_set(Node openSet[], int openSetSize, Point point) {
    for (int i = 0; i < openSetSize; ++i) {
        if (openSet[i].point.x == point.x && openSet[i].point.y == point.y) {
            return i;
        }
    }
    return -1; // Not found
}

void add_to_open_set(Node openSet[], int *openSetSize, Node node) {
    openSet[(*openSetSize)++] = node;
}

int compare_nodes(const void *a, const void *b) {
    Node *nodeA = (Node *)a;
    Node *nodeB = (Node *)b;
    return nodeA->fScore - nodeB->fScore;
}

void sort_open_set(Node openSet[], int openSetSize) {
    qsort(openSet, openSetSize, sizeof(Node), compare_nodes);
}

// // Function to add a node to the priority queue (simplified insertion sort for demonstration purposes)
// void push(Node *heap, int *heapSize, Node node)
// {
//     heap[(*heapSize)++] = node;
//     // Simple insertion sort to keep the queue sorted by fScore
//     for (int i = *heapSize - 1; i > 0 && heap[i].fScore < heap[i - 1].fScore; i--)
//     {
//         Node temp = heap[i];
//         heap[i] = heap[i - 1];
//         heap[i - 1] = temp;
//     }
// }

// // Function to pop the node with the lowest fScore from the priority queue
// Node pop(Node *heap, int *heapSize)
// {
//     Node node = heap[0];
//     memmove(&heap[0], &heap[1], (--(*heapSize)) * sizeof(Node)); // Shift elements down
//     return node;
// }

// int inSet(Point *set, int setSize, Point point)
// {
//     for (int i = 0; i < setSize; i++)
//     {
//         if (set[i].x == point.x && set[i].y == point.y)
//             return 1;
//     }
//     return 0;
// }

// // Check if a point is in the set
// int inNodeSet(Node *set, int setSize, Point point)
// {
//     for (int i = 0; i < setSize; i++)
//     {
//         if (set[i].point.x == point.x && set[i].point.y == point.y)
//             return 1;
//     }
//     return 0;
// }

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

// // Function to mark the path on the map for visualization
// void mark_path(int **map, Point *path, int pathSize)
// {
//     for (int i = 0; i < pathSize; i++)
//     {
//         map[path[i].x][path[i].y] = 2; // Marking the path with '2'
//     }
// }


void print_map_with_path(int **map, Point path[], int pathSize, int map_size) {
    printf("Map with path:\n");
    for (int i = 0; i < map_size; ++i) {
        for (int j = 0; j < map_size; ++j) {
            char cell = '.';
            for (int k = 0; k < pathSize; ++k) {
                if (path[k].x == i && path[k].y == j) {
                    cell = 'P'; // Mark path
                    break;
                }
            }
            if (map[i][j] == 1) cell = '#'; // Mark obstacle
            printf("%c ", cell);
        }
        printf("\n");
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
    Node openSet[map_size * map_size]; // Open set as simple array
    int openSetSize = 0;

    Node nodes[map_size][map_size]; // Node information for each point

    for (int x = 0; x < map_size; ++x) {
        for (int y = 0; y < map_size; ++y) {
            nodes[x][y].point.x = x;
            nodes[x][y].point.y = y;
            nodes[x][y].gScore = INT_MAX;
            nodes[x][y].fScore = INT_MAX;
            nodes[x][y].cameFrom.x = -1; // -1 indicates 'undefined'
            nodes[x][y].cameFrom.y = -1;
        }
    }

    // Initialize start node
    nodes[start.x][start.y].gScore = 0;
    nodes[start.x][start.y].fScore = heuristic(start, end);
    add_to_open_set(openSet, &openSetSize, nodes[start.x][start.y]);

    Point directions[4] = {{-1, 0}, {1, 0}, {0, -1}, {0, 1}};

    while (openSetSize > 0) {
        Node current = openSet[0]; // Node with the lowest fScore
        if (current.point.x == end.x && current.point.y == end.y) {
            // Reconstruct path
            int pathSize = 0;
            while (!(current.point.x == start.x && current.point.y == start.y)) {
                path[pathSize++] = current.point;
                current = nodes[current.cameFrom.x][current.cameFrom.y];
            }
            path[pathSize++] = start; // Include start in the path
            // Reverse path
            for (int i = 0; i < pathSize / 2; ++i) {
                Point temp = path[i];
                path[i] = path[pathSize - 1 - i];
                path[pathSize - 1 - i] = temp;
            }
            return pathSize;
        }

        // Move current Node from Open Set to Closed Set
        memmove(openSet, openSet + 1, (--openSetSize) * sizeof(Node)); // Remove current
        sort_open_set(openSet, openSetSize); // Sort again after removal, simplistic approach

        for (int i = 0; i < 4; ++i) { // Check all four neighbors
            Point nextPoint = {current.point.x + directions[i].x, current.point.y + directions[i].y};
            if (nextPoint.x < 0 || nextPoint.x >= map_size || nextPoint.y < 0 || nextPoint.y >= map_size) continue; // Skip if out of bounds
            if (map_array[nextPoint.x][nextPoint.y] == 1) continue; // Skip obstacles

            int tentative_gScore = current.gScore + 1; // Assume cost between any two nodes is 1
            if (tentative_gScore < nodes[nextPoint.x][nextPoint.y].gScore) {
                // This path is better than any previous one. Record it!
                nodes[nextPoint.x][nextPoint.y].cameFrom = current.point;
                nodes[nextPoint.x][nextPoint.y].gScore = tentative_gScore;
                nodes[nextPoint.x][nextPoint.y].fScore = tentative_gScore + heuristic(nextPoint, end);

                if (find_index_in_open_set(openSet, openSetSize, nextPoint) == -1) {
                    add_to_open_set(openSet, &openSetSize, nodes[nextPoint.x][nextPoint.y]); // Add to open set if not already present
                    sort_open_set(openSet, openSetSize); // Keep open set sorted
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
    int MAP_SIZE = 20;
    init_random(); // Initialize the random number generator

    // Allocate memory for the map
    int **map = (int **)malloc(MAP_SIZE * sizeof(int *));
    for (int i = 0; i < MAP_SIZE; i++)
    {
        map[i] = (int *)malloc(MAP_SIZE * sizeof(int));
    }

    init_map(map, MAP_SIZE); // Initialize map

    // Example: setting up a simple obstacle
    for (int i = 2; i < 8; ++i) {
        map[i][5] = 1; // Vertical wall, except at one gap
    }

    Point start = {0, 0}; // Starting point
    Point end = {9, 9}; // Ending point or goal
    Point path[MAP_SIZE * MAP_SIZE]; // Allocate space for the path
    int pathSize;

    pathSize = astar(map, start, end, path, MAP_SIZE);

    if (pathSize > 0) {
        printf("Path found, size: %d\n", pathSize);
        print_map_with_path(map, path, pathSize, MAP_SIZE);
    } else {
        printf("No path found.\n");
    }
}

int main()
{
    astar_test();
    return 0;
}