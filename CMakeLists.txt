cmake_minimum_required(VERSION 3.13)
project(wandjson VERSION 0.1.0)

add_subdirectory("wandjson")
add_subdirectory("example")

# Generate the version file for the config file
include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/wandjsonConfigVersion.cmake
    VERSION ${PACKAGE_VERSION}
    COMPATIBILITY SameMinorVersion
)

# Create config file
configure_package_config_file(
    cmake/wandjsonConfig.cmake.in wandjsonConfig.cmake
    INSTALL_DESTINATION lib/cmake/wandjson
)

# Install config files
install(
    FILES   
       ${CMAKE_CURRENT_BINARY_DIR}/wandjsonConfig.cmake
       ${CMAKE_CURRENT_BINARY_DIR}/wandjsonConfigVersion.cmake
    DESTINATION
        lib/cmake/${PROJECT_NAME}
)

# Exporting Targets from the Build Tree
install(EXPORT wandjsonTargets
    FILE wandjsonTargets.cmake
    DESTINATION lib/cmake/${PROJECT_NAME}
)
