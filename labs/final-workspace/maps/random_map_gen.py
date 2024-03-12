import numpy as np
import matplotlib.pyplot as plt
from heapq import heappush, heappop

def clear_area_around_point(map_array, point, radius, map_size):
    x_center, y_center = point
    for x in range(max(0, x_center - radius), min(map_size, x_center + radius + 1)):
        for y in range(max(0, y_center - radius), min(map_size, y_center + radius + 1)):
            if np.sqrt((x - x_center) ** 2 + (y - y_center) ** 2) <= radius:
                map_array[x, y] = 0  # Clear the area

def grow_obstacles(map_array, seed_points, max_growth_steps_range, growth_chance, map_size):
    max_growth_steps = np.random.randint(max_growth_steps_range[0], max_growth_steps_range[1] + 1)
    directions = [(-1, 0), (1, 0), (0, -1), (0, 1)]
    for seed in seed_points:
        x, y = seed
        map_array[x, y] = 1  # Mark the seed as an obstacle
        for _ in range(np.random.randint(1, max_growth_steps + 1)):
            if np.random.rand() < growth_chance:
                dx, dy = directions[np.random.randint(0, len(directions))]
                new_x, new_y = x + dx, y + dy
                if 0 <= new_x < map_size and 0 <= new_y < map_size:
                    map_array[new_x, new_y] = 1
                    x, y = new_x, new_y  # Update the current position to new growth
    return map_array

def heuristic(a, b):
    return np.abs(a[0] - b[0]) + np.abs(a[1] - b[1])

def astar(map_array, start, end):
    neighbors = [(-1, 0), (1, 0), (0, -1), (0, 1)]
    close_set = set()
    came_from = {}
    gscore = {start:0}
    fscore = {start:heuristic(start, end)}
    oheap = []
    heappush(oheap, (fscore[start], start))
    
    while oheap:
        current = heappop(oheap)[1]
        if current == end:
            data = []
            while current in came_from:
                data.append(current)
                current = came_from[current]
            return data[::-1]  # Return reversed path, from start to end
        close_set.add(current)
        for i, j in neighbors:
            neighbor = current[0] + i, current[1] + j            
            tentative_g_score = gscore[current] + heuristic(current, neighbor)
            if 0 <= neighbor[0] < map_array.shape[0] and 0 <= neighbor[1] < map_array.shape[1]:
                if map_array[neighbor[0]][neighbor[1]] == 1:
                    continue
                if neighbor in close_set and tentative_g_score >= gscore.get(neighbor, 0):
                    continue
                if tentative_g_score < gscore.get(neighbor, 0) or neighbor not in [i[1] for i in oheap]:
                    came_from[neighbor] = current
                    gscore[neighbor] = tentative_g_score
                    fscore[neighbor] = tentative_g_score + heuristic(neighbor, end)
                    heappush(oheap, (fscore[neighbor], neighbor))
    return False

def fill_enclosed_areas(map_array):
    map_size = map_array.shape[0]
    visited = np.zeros_like(map_array, dtype=bool)

    def flood_fill(x, y):
        stack = [(x, y)]
        while stack:
            x, y = stack.pop()
            if x < 0 or x >= map_size or y < 0 or y >= map_size:
                continue
            if visited[x, y] or map_array[x, y] == 1:
                continue
            visited[x, y] = True
            directions = [(-1, 0), (1, 0), (0, -1), (0, 1)]
            for dx, dy in directions:
                stack.append((x + dx, y + dy))

    # Start flood fill from all border cells that are empty
    for x in range(map_size):
        if not visited[x, 0] and map_array[x, 0] == 0:
            flood_fill(x, 0)
        if not visited[x, map_size - 1] and map_array[x, map_size - 1] == 0:
            flood_fill(x, map_size - 1)
    for y in range(map_size):
        if not visited[0, y] and map_array[0, y] == 0:
            flood_fill(0, y)
        if not visited[map_size - 1, y] and map_array[map_size - 1, y] == 0:
            flood_fill(map_size - 1, y)

    # Fill all unvisited, empty cells
    for x in range(map_size):
        for y in range(map_size):
            if map_array[x, y] == 0 and not visited[x, y]:
                map_array[x, y] = 1

def make_path_wide(map_array, path, width, map_size):
    radius = width // 2
    for point in path:
        clear_area_around_point(map_array, point, radius, map_size)

def calculate_distance(point1, point2):
    return np.sqrt((point2[0] - point1[0]) ** 2 + (point2[1] - point1[1]) ** 2)

def generate_map_with_random_shapes(size, num_seeds, max_growth_steps_range, growth_chance, path_width, attempts=25):
    if attempts <= 0:
        print("Failed to generate a valid map after maximum attempts.")
        return None, None, None, None
    
    map_array = np.zeros((size, size), dtype=np.int8)
    seed_points = [(np.random.randint(0, size), np.random.randint(0, size)) for _ in range(num_seeds)]
    
    # Pick start and end points ensure they are 3/4 of the map size away from each other
    min_distance = 0.75 * size
    while True:
        start_point = (np.random.randint(8, size-8), np.random.randint(8, size-8))
        end_point = (np.random.randint(8, size-8), np.random.randint(8, size-8))
        
        distance = calculate_distance(start_point, end_point)
        
        if distance > min_distance:
            break
    
    # Grow obstacles from seeds
    map_array = grow_obstacles(map_array, seed_points, max_growth_steps_range, growth_chance, size)

    # Fill enclosed areas
    fill_enclosed_areas(map_array)
    
    # Clear areas around start and end points before growing obstacles
    clear_area_around_point(map_array, start_point, 8, size)
    clear_area_around_point(map_array, end_point, 8, size)

    # Ensure the start and end points are not obstacles
    map_array[start_point] = 0
    map_array[end_point] = 0
    
    # Find a path between start and end points
    path = astar(map_array, start_point, end_point)
    if path:
        make_path_wide(map_array, path, path_width, size)  # Make the path wide
        return map_array, start_point, end_point, path
    else:
        print(f"No valid path found, retrying... ({25 - attempts + 1}/25)")
        return generate_map_with_random_shapes(size, num_seeds, max_growth_steps_range, growth_chance, path_width, attempts - 1)

def invert_map(map_array):
    # Invert the map: obstacles become free spaces (1 becomes 0, 0 becomes 1)
    inverted_map = 1 - map_array
    return inverted_map

# Configuration
map_size = 128
num_seeds = 25  # Number of obstacle seeds
max_growth_steps_range = (1500, 2000)  # Range of growth steps for obstacles
growth_chance = 0.7
path_width = 10

map_array_with_random_shapes, start_point, end_point, path = generate_map_with_random_shapes(map_size, num_seeds, max_growth_steps_range, growth_chance, path_width)


# map_array_with_random_shapes = invert_map(map_array_with_random_shapes)

if map_array_with_random_shapes is not None:
    plt.figure(figsize=(10, 10))
    plt.imshow(map_array_with_random_shapes, cmap='binary')
     # Extract X and Y coordinates from the path
    x_coords = [point[0] for point in path]
    y_coords = [point[1] for point in path]

    # Plot the path
    plt.plot(y_coords, x_coords, color="red", linewidth=2)  # Plot path in yellow for visibility
    plt.scatter([start_point[1]], [start_point[0]], color='green')  # Start point
    plt.scatter([end_point[1]], [end_point[0]], color='blue')  # End point
    plt.title("Map with A* Path from Start to End")
    plt.show()
else:
    print("Failed to find a path.")
