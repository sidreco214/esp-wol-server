# For more information about build system see
# https://docs.espressif.com/projects/esp-idf/en/latest/api-guides/build-system.html
# The following five lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.16)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(esp-wol-server)

#scp binary upload
add_custom_target(
    scp
    COMMENT "Sendfing binary to uploader server"
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMAND sh ../scp_bin.sh ${CMAKE_PROJECT_NAME}
    DEPENDS ${CMAKE_PROJECT_NAME}.bin
)