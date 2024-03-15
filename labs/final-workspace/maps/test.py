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
# def fetch_new_map():
#     # URL of the endpoint that returns the new map data
#     endpoint_url = 'https://xcxzt43up4.execute-api.us-east-2.amazonaws.com/GetMap'
#     response = requests.get(endpoint_url)
#     if response.status_code == 200:
#         return response.json()  # Assuming the endpoint returns JSON data
#     else:
#         raise Exception(f"Failed to fetch new map data, status code: {response.status_code}")
        

# # Fetch new map data
# new_map_data = fetch_new_map()
# print(new_map_data['start'])

# data_bytes = json.dumps(new_map_data['map']).encode('utf-8')

# # Compress the byte stream
# compressed_data = zlib.compress(data_bytes)

# # Get the size of the compressed data in bytes
# size_bytes = sys.getsizeof(compressed_data)
# print(size_bytes)

# # put the data back in the json
# new_map_data['map'] = compressed_data

# print(new_map_data)
# size_bytes = sys.getsizeof(new_map_data)
# print(size_bytes)


import json
import numpy as np
import matplotlib.pyplot as plt

# Simulated smaller version of the JSON data for demonstration
json_data = '''
{"end":{"x":"104","y":"9"},"map":[["18410838455278895104","824765870087"],["18411260664522735616","4363686279271"],["18375231904816300032","499423148263"],["18375240702184390656","257731131891"],["18375240693594456064","481065697279"],["1080929889789411328","481040138233"],["864726321383604224","274879873019"],["1671266230599680","412318040032"],["2111096651513856","137438986208"],["34225520640","131040"],["34091302912","131040"],["2251867459420160","131040"],["6755468026314752","262128"],["7881311091949568","212976"],["24769798470631424","32762"],["13895856650381688832","65535"],["13904863849571418112","65535"],["13869362818081357824","262143"],["13851911369521102848","262143"],["16211762389891153920","360287970189705215"],["16284981068212207616","1080863910569050111"],["16284981068325453824","1008806350890762239"],["17581982576762028032","1022317218492350463"],["18446603336483209216","322007476436336639"],["18446181127776239616","359725037416873983"],["18445618182108479488","2809683217526816767"],["18444624223748489216","7205477928816345087"],["18442306453133852672","17581489995301781503"],["18445600590071332864","18445618173911760895"],["18446427418654670848","18446181124016177151"],["18445688543082708992","18446181124293001215"],["18444492274968559616","16715110017522466815"],["18445618174303928320","9726930770727010303"],["18446181124053925888","936607985264754687"],["18406211678006935552","35747322042515455"],["18302628886673883136","69524319247564799"],["18374686481751998464","67976206875639807"],["18418596578118860800","135319095053664255"],["18445618175908249600","540924536493711359"],["18446181125901910016","2273191911915261951"],["18442240475155857408","2277695511542629119"],["18444492274969542704","2075033528310956063"],["18444492274969477168","4097712710953730055"],["18437736878749253688","1764848103975813127"],["18437736878749515838","1744863380629356547"],["18356672085456060479","10977524091715603"],["18158513701852545087","9429411719806995"],["17870283325700970815","2181431069507833"],["8935141677882924030","422212465066239"],["8070450549427796990","1055531162665184"],["3332894619902","492581209243744"],["3307124815998","211106232533056"],["16501264351100","70368744177728"],["16501264351216","105553116266496"],["16527034155000","57174604644352"],["16527034155004","30786325577728"],["8383776161784","65970697666560"],["8379481194488","277076930199552"],["3884797919216","560750930165760"],["3333968363504","1123752423194624"],["4228395302896","280508609069056"],["4364223643632","280512367165440"],["4381940383712","139912586199040"],["4381940383616","34359469932544"],["13835066834732187520","34359671261181"],["13835060186122812928","7971442526207"],["13835058467863034880","31061195100159"],["13835058055336443904","10720229986303"],["13835058055283736576","30786317193199"],["9223372449179500544","70368739985159"],["481848983552","140737484161024"],["1099450810368","125894077186048"],["292003250176","549747426240"],["4690049761280","549723833336"],["5224802549760","137438433272"],["8522284662784","274877648892"],["1926291783680","137438830591"],["9223372045444579328","66538430463"],["9223372037123145728","16777215"],["16140901071475113984","16777215"],["17293826982717947904","16759263"],["18158521398433939456","32473566"],["17293825871932538880","891712726226175936"],["1103806578688","1143914305352106176"],["1103806562304","1148417904979476480"],["7700675002368","1148417904979476480"],["4399724224512","2301339409586323456"],["1041152","6913025428013711360"],["127744","18446181123756130304"],["63232","18446469195802607616"],["65408","18442801225012346880"],["32640","18438013951385010176"],["32512","18429002354083758080"],["32256","18410987955574276096"],["0","18429002354083758080"],["0","18428861616607985664"],["0","18428791247866953728"],["0","18444518662190661632"],["0","18442306446842396672"],["0","18445686345670852608"],["0","18445890854833618944"],["0","18445626972042690560"],["0","9218930012025520128"],["0","18445653362469240832"],["0","18445670954655285248"],["13835058055282163712","18442293252785700865"],["9223372036854775808","18437860021902508035"],["9223372036854775808","18438004060071133187"],["13835058055282163712","18428878111408914435"],["13835058261440593920","18442518652664676355"],["9236883900888776704","18442735121155555335"],["15763685322522624","17291572968297529350"],["287105021705715712","17293261818170376194"],["14122164728402804736","14986858023668875267"],["17289345353459433472","574212010506403843"],["18375038319097544704","1150670323268435975"],["18375812375283499008","574209016914239503"],["18302910356315439104","31525232825069631"],["18303191833439633408","33552447"],["18303191835318681600","67104831"],["18304880685178945536","67104831"],["17512247150761738240","134213695"],["16395354443173855232","536869375"],["16690340102802505728","2147481855"],["16364948480705167360","2147467327"],["16411646869303721984","2147467327"],["17294914109271179264","1020231807"],["13836078402072739840","1007649023"]],"start":{"x":"8","y":"116"}}
'''

# Parsing JSON and converting map to a 2x128 bit array (for demonstration)
data = json.loads(json_data)
packed_map = data['map']

# Normally this would be 128x128 but for demonstration, we use a smaller size
unpacked_map = np.zeros((128, 128), dtype=bool)

for i, row in enumerate(packed_map):
    for j, val in enumerate(row):
        int_val = int(val)
        for k in range(64):
            bit = (int_val >> k) & 1
            unpacked_map[i][j * 64 + k] = bool(bit)

# Display the map using matplotlib
plt.figure(figsize=(10, 10))
plt.imshow(unpacked_map, cmap='gray')
plt.title("Unpacked Map")
plt.show()

# import numpy as np
# import json
# import matplotlib.pyplot as plt

# def pack_array_to_uint64(arr):
#     packed_array = np.zeros((arr.shape[0], arr.shape[1] // 64), dtype=np.uint64)
#     for i in range(arr.shape[0]):
#         for j in range(0, arr.shape[1], 64):
#             packed_int = np.uint64(0)  # Initialize packed_int as uint64 explicitly
#             for k in range(64):
#                 if j + k < arr.shape[1]:
#                     packed_int |= np.uint64(arr[i, j + k]) << np.uint64(k)
#             packed_array[i, j // 64] = packed_int
#     return packed_array

# def unpack_array_from_uint64(packed_array, shape):
#     unpacked_array = np.zeros(shape, dtype=np.uint8)
#     for i in range(shape[0]):
#         for j in range(0, shape[1], 64):
#             packed_int = int(packed_array[i, j // 64])
#             for k in range(64):
#                 if j + k < shape[1]:
#                     unpacked_array[i, j + k] = (packed_int >> k) & 1
#     return unpacked_array

# def test_packing_unpacking():
#     # Generate a random 128x128 binary array
#     original_array = np.random.randint(0, 2, (128, 128), dtype=np.uint8)

#     # Pack the array into uint64
#     packed_array = pack_array_to_uint64(original_array)

#     # Convert packed array to JSON with numbers as strings
#     json_map = json.dumps(packed_array.tolist(), default=str)

#     # Convert JSON back to Python object and then to numpy array of uint64
#     loaded_map_strings = json.loads(json_map)
#     loaded_map = np.array([[int(num_str) for num_str in row] for row in loaded_map_strings], dtype=np.uint64)

#     # Unpack the array back to original form
#     unpacked_array = unpack_array_from_uint64(loaded_map, original_array.shape)

#     # Plot the original and unpacked arrays for visual confirmation
#     plt.figure(figsize=(12, 6))
#     plt.subplot(1, 2, 1)
#     plt.title("Original Array")
#     plt.imshow(original_array, cmap='gray', interpolation='nearest')

#     plt.subplot(1, 2, 2)
#     plt.title("Unpacked Array")
#     plt.imshow(unpacked_array, cmap='gray', interpolation='nearest')

#     plt.show()

# test_packing_unpacking()


