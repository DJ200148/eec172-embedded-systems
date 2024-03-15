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

typedef struct MappingRows {
    int mapRows[128];
} MappingRows;

typedef struct Mapping {
    //0 is path, 1 is OOB
    int map[128][128];
    Point start;
    Point goal;
} Mapping;

int find_index_in_open_set(Node openSet[], int openSetSize, Point point)
{
    for (int i = 0; i < openSetSize; ++i)
    {
        if (openSet[i].point.x == point.x && openSet[i].point.y == point.y)
        {
            return i;
        }
    }
    return -1; // Not found
}

void add_to_open_set(Node openSet[], int *openSetSize, Node node)
{
    openSet[(*openSetSize)++] = node;
}

int compare_nodes(const void *a, const void *b)
{
    Node *nodeA = (Node *)a;
    Node *nodeB = (Node *)b;
    return nodeA->fScore - nodeB->fScore;
}

void sort_open_set(Node openSet[], int openSetSize)
{
    qsort(openSet, openSetSize, sizeof(Node), compare_nodes);
}

// Function to initialize the map array
void init_map_value(int **map, int size, int value)
{
    for (int i = 0; i < size; i++)
    {
        for (int j = 0; j < size; j++)
        {
            map[i][j] = value; // Initialize all cells to 0
        }
    }
}
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

void print_map_with_path(int **map, Point path[], int pathSize, int map_size)
{
    printf("Map with path:\n");
    for (int i = 0; i < map_size; ++i)
    {
        for (int j = 0; j < map_size; ++j)
        {
            char cell = '.';
            for (int k = 0; k < pathSize; ++k)
            {
                if (path[k].x == i && path[k].y == j)
                {
                    cell = 'P'; // Mark path
                    break;
                }
            }
            if (map[i][j] == 1)
                cell = '#'; // Mark obstacle
            printf("%c ", cell);
        }
        printf("\n");
    }
}

double calculate_distance(Point point1, Point point2)
{
    return sqrt(pow(point2.x - point1.x, 2) + pow(point2.y - point1.y, 2));
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

    for (int x = 0; x < map_size; ++x)
    {
        for (int y = 0; y < map_size; ++y)
        {
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

    while (openSetSize > 0)
    {
        Node current = openSet[0]; // Node with the lowest fScore
        if (current.point.x == end.x && current.point.y == end.y)
        {
            // Reconstruct path
            int pathSize = 0;
            while (!(current.point.x == start.x && current.point.y == start.y))
            {
                path[pathSize++] = current.point;
                current = nodes[current.cameFrom.x][current.cameFrom.y];
            }
            path[pathSize++] = start; // Include start in the path
            // Reverse path
            for (int i = 0; i < pathSize / 2; ++i)
            {
                Point temp = path[i];
                path[i] = path[pathSize - 1 - i];
                path[pathSize - 1 - i] = temp;
            }
            return pathSize;
        }

        // Move current Node from Open Set to Closed Set
        memmove(openSet, openSet + 1, (--openSetSize) * sizeof(Node)); // Remove current
        sort_open_set(openSet, openSetSize);                           // Sort again after removal, simplistic approach

        for (int i = 0; i < 4; ++i)
        { // Check all four neighbors
            Point nextPoint = {current.point.x + directions[i].x, current.point.y + directions[i].y};
            if (nextPoint.x < 0 || nextPoint.x >= map_size || nextPoint.y < 0 || nextPoint.y >= map_size)
                continue; // Skip if out of bounds
            if (map_array[nextPoint.x][nextPoint.y] == 1)
                continue; // Skip obstacles

            int tentative_gScore = current.gScore + 1; // Assume cost between any two nodes is 1
            if (tentative_gScore < nodes[nextPoint.x][nextPoint.y].gScore)
            {
                // This path is better than any previous one. Record it!
                nodes[nextPoint.x][nextPoint.y].cameFrom = current.point;
                nodes[nextPoint.x][nextPoint.y].gScore = tentative_gScore;
                nodes[nextPoint.x][nextPoint.y].fScore = tentative_gScore + heuristic(nextPoint, end);

                if (find_index_in_open_set(openSet, openSetSize, nextPoint) == -1)
                {
                    add_to_open_set(openSet, &openSetSize, nodes[nextPoint.x][nextPoint.y]); // Add to open set if not already present
                    sort_open_set(openSet, openSetSize);                                     // Keep open set sorted
                }
            }
        }
    }

    return 0; // Path not found
}
void make_path_wide(int **map_array, Point *path, int pathLength, int width, int map_size)
{
    int radius = width / 2; // Integer division
    for (int i = 0; i < pathLength; ++i)
    {
        clear_area_around_point(map_array, path[i].x, path[i].y, radius, map_size);
    }
}
void flood_fill(int **map_array, int map_size, int visited[map_size][map_size], int x, int y)
{
    Point stack[map_size * map_size]; // Stack can potentially hold all cells in the worst case
    int top = 0;                      // Stack pointer

    stack[top++] = (Point){x, y}; // Push initial cell to stack
    while (top > 0)
    {
        printf("Stack: %d\n", top);
        Point p = stack[--top]; // Pop cell from stack
        x = p.x;
        y = p.y;

        if (x < 0 || x >= map_size || y < 0 || y >= map_size)
        {
            continue;
        }
        if (visited[x][y] || map_array[x][y] == 1)
        {
            continue;
        }
        visited[x][y] = 1; // Mark as visited

        // Directions: Up, Down, Left, Right
        Point directions[4] = {{-1, 0}, {1, 0}, {0, -1}, {0, 1}};
        for (int i = 0; i < 4; ++i)
        {
            int dx = directions[i].x, dy = directions[i].y;
            stack[top++] = (Point){x + dx, y + dy}; // Push neighboring cells to stack
        }
    }
}
void fill_enclosed_areas(int **map_array, int map_size)
{
    int visited[map_size][map_size];
    memset(visited, 0, sizeof(visited)); // Initialize visited array

    // Start flood fill from all border cells that are empty
    for (int x = 0; x < map_size; ++x)
    {
        if (!visited[x][0] && map_array[x][0] == 0)
        {
            flood_fill(map_array, map_size, visited, x, 0);
        }
        if (!visited[x][map_size - 1] && map_array[x][map_size - 1] == 0)
        {
            flood_fill(map_array, map_size, visited, x, map_size - 1);
        }
    }
    for (int y = 0; y < map_size; ++y)
    {
        if (!visited[0][y] && map_array[0][y] == 0)
        {
            flood_fill(map_array, map_size, visited, 0, y);
        }
        if (!visited[map_size - 1][y] && map_array[map_size - 1][y] == 0)
        {
            flood_fill(map_array, map_size, visited, map_size - 1, y);
        }
    }

    // Fill all unvisited, empty cells
    for (int x = 0; x < map_size; ++x)
    {
        for (int y = 0; y < map_size; ++y)
        {
            if (map_array[x][y] == 0 && !visited[x][y])
            {
                map_array[x][y] = 1; // Mark as obstacle
            }
        }
    }
}

void generate_map_with_random_shapes(int size, int num_seeds, int min_growth_steps, int max_growth_steps, float growth_chance, int path_width, int padding)
{
    int attempts = 25;
    int **map_array = (int **)malloc(size * sizeof(int *));
    for (int i = 0; i < size; i++)
    {
        map_array[i] = (int *)malloc(size * sizeof(int));
    }

    Point *seed_points = (Point *)malloc(num_seeds * sizeof(Point));
    Point start_point, end_point;
    double min_distance = 0.75 * size;

    while (attempts > 0)
    {
        init_map(map_array, size); // Initialize map with zeros
        printf("Attempt %d:\n", 26 - attempts);
        // print_map(map_array, size);

        // Generate seed points
        for (int i = 0; i < num_seeds; i++)
        {
            seed_points[i].x = random_int(0, size - 1);
            seed_points[i].y = random_int(0, size - 1);
        }

        // Attempt to pick start and end points at least 3/4 of the map size apart
        printf("Picking start and end points...\n");
        do
        {
            start_point.x = random_int(padding, size - padding + 1);
            start_point.y = random_int(padding, size - padding + 1);
            end_point.x = random_int(padding, size - padding + 1);
            end_point.y = random_int(padding, size - padding + 1);
        } while (calculate_distance(start_point, end_point) < min_distance);
        printf("Start: (%d, %d), End: (%d, %d)\n", start_point.x, start_point.y, end_point.x, end_point.y);

        // Grow obstacles from seeds
        printf("Growing obstacles...\n");
        grow_obstacles(map_array, seed_points, num_seeds, min_growth_steps, max_growth_steps, growth_chance, size);
        printf("Map after growing obstacles:\n");
        print_map(map_array, size);
        
        // Fill enclosed areas
        printf("Filling enclosed areas...\n");
        fill_enclosed_areas(map_array, size);
        printf("Map after filling enclosed areas:\n");
        print_map(map_array, size);

        // Clear areas around start and end points
        printf("Clearing areas around start and end points...\n");
        clear_area_around_point(map_array, start_point.x, start_point.y, 8, size);
        clear_area_around_point(map_array, end_point.x, end_point.y, 8, size);
        printf("Map after clearing areas around start and end points:\n");
        // print_map(map_array, size);

        // Ensure the start and end points are not obstacles
        map_array[start_point.x][start_point.y] = 0;
        map_array[end_point.x][end_point.y] = 0;

        // Find a path between start and end points
        Point path[size * size]; // Allocate maximum possible path size
        int pathSize = astar(map_array, start_point, end_point, path, size);
        if (pathSize > 0)
        {
            make_path_wide(map_array, path, pathSize, path_width, size);
            // Success: print map or handle as needed
            printf("Map with path:\n");
            // print_map(map_array, size);
            break;
        }
        else
        {
            printf("No valid path found, retrying... (%d attempts left)\n", --attempts);
        }
    }

    if (attempts == 0)
    {
        printf("Failed to generate a valid map after maximum attempts.\n");
    }

    // Clean up
    for (int i = 0; i < size; i++)
    {
        free(map_array[i]);
    }
    free(map_array);
    free(seed_points);
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
    int MAP_SIZE = 128;
    int num_seeds = 25;
    int min_growth_steps = 1500;
    int max_growth_steps = 2000;
    init_random(); // Initialize the random number generator

    // Allocate memory for the map
    int **map = (int **)malloc(MAP_SIZE * sizeof(int *));
    for (int i = 0; i < MAP_SIZE; i++)
    {
        map[i] = (int *)malloc(MAP_SIZE * sizeof(int));
    }

    init_map(map, MAP_SIZE); // Initialize map

    // Define seed points
    Point *seed_points = (Point *)malloc(num_seeds * sizeof(Point));

    // Generate seed points
    for (int i = 0; i < num_seeds; i++)
    {
        seed_points[i].x = random_int(0, MAP_SIZE - 1);
        seed_points[i].y = random_int(0, MAP_SIZE - 1);
    }


    // Call the grow_obstacles function
    grow_obstacles(map, seed_points, num_seeds, min_growth_steps, max_growth_steps, 0.75, MAP_SIZE);

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
    for (int i = 2; i < 8; ++i)
    {
        map[i][5] = 1; // Vertical wall, except at one gap
    }

    Point start = {0, 0};            // Starting point
    Point end = {9, 9};              // Ending point or goal
    Point path[MAP_SIZE * MAP_SIZE]; // Allocate space for the path
    int pathSize;

    pathSize = astar(map, start, end, path, MAP_SIZE);

    if (pathSize > 0)
    {
        printf("Path found, size: %d\n", pathSize);
        print_map_with_path(map, path, pathSize, MAP_SIZE);
    }
    else
    {
        printf("No path found.\n");
    }
}
void test_make_path_wide()
{
    int MAP_SIZE = 20;
    init_random(); // Initialize the random number generator

    // Allocate memory for the map
    int **map = (int **)malloc(MAP_SIZE * sizeof(int *));
    for (int i = 0; i < MAP_SIZE; i++)
    {
        map[i] = (int *)malloc(MAP_SIZE * sizeof(int));
    }

    init_map_value(map, MAP_SIZE, 1); // Initialize map

    // Define a simple path
    Point path[] = {{2, 2}, {2, 3}, {2, 4}, {2, 5}, {2, 6}, {2, 7}, {2, 8}, {2, 9}};
    int pathLength = sizeof(path) / sizeof(path[0]);

    // Apply make_path_wide with width = 3
    make_path_wide(map, path, pathLength, 5, MAP_SIZE);

    // Print the map
    printf("Map after making path wide:\n");
    print_map(map, MAP_SIZE);

    // Free allocated memory
    for (int i = 0; i < MAP_SIZE; i++)
    {
        free(map[i]);
    }
    free(map);
}
void test_fill_enclosed_areas()
{
    int MAP_SIZE = 8;                                     // Define the size of the map
    int **map = (int **)malloc(MAP_SIZE * sizeof(int *)); // Dynamically allocate rows
    for (int i = 0; i < MAP_SIZE; i++)
    {
        map[i] = (int *)malloc(MAP_SIZE * sizeof(int)); // Allocate columns for each row
        for (int j = 0; j < MAP_SIZE; j++)
        {
            map[i][j] = 0; // Initialize map as empty
        }
    }

    // Create some enclosed areas and borders
    for (int i = 0; i < MAP_SIZE; i++)
    {
        map[i][0] = 1;
        map[i][MAP_SIZE - 1] = 1; // Vertical borders
        map[0][i] = 1;
        map[MAP_SIZE - 1][i] = 1; // Horizontal borders
    }
    // Create an enclosed area
    map[2][2] = 1;
    map[2][3] = 1;
    map[2][4] = 1;
    map[3][2] = 1; /* empty */
    map[3][4] = 1;
    map[4][2] = 1;
    map[4][3] = 1;
    map[4][4] = 1;

    printf("Original map:\n");
    print_map(map, MAP_SIZE);

    fill_enclosed_areas(map, MAP_SIZE);

    printf("Map after filling enclosed areas:\n");
    print_map(map, MAP_SIZE);

    // Free the allocated memory
    for (int i = 0; i < MAP_SIZE; i++)
    {
        free(map[i]);
    }
    free(map);
}

int main()
{
    init_random(); // Initialize random seed
    int size = 128;
    int num_seeds = 25;
    int min_growth_steps = 1500;
    int max_growth_steps = 2000;
    float growth_chance = 0.7;
    int path_width = 10;
    int padding = 8;
    generate_map_with_random_shapes(size,
                                    num_seeds,
                                    min_growth_steps,
                                    max_growth_steps,
                                    growth_chance,
                                    path_width,
                                    padding);
    // grow_obstacles_test();
    return 0;
}
