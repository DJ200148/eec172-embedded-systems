#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <string.h>
#include <limits.h>
#include <stdbool.h>
#include <stdint.h>

#define MAP_SIZE 128
#define BITPACK_SIZE (MAP_SIZE * MAP_SIZE / 8)
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

typedef struct MappingRows
{
    int mapRows[128];
} MappingRows;

typedef struct Mapping
{
    // 0 is path, 1 is OOB
    bool map[128][128];
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
void init_map_value(bool map[][128], int size, int value)
{
    for (int i = 0; i < size; i++)
    {
        for (int j = 0; j < size; j++)
        {
            map[i][j] = value; // Initialize all cells to 0
        }
    }
}
void init_map(bool map[][128], int size)
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
void print_map(bool map[][128], int size)
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

void print_map_with_path(bool map[][128], Point path[], int pathSize, int map_size)
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
void clear_area_around_point(bool map_array[][128], int x_center, int y_center, int radius, int map_size)
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
void grow_obstacles(bool map_array[][128], Point *seed_points, int num_seeds, int min_growth_steps, int max_growth_steps, float growth_chance, int map_size)
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
int astar(bool map_array[][128], Point start, Point end, Point *path, int map_size)
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
void make_path_wide(bool map_array[][128], Point *path, int pathLength, int width, int map_size)
{
    int radius = width / 2; // Integer division
    for (int i = 0; i < pathLength; ++i)
    {
        clear_area_around_point(map_array, path[i].x, path[i].y, radius, map_size);
    }
}
void flood_fill(bool map_array[][128], int map_size, int visited[map_size][map_size], int x, int y)
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
void fill_enclosed_areas(bool map_array[][128], int map_size)
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

// Set a cell as visited in the bit-packed array
void set_visited(uint8_t visited[], int x, int y)
{
    int index = y * MAP_SIZE + x;
    visited[index / 8] |= (1 << (index % 8));
}

// Check if a cell has been visited
bool is_visited(uint8_t visited[], int x, int y)
{
    int index = y * MAP_SIZE + x;
    return (visited[index / 8] & (1 << (index % 8))) != 0;
}

// The map_array should now only contain PATH and WALL information
bool dfs(bool map_array[][MAP_SIZE], uint8_t visited[BITPACK_SIZE], Point current, Point end, Point path[], int *pathSize)
{
    // if (current.x < 0 || current.x >= MAP_SIZE || current.y < 0 || current.y >= MAP_SIZE) return false; // Out of bounds
    // if (map_array[current.x][current.y] == false || is_visited(visited, current.x, current.y)) return false; // Not a valid path or already visited
    // printf("Entering DFS at: %d, %d\n", current.x, current.y);
    if (current.x < 0 || current.x >= MAP_SIZE || current.y < 0 || current.y >= MAP_SIZE)
    {
        // printf("Out of bounds\n");
        return false; // Out of bounds
    }
    if (map_array[current.x][current.y] == true || is_visited(visited, current.x, current.y))
    {
        // printf("Blocked or visited\n");
        return false; // Not a valid path or already visited
    }
    if (current.x == end.x && current.y == end.y)
    {                                  // Reached the end
        path[(*pathSize)++] = current; // Add end to path
        return true;
    }

    set_visited(visited, current.x, current.y); // Mark the current cell as visited

    // Explore neighbors: Up, Down, Left, Right
    Point directions[4] = {{0, -1}, {0, 1}, {-1, 0}, {1, 0}};
    for (int i = 0; i < 4; ++i)
    {
        // printf("Current: %d %d\n", current.x, current.y);
        Point next = {current.x + directions[i].x, current.y + directions[i].y};
        if (dfs(map_array, visited, next, end, path, pathSize))
        {
            // Path is being constructed in reverse; no need to add current here again
            return true; // Path to the end found
        }
    }

    // No path found through this node, backtrack without adding to path
    return false;
}
void generate_map_with_random_shapes(Mapping *mapData, int size, int num_seeds, int min_growth_steps, int max_growth_steps, float growth_chance, int path_width, int padding, int attempts)
{
    Point seed_points[num_seeds]; // Allocate seed points on the stack
    Point path[size * size];      // Allocate path storage on the stack; adjust size as needed for your system's limitations
    double min_distance = 0.75 * size;

    for (int attempt = 1; attempt <= attempts; ++attempt)
    {
        init_map(mapData->map, size); // Initialize map with zeros

        // Generate seed points
        for (int i = 0; i < num_seeds; ++i)
        {
            seed_points[i].x = random_int(padding, size - padding - 1);
            seed_points[i].y = random_int(padding, size - padding - 1);
        }

        // Attempt to pick start and goal points
        do
        {
            mapData->start.x = random_int(padding, size - padding - 1);
            mapData->start.y = random_int(padding, size - padding - 1);
            mapData->goal.x = random_int(padding, size - padding - 1);
            mapData->goal.y = random_int(padding, size - padding - 1);
        } while (calculate_distance(mapData->start, mapData->goal) < min_distance);

        // Grow obstacles from seeds
        grow_obstacles(mapData->map, seed_points, num_seeds, min_growth_steps, max_growth_steps, growth_chance, size);

        // Fill enclosed areas
        // fill_enclosed_areas(mapData->map, size);

        // Clear areas around start and goal points
        clear_area_around_point(mapData->map, mapData->start.x, mapData->start.y, path_width / 2, size);
        clear_area_around_point(mapData->map, mapData->goal.x, mapData->goal.y, path_width / 2, size);

        // Ensure the start and goal points are not obstacles
        mapData->map[mapData->start.x][mapData->start.y] = 0;
        mapData->map[mapData->goal.x][mapData->goal.y] = 0;

        // Find a path between start and goal points
        uint8_t visited[BITPACK_SIZE] = {0};
        int pathSize = 0;
        bool madePath = dfs(mapData->map, visited, mapData->start, mapData->goal, path, &pathSize);
        printf("Path size after dfs: %d\n", pathSize);
        if (madePath)
        {
            make_path_wide(mapData->map, path, pathSize, path_width, size);
            // Path found, process or output the map as needed
            printf("Map generated after %d attempts.\n", attempt);
            break;
        }
        else if (attempt == attempts)
        {
            printf("Failed to generate a valid map after maximum attempts.\n");
        }
    }
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
    int attempts = 25;

    Mapping mapData;
    // generate_map_with_random_shapes(&mapData, 10, 5, 15, 0.7, 3, 1);
    generate_map_with_random_shapes(&mapData,
                                    size,
                                    num_seeds,
                                    min_growth_steps,
                                    max_growth_steps,
                                    growth_chance,
                                    path_width,
                                    padding,
                                    attempts);
    // grow_obstacles_test();
    return 0;
}
