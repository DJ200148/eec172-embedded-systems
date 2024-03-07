# Generate C code from the 2D array and export it to a .c file
import numpy as np
base_path = 'labs/final-workspace/maps/'

def generate_c_code(maze, filename="maze_map.c"):
    height, width = maze.shape
    c_code = "bool maze_map[{}][{}] = [\n".format(height, width)
    for row in maze:
        c_code += "    [ " + ", ".join(["1" if cell else "0" for cell in row]) + " ],\n"
    c_code += "];\n"
    
    # Write the generated C code to a file
    with open(f"{base_path}{filename}", "w") as file:
        file.write(c_code)
        
    return f"{base_path}{filename}"

# Adjust the create_center_path_maze function to generate a 128x128 maze with a path width of 20 pixels through the center from left to right

def create_center_path_maze_adjusted(width, height, path_width):
    # Initialize the maze with 0s
    maze = np.zeros((height, width), dtype=np.int8)
    
    # Calculate the start and end positions of the path
    start_y = height // 2 - path_width // 2
    end_y = start_y + path_width
    
    # Draw the path across the center of the map from left to right
    maze[start_y:end_y, :] = 1
    
    return maze

# Redefine dimensions and path width according to new specifications
width, height, path_width = 128, 128, 20

# Create the adjusted maze
center_path_maze_adjusted = create_center_path_maze_adjusted(width, height, path_width)

# Generate C code for the adjusted maze and export it
c_file_path_adjusted = generate_c_code(center_path_maze_adjusted, "maze_map_adjusted.c")

c_file_path_adjusted

