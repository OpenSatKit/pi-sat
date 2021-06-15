cmake_minimum_required(VERSION 2.6.4)
project(CFS_DEMO C)

include_directories(src)
include_directories(${mipea_MISSION_DIR}/src)

aux_source_directory(src APP_SRC_FILES)

# Create the app module
add_cfe_app(demo ${APP_SRC_FILES})

# link mipea - if static link
#target_link_libraries(demo mipea)
