cmake_minimum_required(VERSION 3.16)

set(IDF_TARGET "esp32")

execute_process(
    COMMAND git describe --tags --always --dirty=-dev --broken=-dev
    OUTPUT_VARIABLE GIT_VERSION
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

set(PROJECT_VER "${GIT_VERSION}")

include($ENV{IDF_PATH}/tools/cmake/project.cmake)

project("Lightbar2api" C)
