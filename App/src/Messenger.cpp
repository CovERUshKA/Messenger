#include "Messenger.h"

using namespace std;

// Data
static ID3D11Device* g_pd3dDevice = NULL;
static ID3D11DeviceContext* g_pd3dDeviceContext = NULL;
static IDXGISwapChain* g_pSwapChain = NULL;
static ID3D11RenderTargetView* g_mainRenderTargetView = NULL;
static bool g_wndMinimized = false;
static HWND hwnd;
static WNDCLASSEX wc;

struct User {
    int user_id = -1;
    std::string username = "";
};

struct Message {
    int message_id;
    int from;
    std::string text;
};

struct Chat {
    int chat_id;
    std::string name;
    std::vector<Message> messages;
};

static User local_user;
static std::vector<Chat> chats;
static std::vector<User> search_data;

// Forward declarations of helper functions
bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void CreateRenderTarget();
void CleanupRenderTarget();
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Helper to display a little (?) mark which shows a tooltip when hovered.
// In your own code you may want to display an actual icon if you are using a merged icon fonts (see docs/FONTS.md)
static void OpenSettings()
{
    ImGui::OpenPopup("ImportAssetDialog");

    if (ImGui::BeginPopupModal("ImportAssetDialog"))
    {
        ImGui::Text("test");
        ImGui::EndPopup();
    }
}

Message JsonMessageToMessage(nlohmann::json message)
{
    int message_id = message["message_id"].get<int>();
    int from = message["from"].get<int>();
    std::string text = message["text"].get<std::string>();

    return Message{message_id, from, text};
}

void DisplayMessage(std::string message_id, std::string username, std::string text)
{
    if (username == local_user.username)
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.0f, 0.5f, 1.0f, 0.7f));
    else
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(1.0f, 1.0f, 1.0f, 0.2f));
    
    ImGuiStyle& style = ImGui::GetStyle();

    std::string final_text = username;
    final_text.append("\n    ");
    final_text.append(text);

    ImVec2 textSize = ImGui::CalcTextSize(final_text.c_str());

    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 5.0f);

    ImGui::BeginChild(message_id.c_str(), ImVec2(textSize.x + 4, textSize.y + 4));
    
    ImGui::SetCursorPos(ImVec2(2, 2));

    ImGui::Text(final_text.c_str());

    ImGui::EndChild();

    ImGui::PopStyleVar();

    ImGui::PopStyleColor();

    return;
}

bool AddMessageToChat(int chat_id, std::string chat_name, nlohmann::json message)
{
    // range-based for
    for (auto& chat : chats) {
        if (chat.chat_id == chat_id)
        {
            chat.messages.push_back(JsonMessageToMessage(message));
            return true;
        }
    }

    Chat chat;

    chat.chat_id = chat_id;
    chat.name = chat_name;
    chat.messages.push_back(JsonMessageToMessage(message));

    chats.push_back(chat);

    return false;
}

DWORD WINAPI GetUpdates(LPVOID lpvoid)
{
    Network updates_network;
    nlohmann::json updates;
    std::string query_arguments("user_id=");

    int current_user_id = local_user.user_id;

    query_arguments.append(to_string(local_user.user_id).c_str());

    updates_network.Initialize();

    if (updates_network.IsInitialized())
    {
        while (true)
        {
            if (local_user.user_id != -1)
            {
                if (current_user_id != local_user.user_id)
                {
                    query_arguments = std::string("user_id=").append(to_string(local_user.user_id));
                    current_user_id = local_user.user_id;
                }

                if (!updates_network.get_api_with_arguments("getUpdates", query_arguments.c_str(), updates))
                    continue;

                // range-based for
                for (auto& element : updates["result"]) {
                    if (element.contains("message"))
                    {
                        nlohmann::json message;
                        message = element["message"];

                        int chat_id = message["chat"]["chat_id"].get<int>();
                        std::string chat_name = message["chat"]["name"].get<std::string>();

                        message.erase("chat");

                        AddMessageToChat(chat_id, chat_name, message);
                    }
                }
            }
        }
    }

    return NULL;
}

int GetMinimum(nlohmann::json messages)
{
    int minimum = -1;

    // range-based for
    for (auto& message : messages) {
        int message_id = message["message_id"].get<int>();

        if (minimum == -1 || message_id < minimum)
            minimum = message_id;
    }

    return minimum;
}

void RenderMessages(std::string chat_name, std::vector<Message> messages)
{
    for (auto& message : messages)
    {
        std::string username = message.from == local_user.user_id ? local_user.username : chat_name;
        DisplayMessage(std::to_string(message.message_id), username, message.text);
    }

    return;
}

bool LoadChats()
{
    nlohmann::json chats_json;
    std::string query_arguments("user_id=");

    query_arguments.append(to_string(local_user.user_id).c_str());

    if (network.get_api_with_arguments("getChats", query_arguments.c_str(), chats_json))
    {
        chats_json = chats_json["result"]["chats"];

        // range-based for
        for (auto& chat : chats_json) {
            if (chat.contains("chat_id"))
            {
                int chat_id = chat["chat_id"].get<int>();
                std::string chat_name = chat["name"].get<std::string>();

                auto messages_json = chat["messages"];

                std::vector<Message> messages;

                for (size_t i = 0; i < messages_json.size();)
                {
                    int message_id = GetMinimum(messages_json);

                    nlohmann::json message = messages_json[to_string(message_id)];
                    messages_json.erase(to_string(message_id));

                    int from = message["from"].get<int>();
                    std::string text = message["text"].get<std::string>();

                    messages.push_back({message_id, from, text});
                }

                chats.push_back({chat_id, chat_name, messages});
            }
            //std::cout << element << '\n';
        }

        return true;
    }

    return false;
}

static HANDLE hUpdates = 0;

bool BeginUpdatesThread()
{
    if(!hUpdates)
    {
        hUpdates = CreateThread(0, 0, GetUpdates, 0, 0, 0);
    }

    return true;
}

// Simple helper function to load an image into a DX11 texture with common settings
bool LoadTextureFromFile(const char* filename, ID3D11ShaderResourceView** out_srv, int* out_width, int* out_height)
{
    // Load from disk into a raw RGBA buffer
    int image_width = 0;
    int image_height = 0;
    unsigned char* image_data = stbi_load(filename, &image_width, &image_height, NULL, 4);
    if (image_data == NULL)
        return false;

    // Create texture
    D3D11_TEXTURE2D_DESC desc;
    ZeroMemory(&desc, sizeof(desc));
    desc.Width = image_width;
    desc.Height = image_height;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    desc.CPUAccessFlags = 0;

    ID3D11Texture2D *pTexture = NULL;
    D3D11_SUBRESOURCE_DATA subResource;
    subResource.pSysMem = image_data;
    subResource.SysMemPitch = desc.Width * 4;
    subResource.SysMemSlicePitch = 0;
    g_pd3dDevice->CreateTexture2D(&desc, &subResource, &pTexture);

    // Create texture view
    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
    ZeroMemory(&srvDesc, sizeof(srvDesc));
    srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = desc.MipLevels;
    srvDesc.Texture2D.MostDetailedMip = 0;
    g_pd3dDevice->CreateShaderResourceView(pTexture, &srvDesc, out_srv);
    pTexture->Release();

    *out_width = image_width;
    *out_height = image_height;
    stbi_image_free(image_data);

    return true;
}

// Main code
int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR    lpCmdLine,
    _In_ int       nCmdShow)
{
    try
    {
        // Create application window
        //ImGui_ImplWin32_EnableDpiAwareness();
        wc = { sizeof(WNDCLASSEX), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(NULL), NULL, NULL, NULL, NULL, TEXT("ImGui Example"), NULL };
        ::RegisterClassEx(&wc);
        hwnd = ::CreateWindow(wc.lpszClassName, TEXT("Messenger"), WS_OVERLAPPEDWINDOW, 100, 100, 1280, 800, NULL, NULL, wc.hInstance, NULL);

        // Initialize Direct3D
        if (!CreateDeviceD3D(hwnd))
        {
            CleanupDeviceD3D();
            ::UnregisterClass(wc.lpszClassName, wc.hInstance);
            return 1;
        }

        // Show the window
        ::ShowWindow(hwnd, SW_SHOWDEFAULT);
        ::UpdateWindow(hwnd);

        if (curl_global_init(CURL_GLOBAL_DEFAULT))
            throw std::exception("curl_global_init failed");

        if (!network.Initialize())
        {
            return 1;
        }

        nlohmann::json success;

        // Setup Dear ImGui context
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO(); (void)io;
        //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
        //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

        // Setup Dear ImGui style
        ImGui::StyleColorsDark();
        //ImGui::StyleColorsClassic();

        ImGuiStyle& style = ImGui::GetStyle();
        style.WindowBorderSize = 0;
        style.WindowPadding = ImVec2(0, 0);
        style.ScrollbarSize = 5;
        //style.ItemSpacing = ImVec2(1, 0);
        //style.

        // Setup Platform/Renderer backends
        ImGui_ImplWin32_Init(hwnd);
        ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);
        
        // Load Fonts
        // - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
        // - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
        // - If the file cannot be loaded, the function will return NULL. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
        // - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
        // - Read 'docs/FONTS.md' for more instructions and details.
        // - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
        io.Fonts->AddFontDefault();
        //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf", 16.0f);
        //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
        //io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f);
        //io.Fonts->AddFontFromFileTTF("../../misc/fonts/ProggyTiny.ttf", 10.0f);
        //ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, NULL, io.Fonts->GetGlyphRangesJapanese());
        //IM_ASSERT(font != NULL);
        ImFont* pFont = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\arial.ttf", 15.0f, NULL, io.Fonts->GetGlyphRangesCyrillic());
        IM_ASSERT(pFont != NULL);

        // Our state
        bool show_demo_window = true;
        bool show_another_window = false;
        ImVec4 clear_color = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);

        std::string input_username;
        //SecureZeroMemory(input_username, sizeof(input_username));

        std::string input_password;

        std::string input_password_confirmation;

        std::string input_search;

        std::string input_message;

        std::string login_register_error;

        // Main loop
        MSG msg;
        ZeroMemory(&msg, sizeof(msg));
        while (msg.message != WM_QUIT)
        {
            // Poll and handle messages (inputs, window resize, etc.)
            // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
            // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application.
            // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application.
            // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
            if (::PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE))
            {
                ::TranslateMessage(&msg);
                ::DispatchMessage(&msg);
                continue;
            }

            // Don't render if the window is minimized
            if (g_wndMinimized)
            {
                Sleep(1); // Let the processor sleep a bit
                continue;
            }

            // Start the Dear ImGui frame
            ImGui_ImplDX11_NewFrame();
            ImGui_ImplWin32_NewFrame();
            ImGui::NewFrame();

            ImGui::PushFont(pFont);

            // 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
            //if (show_demo_window)
                //ImGui::ShowDemoWindow(&show_demo_window);

            static bool p_open = true;
            static int active_chat = -1;
            static int type = 0; // 0 - Login, 1 - Registration, 2 - Logged in

            static bool use_work_area = true;
            static ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings;

            // We demonstrate using the full viewport area or the work area (without menu-bars, task-bars etc.)
            // Based on your use case you may want one of the other.
            const ImGuiViewport* viewport = ImGui::GetMainViewport();
            ImGui::SetNextWindowPos(use_work_area ? viewport->WorkPos : viewport->Pos);
            ImGui::SetNextWindowSize(use_work_area ? viewport->WorkSize : viewport->Size);

            ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.0f/255.0f, 0.0f/255.0f, 0.0f/255.0f, 1.0f));

            if (ImGui::Begin("Example: Fullscreen window", &p_open, flags))
            {
                ImGui::PopStyleColor();
                //ImGui::Text(user_data.dump().c_str());

                //DisplayMessage("qweqwe", "q2323123we", "qweqweqwe");
                //DisplayMessage("qweqwlre", "q31231we", "qweqweqwe");
                //DisplayMessage("qwekgwer", "qw12312e", "qweqweqwe");

                if (type != 2 && login_register_error.length() != 0)
                {
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.0f, 0.0f, 1.0f));
                    ImGui::Text(login_register_error.c_str());
                    ImGui::PopStyleColor();
                }

                for (size_t i = 0; i < input_username.length(); i++)
                {
                    auto c = input_username[i];
                    if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')))
                    {
                        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.0f, 0.0f, 1.0f));
                        ImGui::Text("Invalid symbols is username");
                        ImGui::PopStyleColor();
                        break;
                    }
                }
                
                // Login menu
                if (type == 0)
                {
                    ImGui::InputTextWithHint("##Username", "Username", &input_username);

                    ImGui::InputTextWithHint("##Password", "Password", &input_password, ImGuiInputTextFlags_Password);

                    bool disabled = (input_username.length() == 0 || input_password.length() == 0);

                    if (disabled)
                    {
                        ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
                        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
                    }

                    //ImGui::SetCursorPosX()
                    //ImGui::PushIte
                    
                    if (ImGui::Button("Login"))
                    {
                        nlohmann::json result;
                        std::string login_query("username=");

                        login_query.append(input_username);
                        login_query.append("&password=");
                        login_query.append(input_password);

                        // username=KEK&password=12345
                        auto success = network.get_api_with_arguments("login", login_query.c_str(), result);

                        if (success)
                        {
                            if (result["ok"] == true)
                            {
                                nlohmann::json user_data = result["result"];

                                int user_id = user_data["user_id"].get<int>();
                                std::string username = user_data["username"].get<std::string>();

                                local_user = {user_id, username};

                                if (LoadChats())
                                {
                                    BeginUpdatesThread();

                                    input_username.clear();
                                    input_password.clear();

                                    style.ItemSpacing = ImVec2(1, 0);

                                    type = 2;
                                }
                                else
                                {
                                    login_register_error = "Unable to load chats";
                                }
                            }
                            else if (result["ok"] == false)
                            {
                                login_register_error = result["description"];
                            }
                        }
                        else
                        {
                            login_register_error = result["description"];
                        }
                        
                    }

                    if (disabled)
                    {
                        ImGui::PopItemFlag();
                        ImGui::PopStyleVar();
                    }

                    if (ImGui::Button("Create Account"))
                    {
                        input_username.clear();
                        input_password.clear();
                        type = 1;
                    }
                }
                // Registration menu
                else if (type == 1)
                {
                    ImGui::InputTextWithHint("##Username", "Username", &input_username);

                    ImGui::InputTextWithHint("##Password", "Password", &input_password, ImGuiInputTextFlags_Password);

                    ImGui::InputTextWithHint("##Password Confirmation", "Password Confirmation", &input_password_confirmation, ImGuiInputTextFlags_Password);

                    bool disabled = (input_username.length() == 0
                                    || input_password.length() == 0
                                    || input_password_confirmation.length() == 0
                    );

                    if (disabled)
                    {
                        ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
                        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
                    }

                    if (ImGui::Button("Create"))
                    {
                        nlohmann::json response;
                        std::string create_query("username=");

                        create_query.append(input_username);
                        create_query.append("&password=");
                        create_query.append(input_password);

                        if (input_password.compare(input_password_confirmation) == 0)
                        {
                            // username=KEK&password=12345
                            if (network.get_api_with_arguments("createAccount", create_query.c_str(), response))
                            {
                                if(response["ok"] == true)
                                {
                                    auto result = response["result"];
                                    
                                    int user_id = result["user_id"].get<int>();
                                    std::string username = result["username"].get<std::string>();

                                    local_user = {user_id, username};

                                    if (LoadChats())
                                    {
                                        BeginUpdatesThread();

                                        input_username.clear();
                                        input_password.clear();
                                        input_password_confirmation.clear();

                                        style.ItemSpacing = ImVec2(1, 0);

                                        type = 2;
                                    }
                                }
                                else
                                {
                                    login_register_error = response["description"];
                                }
                            }
                            else
                            {
                                login_register_error = response["description"];
                            }
                        }
                        else
                        {
                            login_register_error = "Passwords doesn't equal";
                        }
                    }

                    if (disabled)
                    {
                        ImGui::PopItemFlag();
                        ImGui::PopStyleVar();
                    }

                    if (ImGui::Button("Back to Login"))
                    {
                        input_username.clear();
                        input_password.clear();
                        input_password_confirmation.clear();
                        type = 0;
                    }
                }
                // Logged in
                else if (type == 2)
                {
                    auto chats_copy = chats;

                    // List of chats
                    ImGui::BeginChild("Chats", ImVec2(100, viewport->WorkSize.y));
                    
                    style.ItemSpacing = ImVec2(1, 1);

                    if (ImGui::Button("~", ImVec2(30, 30)))
                    {
                        ImGui::SetNextWindowPos(ImVec2(viewport->WorkSize.x/2, viewport->WorkSize.y/2), ImGuiCond_Appearing, ImVec2(0.5f,0.5f));
                        ImGui::OpenPopup("Settings");
                    }

                    if (ImGui::BeginPopup("Settings", ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoMove)) // <-- use last item id as popup id
                    {
                        if (ImGui::Button("Logout"))
                        {
                            type = 0;

                            SecureZeroMemory(&local_user, sizeof(User));

                            local_user.user_id = -1;

                            chats.clear();
                            search_data.clear();

                            style.ItemSpacing = ImVec2(8, 4);

                            ImGui::CloseCurrentPopup();
                        }
                        
                        if (ImGui::Button("Close popup"))
                            ImGui::CloseCurrentPopup();
                        ImGui::EndPopup();
                    }

                    int my_image_width = 25;
                    int my_image_height = 25;
                    ID3D11ShaderResourceView* my_texture = NULL;
                    bool ret = LoadTextureFromFile("settings.png", &my_texture, &my_image_width, &my_image_height);
                    IM_ASSERT(ret);

                    ImGui::Image((void*)my_texture, ImVec2(0.1f, 0.1f));

                    ImGui::SameLine();

                    if (ImGui::InputTextWithHint("##Search", "Search", &input_search, ImGuiInputTextFlags_EnterReturnsTrue))
                    {
                        nlohmann::json result;
                        std::string query_arguments("user_id=");

                        query_arguments.append(to_string(local_user.user_id).c_str());

                        query_arguments.append("&data=");

                        query_arguments.append(input_search);

                        if (network.get_api_with_arguments("search", query_arguments.c_str(), result))
                        {
                            if (result["ok"] == true)
                            {
                                auto users_json = result["result"]["users"];

                                search_data.clear();

                                // range-based for
                                for (auto& user : users_json) {
                                    if (user.contains("user_id") && user.contains("username"))
                                    {
                                        int user_id = user["user_id"].get<int>();
                                        std::string username = user["username"].get<std::string>();

                                        search_data.push_back({user_id, username});
                                    }
                                    //std::cout << element << '\n';
                                }
                            }
                            else
                            {
                                
                            }
                        }
                    }

                    auto old_style = style.ItemSpacing;
                    style.ItemSpacing = ImVec2(1, 0);

                    if (input_search.length() != 0)
                    {
                        // range-based for
                        for (auto& user : search_data) {
                            bool current = (active_chat == user.user_id);
                            if (current)
                            {
                                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.26f, 0.59f, 0.98f, 0.8f));
                            }

                            if (ImGui::Button(user.username.c_str(), ImVec2(100, 30)))
                            {
                                active_chat = user.user_id;
                            }

                            if (current)
                            {
                                ImGui::PopStyleColor();
                            }
                        }
                    }
                    else
                    {
                        if(search_data.size() != 0)
                        {
                            search_data.clear();
                        }

                        // range-based for
                        for (auto& chat : chats_copy) {
                            bool current = (active_chat == chat.chat_id);
                            if (current)
                            {
                                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.26f, 0.59f, 0.98f, 0.8f));
                            }

                            if (ImGui::Button(chat.name.c_str(), ImVec2(100, 30)))
                            {
                                active_chat = chat.chat_id;
                            }

                            if (current)
                            {
                                ImGui::PopStyleColor();
                            }
                        }
                    }

                    ImGui::EndChild();

                    ImGui::SameLine();

                    if (active_chat != -1)
                    {
                        Chat active_chat_struct;

                        // range-based for
                        for (auto& chat : chats_copy) {
                            if (chat.chat_id == active_chat)
                            {
                                active_chat_struct = chat;
                            }
                        }

                        auto messages = active_chat_struct.messages;

                        // Active chat
                        ImGui::BeginChild("Active Chat", ImVec2(viewport->WorkSize.x/5 * 4 - 1, viewport->WorkSize.y));
                        //ImGui::InputTextMultiline("Everything", buf, sizeof(buf));
                        //ImGui::Text("Success: %s", chats_copy.dump().c_str());
                        //ImGui::Text("Ку");
                        //ImGui::Text(active_chat_json.dump().c_str());

                        // Active chat messages
                        ImGui::BeginChild("Active Chat Messages", ImVec2(viewport->WorkSize.x/5 * 4 - 1, viewport->WorkSize.y - 21));

                        // Render messages
                        RenderMessages(active_chat_struct.name, messages);

                        ImGui::EndChild();

                        if (ImGui::InputTextWithHint("##Message input", "Message input", &input_message, ImGuiInputTextFlags_EnterReturnsTrue) && input_message.length() != 0 && input_message.length() <= 4960)
                        {
                            std::string query_arguments("user_id=");

                            query_arguments.append(to_string(local_user.user_id).c_str());

                            query_arguments.append("&chat_id=");

                            query_arguments.append(to_string(active_chat).c_str());

                            query_arguments.append("&text=");

                            query_arguments.append(network.encode_string(input_message));

                            network.get_api_with_arguments("sendMessage", query_arguments.c_str(), success);

                            input_message.clear();
                        }

                        if (ImGui::IsItemDeactivatedAfterEdit())
                            ImGui::SetKeyboardFocusHere(-1); // Auto focus previous widget

                        // Demonstrate keeping auto focus on the input box
                        // if (ImGui::IsItemHovered() || (ImGui::IsRootWindowOrAnyChildFocused() && !ImGui::IsAnyItemActive() && !ImGui::IsMouseClicked(0)))
                        //     ImGui::SetKeyboardFocusHere(-1); // Auto focus previous widget

                        ImGui::EndChild();
                    }

                    style.ItemSpacing = old_style;
                }

                ImGui::End();
            }

            //ImGui::PopStyleColor();

            // 2. Show a simple window that we create ourselves. We use a Begin/End pair to created a named window.
            /*{
                static float f = 0.0f;
                static int counter = 0;

                ImGui::Begin("Hello, world!");                          // Create a window called "Hello, world!" and append into it.

                ImGui::Text("This is some useful text.");               // Display some text (you can use a format strings too)
                ImGui::Checkbox("Demo Window", &show_demo_window);      // Edit bools storing our window open/close state
                ImGui::Checkbox("Another Window", &show_another_window);

                ImGui::SliderFloat("float", &f, 0.0f, 1.0f);            // Edit 1 float using a slider from 0.0f to 1.0f
                ImGui::ColorEdit3("clear color", (float*)&clear_color); // Edit 3 floats representing a color

                if (ImGui::Button("Button"))                            // Buttons return true when clicked (most widgets return true when edited/activated)
                    counter++;
                ImGui::SameLine();
                ImGui::Text("counter = %d", counter);

                ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
                ImGui::End();
            }

            // 3. Show another simple window.
            if (show_another_window)
            {
                ImGui::Begin("Another Window", &show_another_window);   // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
                ImGui::Text("Hello from another window!");
                if (ImGui::Button("Close Me"))
                    show_another_window = false;
                ImGui::End();
            }*/

            ImGui::PopFont();

            // Rendering
            ImGui::Render();
            g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, NULL);
            g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, (float*)&clear_color);
            ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

            g_pSwapChain->Present(1, 0); // Present with vsync
            //g_pSwapChain->Present(0, 0); // Present without vsync
        }
    }
    catch (const std::exception& e)
    {
        MessageBoxA(hwnd, e.what(), "Messenger - Exception", MB_OK | MB_ICONERROR);
    }

    // Cleanup
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    CleanupDeviceD3D();
    ::DestroyWindow(hwnd);
    ::UnregisterClass(wc.lpszClassName, wc.hInstance);

    return 0;
}

// Helper functions
bool CreateDeviceD3D(HWND hWnd)
{
    // Setup swap chain
    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory(&sd, sizeof(sd));
    sd.BufferCount = 2;
    sd.BufferDesc.Width = 0;
    sd.BufferDesc.Height = 0;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    UINT createDeviceFlags = 0;
    //createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0, };
    if (D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext) != S_OK)
        return false;

    CreateRenderTarget();
    return true;
}

void CleanupDeviceD3D()
{
    CleanupRenderTarget();
    if (g_pSwapChain) { g_pSwapChain->Release(); g_pSwapChain = NULL; }
    if (g_pd3dDeviceContext) { g_pd3dDeviceContext->Release(); g_pd3dDeviceContext = NULL; }
    if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = NULL; }
}

void CreateRenderTarget()
{
    ID3D11Texture2D* pBackBuffer;
    g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    g_pd3dDevice->CreateRenderTargetView(pBackBuffer, NULL, &g_mainRenderTargetView);
    pBackBuffer->Release();
}

void CleanupRenderTarget()
{
    if (g_mainRenderTargetView) { g_mainRenderTargetView->Release(); g_mainRenderTargetView = NULL; }
}

// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Win32 message handler
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg)
    {
    case WM_SIZE:
        if (g_pd3dDevice != NULL && wParam != SIZE_MINIMIZED)
        {
            CleanupRenderTarget();
            g_pSwapChain->ResizeBuffers(0, (UINT)LOWORD(lParam), (UINT)HIWORD(lParam), DXGI_FORMAT_UNKNOWN, 0);
            CreateRenderTarget();
        }
        g_wndMinimized = (wParam == SIZE_MINIMIZED);
        return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
            return 0;
        break;
    case WM_DESTROY:
        ::PostQuitMessage(0);
        return 0;
    }
    return ::DefWindowProc(hWnd, msg, wParam, lParam);
}
