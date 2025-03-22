find_package(peff REQUIRED)

file(GLOB HEADERS *.h)
file(GLOB SRC *.cc)

add_library(wandjson SHARED)
target_compile_definitions(wandjson PRIVATE IS_WANDJSON_BASE_BUILDING=1)
set_target_properties(wandjson PROPERTIES CXX_STANDARD 17)
target_sources(wandjson PRIVATE ${HEADERS} ${SRC})
target_include_directories(wandjson PUBLIC $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/> $<INSTALL_INTERFACE:include/>)
target_link_libraries(wandjson PUBLIC peff_base peff_utils peff_containers)
set_target_properties(wandjson PROPERTIES
    PUBLIC_HEADER "${HEADERS}"
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    ARCHIVE FALSE
)

add_library(wandjson_static STATIC)
target_compile_definitions(wandjson_static PRIVATE WANDJSON_STATIC_LINK=1 IS_WANDJSON_BASE_BUILDING=1)
set_target_properties(wandjson_static PROPERTIES CXX_STANDARD 17)
target_sources(wandjson_static PRIVATE ${HEADERS} ${SRC})
target_link_libraries(wandjson_static PUBLIC peff_base_static peff_utils_static peff_containers_static)
target_include_directories(wandjson_static PUBLIC $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/> $<INSTALL_INTERFACE:include/>)
set_target_properties(wandjson_static PROPERTIES
    PUBLIC_HEADER "${HEADERS}"
    VERSION ${PROJECT_VERSION}
)

install(TARGETS wandjson wandjson_static
    EXPORT wandjsonTargets
    LIBRARY DESTINATION lib/
    ARCHIVE DESTINATION lib/
    RUNTIME DESTINATION bin
    PUBLIC_HEADER DESTINATION include/peff/base/
)
