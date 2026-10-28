cmake_minimum_required(VERSION 3.10)
project(acre_webkit_to_firefox_addon_adapter VERSION 0.1.0 LANGUAGES C)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK3 REQUIRED gtk+-3.0)
pkg_check_modules(WEBKIT REQUIRED webkit2gtk-4.1)
pkg_check_modules(JSONGLIB REQUIRED json-glib-1.0)

include_directories(include ${GTK3_INCLUDE_DIRS} ${WEBKIT_INCLUDE_DIRS} ${JSONGLIB_INCLUDE_DIRS})

add_library(acrefxadapter SHARED
    src/adapter.c
    src/manifest.c
    src/message_bus.c
    src/content_bridge.c
    src/background_bridge.c
    src/api_bridge.c
    src/storage_bridge.c
)

target_link_libraries(acrefxadapter ${GTK3_LIBRARIES} ${WEBKIT_LIBRARIES} ${JSONGLIB_LIBRARIES})
target_include_directories(acrefxadapter PUBLIC include)

install(TARGETS acrefxadapter LIBRARY DESTINATION lib)
install(DIRECTORY include/ DESTINATION include/acrefxadapter)
