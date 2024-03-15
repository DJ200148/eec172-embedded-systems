import numpy as np
import zlib
import json
import sys
import requests


# def pack_array_to_uint64(arr):
#     packed_array = np.zeros((arr.shape[0], arr.shape[1]//64), dtype=np.uint64)
#     for i in range(arr.shape[0]):
#         for j in range(0, arr.shape[1], 64):
#             packed_int = 0
#             for k in range(64):
#                 packed_int |= arr[i, j + k] << k
#             packed_array[i, j // 64] = packed_int
#     return packed_array

# def unpack_uint64_to_array(packed_array, shape):
#     unpacked_array = np.zeros(shape, dtype=int)
#     for i in range(shape[0]):
#         for j in range(0, shape[1], 64):
#             # Convert numpy.uint64 to Python int before bitwise operations
#             packed_int = int(packed_array[i, j // 64])
#             for k in range(64):
#                 unpacked_array[i, j + k] = (packed_int >> k) & 1
#     return unpacked_array

# def randomize_array(shape):
#     return np.random.randint(2, size=shape)




# # Your data structure
# # data = {
# #     "map": [[1007616000, 4593671619917905920], ...],  # Truncated for brevity
# #     "start": {"x": 11, "y": 17},
# #     "end": {"x": 74, "y": 107}
# # }



# # Optionally, save compressed_data to a file or transmit it


# # Define shape of the array
# ROWS = 128
# COLS = 128

# # Randomize the original array
# original_array = randomize_array((ROWS, COLS))

# # Pack array into an array of uint64
# packed_array = pack_array_to_uint64(original_array).tolist()

# # Print packed array (just for verification)
# print("Packed array:")
# print(packed_array)


# # Convert the data structure to a JSON string, then encode it to bytes
# data_bytes = json.dumps(original_array.tolist()).encode('utf-8')

# # Compress the byte stream
# compressed_data = zlib.compress(data_bytes)

# # print(compressed_data)

# uncompressed_data = zlib.decompress(compressed_data)
# # print(uncompressed_data)


# payload_str = str(compressed_data)
            
# # Get the size of the string in bytes
# size_bytes = sys.getsizeof(compressed_data)
# print(size_bytes)


# # Unpack array of uint64 into the original array
# unpacked_array = unpack_uint64_to_array(packed_array, original_array.shape)

# # Print unpacked array (just for verification)
# print("\nUnpacked array:")
# print(unpacked_array)

# Function to fetch new map data
def fetch_new_map():
    # URL of the endpoint that returns the new map data
    endpoint_url = 'https://xcxzt43up4.execute-api.us-east-2.amazonaws.com/GetMap'
    response = requests.get(endpoint_url)
    if response.status_code == 200:
        return response.json()  # Assuming the endpoint returns JSON data
    else:
        raise Exception(f"Failed to fetch new map data, status code: {response.status_code}")
        

# Fetch new map data
new_map_data = fetch_new_map()
print(new_map_data['start'])

data_bytes = json.dumps(new_map_data['map']).encode('utf-8')

# Compress the byte stream
compressed_data = zlib.compress(data_bytes)

# Get the size of the compressed data in bytes
size_bytes = sys.getsizeof(compressed_data)
print(size_bytes)

# put the data back in the json
new_map_data['map'] = compressed_data

print(new_map_data)
size_bytes = sys.getsizeof(new_map_data)
print(size_bytes)