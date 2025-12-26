file(GLOB_RECURSE SOURCES "*.c")

file(GLOB_RECURSE INCLUDE_DIRS_LIST "*.h")
set(INCLUDE_DIRS "")
foreach(header ${INCLUDE_DIRS_LIST})
    get_filename_component(dir ${header} DIRECTORY)
    list(APPEND INCLUDE_DIRS ${dir})
endforeach()
list(REMOVE_DUPLICATES INCLUDE_DIRS)

idf_component_register(
    SRCS ${SOURCES}
    INCLUDE_DIRS ${INCLUDE_DIRS}
)

spiffs_create_partition_image(www webserver/html FLASH_IN_PROJECT)

set(CONFIG_DIR ${CMAKE_CURRENT_SOURCE_DIR}/config)
file(GLOB CONFIG_JSON_FILES "${CONFIG_DIR}/*.json")
if(CONFIG_JSON_FILES)
    set(TEMP_CONFIG_DIR ${CMAKE_BINARY_DIR}/temp_config)
    file(MAKE_DIRECTORY ${TEMP_CONFIG_DIR})
    foreach(json_file ${CONFIG_JSON_FILES})
        get_filename_component(filename ${json_file} NAME)
        file(COPY ${json_file} DESTINATION ${TEMP_CONFIG_DIR})
    endforeach()
    spiffs_create_partition_image(config ${TEMP_CONFIG_DIR} FLASH_IN_PROJECT)
else()
    spiffs_create_partition_image(config config FLASH_IN_PROJECT)
endif()
