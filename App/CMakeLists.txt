cmake_minimum_required(VERSION 3.0)

set(CMAKE_BUILD_TYPE Release)

project(Messenger)

set(SOURCES
    src/config.h
    src/Messenger.h
    src/Messenger.cpp
    src/Network/Network.h
    src/Network/Network.cpp)

# Add imgui files
set(imgui
    thirdparty/imgui-1.85/imconfig.h
    thirdparty/imgui-1.85/imgui.cpp
    thirdparty/imgui-1.85/imgui.h
    thirdparty/imgui-1.85/imgui_demo.cpp
    thirdparty/imgui-1.85/imgui_draw.cpp
    thirdparty/imgui-1.85/imgui_internal.h
    thirdparty/imgui-1.85/imgui_tables.cpp
    thirdparty/imgui-1.85/imgui_widgets.cpp
    thirdparty/imgui-1.85/imstb_rectpack.h
    thirdparty/imgui-1.85/imstb_textedit.h
    thirdparty/imgui-1.85/imstb_truetype.h
    thirdparty/imgui-1.85/imgui_stdlib.h
    thirdparty/imgui-1.85/imgui_stdlib.cpp
    thirdparty/imgui-1.85/backends/imgui_impl_dx11.cpp
    thirdparty/imgui-1.85/backends/imgui_impl_dx11.h
    thirdparty/imgui-1.85/backends/imgui_impl_win32.cpp
    thirdparty/imgui-1.85/backends/imgui_impl_win32.h
)



add_executable(Messenger WIN32 ${SOURCES} ${imgui})

source_group("imgui" FILES ${imgui})

# We need to find this OpenSSL package
#find_package(OpenSSL REQUIRED)

target_compile_definitions(Messenger
                            PUBLIC CURL_STATICLIB)

# Compiler is visual studio do this, because libcmt.lib conflicts with some other libraries
# if(MSVC)
#     set_target_properties(Messenger
#                         PROPERTIES LINK_FLAGS "/NODEFAULTLIB:MSVCRT.lib")
# endif()

target_link_libraries(Messenger
                d3d11.lib
                Crypt32.lib
                Ws2_32.lib
                Wldap32.lib
                ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/curl/lib/libcurl.lib
                ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/openssl/libcrypto_static.lib
                ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/openssl/libssl_static.lib
)

target_include_directories(Messenger
                            PUBLIC thirdparty/json
                            PUBLIC thirdparty/imgui-1.85
                            PUBLIC thirdparty/curl/include
                            PUBLIC thirdparty/openssl/include
                            PUBLIC thirdparty/stb-master
)