#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

typedef struct {
    int x, y;
} Point;

typedef struct {
    Point point;
    int gScore;
    int fScore;
    Point cameFrom;
} Node;

#define MAP_SIZE 10

int heuristic(Point a, Point b) {
    return abs(a.x - b.x) + abs(a.y - b.y);
}

void init_map(int map[MAP_SIZE][MAP_SIZE]) {
    memset(map, 0, sizeof(int) * MAP_SIZE * MAP_SIZE); // Initialize map as walkable
    // Add obstacles to map as necessary, e.g., map[1][2] = 1;
}

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

int astar(int map[MAP_SIZE][MAP_SIZE], Point start, Point end, Point path[MAP_SIZE * MAP_SIZE], int map_size) {
    Node openSet[MAP_SIZE * MAP_SIZE]; // Open set as simple array
    int openSetSize = 0;

    Node nodes[MAP_SIZE][MAP_SIZE]; // Node information for each point

    for (int x = 0; x < MAP_SIZE; ++x) {
        for (int y = 0; y < MAP_SIZE; ++y) {
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
            if (nextPoint.x < 0 || nextPoint.x >= MAP_SIZE || nextPoint.y < 0 || nextPoint.y >= MAP_SIZE) continue; // Skip if out of bounds
            if (map[nextPoint.x][nextPoint.y] == 1) continue; // Skip obstacles

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




// Assuming the Point, Node, and astar function declarations are available
// Include or directly write the A* implementation here

void print_map_with_path(int map[MAP_SIZE][MAP_SIZE], Point path[], int pathSize) {
    printf("Map with path:\n");
    for (int i = 0; i < MAP_SIZE; ++i) {
        for (int j = 0; j < MAP_SIZE; ++j) {
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

void test_astar() {
    int map[MAP_SIZE][MAP_SIZE];
    init_map(map); // Initialize your map and obstacles here

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
        print_map_with_path(map, path, pathSize);
    } else {
        printf("No path found.\n");
    }
}

int main() {
    test_astar();
    return 0;
}
