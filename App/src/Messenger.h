#pragma once

#define WIN32_LEAN_AND_MEAN

#include <Windows.h>
#include <wincrypt.h>
#include <iostream>
#include <d3d11.h>
#include <fstream>
#include <WS2tcpip.h>

#include <single_include/nlohmann/json.hpp>

#include <imgui.h>
#include <imgui_stdlib.h>
#include <backends/imgui_impl_dx11.h>
#include <backends/imgui_impl_win32.h>
#include <imgui_internal.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "Network/Network.h"

using namespace std;