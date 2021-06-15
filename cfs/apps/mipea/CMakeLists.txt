cmake_minimum_required(VERSION 2.6.4)
project(MIPEA C)

include_directories(src)

aux_source_directory(src APP_SRC_FILES)

# Create the app module
add_cfe_app(mipea ${APP_SRC_FILES})

