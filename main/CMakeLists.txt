set(COMPONENT_SRCS "main.c tcp_client.c stdin.c stdout.c http_server.c")
set(COMPONENT_ADD_INCLUDEDIRS ".")

register_component()

# Create a SPIFFS image from the contents of the 'fonts' directory
# that fits the partition named 'storage'. FLASH_IN_PROJECT indicates that
# the generated image should be flashed when the entire project is flashed to
# the target with 'idf.py -p PORT flash
spiffs_create_partition_image(storage ../data FLASH_IN_PROJECT)
