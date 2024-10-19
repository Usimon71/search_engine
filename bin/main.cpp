#include "trie.hpp"
#include "indexer.hpp"
#include "searcher.hpp"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <stdio.h>
#define GL_SILENCE_DEPRECATION

#include <GLES2/gl2.h>
#include <GLFW/glfw3.h> // Will drag system OpenGL headers
//#if defined(IMGUI_IMPL_OPENGL_ES2)

#include <thread>

#define STB_IMAGE_IMPLEMENTATION
#include "../stb_image.h"

//sates
static bool show_index_window = false;
static bool    show_searcher_window = false;
static bool    run_loader = false;
static bool    show_result_q = false;
static bool    invalid_query = false;
static bool    invalid_dir = false;
static bool   not_found = false;
static bool    finish_thread = false;

//textures
static GLuint my_image_texture = 0;
static GLuint my_image_texture2 = 0;
static GLuint my_image_texture3 = 0;

//fonts
static ImFont*  font_title;
static ImFont*  font_regular;
static ImFont*  font_mini;
static ImFont*  font_path;

//texture sizes
static int my_image_width = 0;
static int my_image_height = 0;
static int my_image_width2 = 0;
static int my_image_height2 = 0;
static int my_image_width3 = 0;
static int my_image_height3 = 0;

static std::string cur_indexed_dir;
static int top = 3;
static std::vector<DocI> result_q;

void IndexMe(Indexer&& idx, const char* path) {
    try {
        idx.Index(run_loader);
        invalid_dir = false;
        cur_indexed_dir = path;
    } catch (std::runtime_error& e) {
        invalid_dir = true;
        run_loader = false;
    }
}

bool LoadTextureFromFile(const char* filename, GLuint* out_texture, int* out_width, int* out_height)
{
    // Load from file
    int image_width = 0;
    int image_height = 0;
    unsigned char* image_data = stbi_load(filename, &image_width, &image_height, NULL, 4);
    if (image_data == NULL)
        return false;

    // Create a OpenGL texture identifier
    GLuint image_texture;
    glGenTextures(1, &image_texture);
    glBindTexture(GL_TEXTURE_2D, image_texture);

    // Setup filtering parameters for display
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); // This is required on WebGL for non power-of-two textures
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); // Same

    // Upload pixels into texture
#if defined(GL_UNPACK_ROW_LENGTH) && !defined(__EMSCRIPTEN__)
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
#endif
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image_width, image_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image_data);
    stbi_image_free(image_data);

    *out_texture = image_texture;
    *out_width = image_width;
    *out_height = image_height;

    return true;
}

void LoadHomeButton(bool& status) {
    ImGui::SetCursorPos(ImVec2(750, 600));
    ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(0,153,153,255));
    ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(0, 0, 0, 255));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(0,153,153,100));
    if (ImGui::Button("Home", ImVec2(180, 60)))    {
        status = false;
    }
    ImGui::PopStyleColor();
    ImGui::PopStyleColor();
    ImGui::PopStyleColor();
}

void LoadMainPage() {
    ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(ImVec2(960, 720));
        
        ImGuiWindowFlags window_flags = 0;
        window_flags |= ImGuiWindowFlags_NoTitleBar;
        window_flags |= ImGuiWindowFlags_NoResize;
        window_flags |= ImGuiWindowFlags_NoCollapse;
        window_flags |= ImGuiWindowFlags_NoScrollbar;
        ImGui::Begin("Main page", nullptr, window_flags);

        ImGui::SetCursorPos({0, 0});
        ImGui::Image((void*)(intptr_t)my_image_texture, ImVec2(my_image_width, my_image_height));

        ImDrawList* draw_list = ImGui::GetWindowDrawList();    
        (*draw_list).AddRectFilled({0, 0}, {960, 720}, IM_COL32(255, 255, 255, 100));

        ImGui::SetCursorPos(ImVec2(580, 350));
        ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(0,128,255,255));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(0,128,255, 100));
        if (ImGui::Button("Search", ImVec2(180, 60)))    {
            show_searcher_window = true;
        }
        ImGui::PopStyleColor();
        ImGui::PopStyleColor();

        ImGui::SetCursorPos(ImVec2(225, 300));
        ImGui::PushFont(font_title);
        ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(255,102,102,255));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(255,102,102,100));
        
        if (ImGui::Button("Index", ImVec2(180, 60)))    {
            show_index_window = true;
        }
        ImGui::PopStyleColor();
        ImGui::PopStyleColor();

        ImGui::PopFont();

        ImGui::End();
}

void LoadIndexPage() {
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImVec2(960, 720));
    //ImGui::StyleColorsClassic();
    
    ImGuiWindowFlags window_flags = 0;
    window_flags |= ImGuiWindowFlags_NoTitleBar;
    window_flags |= ImGuiWindowFlags_NoResize;
    window_flags |= ImGuiWindowFlags_NoCollapse;
    window_flags |= ImGuiWindowFlags_NoScrollbar;
    ImGui::PushStyleColor(ImGuiCol_WindowBg, IM_COL32(0,0,0,255));

    ImGui::Begin("Searcher", nullptr, window_flags);

    ImGui::SetCursorPos({0, 0});
    ImGui::Image((void*)(intptr_t)my_image_texture2, ImVec2(my_image_width2, my_image_height2));

    ImDrawList* draw_list = ImGui::GetWindowDrawList();    
    (*draw_list).AddRectFilled({0, 0}, {960, 720}, IM_COL32(255, 255, 255, 100));

    (*draw_list).AddRectFilled({135, 115}, {850, 265}, IM_COL32(102, 255, 102, 200));

    ImGui::PushFont(font_title);
    ImGui::SetCursorPos(ImVec2(150, 130));
    ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(0,0,0,255));
    ImGui::Text("Enter path to index: ");
    ImGui::PopFont();

    static char path[256];
    ImGui::SetCursorPos(ImVec2(140, 180));
    ImGui::SetNextItemWidth(700);
    ImGui::PushFont(font_path);
    ImGui::InputText("##", path, IM_ARRAYSIZE(path));
    ImGui::PopFont();
    
    ImGui::SameLine();
    ImGui::SetCursorPos({850, 175});
    if (run_loader) {
        ImGui::LoadingIndicatorCircle("##spinner", 23, ImVec4(153,255,153,255), ImVec4(0,255,0,255), 32, 7);
    }

    (*draw_list).AddRectFilled({140, 220}, {842, 260}, IM_COL32(224, 224, 224, 200));

    ImGui::SetCursorPos({140, 220});
    ImGui::PushFont(font_mini);
    ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(0,128,255,255));
    ImGui::Text("Current indexed directory:");
    ImGui::PopStyleColor();
    ImGui::SetCursorPosX(140);
    ImGui::Text(cur_indexed_dir.c_str());
    ImGui::PopFont();

    if (invalid_dir) {
        (*draw_list).AddRectFilled({137, 272}, {335, 297}, IM_COL32(255, 153, 153, 150));
        ImGui::PushFont(font_regular);
        ImGui::SetCursorPos(ImVec2(142, 270));
        ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 0, 0, 255));
        ImGui::Text("Invalid directory!");
        ImGui::PopStyleColor();
        ImGui::PopFont();
    }

    ImGui::SetCursorPos(ImVec2(400, 300));
    ImGui::PushFont(font_title);
    ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(0,255,0,255));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(0,255,0,100));
    if (ImGui::Button("Index!", ImVec2(180, 60))) {
        Indexer idx(path);
        std::thread thread = std::thread(IndexMe, std::move(idx), path);
        thread.detach();
    }
    ImGui::PopStyleColor();
    ImGui::PopStyleColor();

    LoadHomeButton(show_index_window);

    ImGui::PopFont();

    ImGui::PopStyleColor();
    ImGui::PopStyleColor();
        
    ImGui::End();
}

void LoadSearcherPage() {
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImVec2(960, 720));
    //ImGui::StyleColorsClassic();
    
    ImGuiWindowFlags window_flags = 0;
    window_flags |= ImGuiWindowFlags_NoTitleBar;
    window_flags |= ImGuiWindowFlags_NoResize;
    window_flags |= ImGuiWindowFlags_NoCollapse;
    window_flags |= ImGuiWindowFlags_NoScrollbar;

    ImGui::Begin("Searcher", nullptr, window_flags);

    ImGui::SetCursorPos({0, 0});
    ImGui::Image((void*)(intptr_t)my_image_texture3, ImVec2(my_image_width3, my_image_height3));

    ImDrawList* draw_list = ImGui::GetWindowDrawList();    
    (*draw_list).AddRectFilled({0, 0}, {960, 720}, IM_COL32(255, 255, 255, 100));

    (*draw_list).AddRectFilled({130, 120}, {820, 265}, IM_COL32(255, 255, 255, 150));
    
    ImGui::PushFont(font_title);

    ImGui::SetCursorPos(ImVec2(140, 130));
    ImGui::Text("Enter your query: ");

    ImGui::PushFont(font_path);
    static char query[128];
    ImGui::SetCursorPos(ImVec2(140, 180));
    ImGui::SetNextItemWidth(675);
    ImGui::InputText("##", query, IM_ARRAYSIZE(query));
    ImGui::PopFont();

    if (invalid_query) {
        (*draw_list).AddRectFilled({137, 272}, {302, 297}, IM_COL32(255, 153, 153, 150));
        ImGui::PushFont(font_regular);
        ImGui::SetCursorPos(ImVec2(142, 270));
        ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 0, 0, 255));
        ImGui::Text("Invalid query!");
        ImGui::PopStyleColor();
        ImGui::PopFont();
    }

    if (not_found) {
            (*draw_list).AddRectFilled({137, 272}, {260, 297}, IM_COL32(51, 153, 255, 150));
        ImGui::PushFont(font_regular);
        ImGui::SetCursorPos(ImVec2(142, 270));
        ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(0, 0, 255, 255));
        ImGui::Text("Not found");
        ImGui::PopStyleColor();
        ImGui::PopFont();
    }

    //(*draw_list).AddRectFilled({140, 220}, {815, 260}, IM_COL32(224, 224, 224, 200));

    ImGui::SetCursorPos({140, 220});
    ImGui::PushFont(font_mini);
    ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(0,128,255,255));
    ImGui::Text("Searching in directory:");
    ImGui::PopStyleColor();
    ImGui::SetCursorPosX(150);
    ImGui::Text(cur_indexed_dir.c_str());
    ImGui::PopFont();

    ImGui::SetCursorPos(ImVec2(400, 300));
    ImGui::PushFont(font_title);
    ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(255,255,255,255));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(255,153,153,200));
    if (ImGui::Button("Search!", ImVec2(200, 60)))    {
        SyntaxTree st;
        try {
            result_q = std::move(st.GetResult(std::string(query), top));
            if (result_q.size() == 0) {
                not_found = true;
                show_result_q = false;
            } else {
                show_result_q = true;
                not_found = false;
            }
            
            invalid_query = false;
        } catch (std::exception& e) {
            invalid_query = true;
            not_found = false;
            show_result_q = false;
        }
    }
    ImGui::PopStyleColor();
    ImGui::PopStyleColor();

    ImGui::PushFont(font_path);
    if (show_result_q) {
        (*draw_list).AddRectFilled({0, 365}, {960, 550}, IM_COL32(224, 224, 224, 200));
        ImGuiTabBarFlags tab_bar_flags = ImGuiTabBarFlags_FittingPolicyResizeDown;
        if (ImGui::BeginTabBar("##", tab_bar_flags)) {
            for (int i = 0; i != result_q.size(); ++i) {
                ImGui::PushItemWidth(100);
                if (ImGui::BeginTabItem(std::to_string(i + 1).c_str())) {
                    ImGui::PushFont(font_regular);
                    ImGui::TextWrapped(result_q[i].filename);
                    std::string lines;
                    int j = 0;
                    for (;j != result_q[i].string_no.size() - 1 && j != 60; ++j) {
                        lines += std::to_string(result_q[i].string_no[j]) + ", ";
                    }
                    if (result_q[i].string_no.size() != 0) {
                        lines += std::to_string(result_q[i].string_no[result_q[i].string_no.size() - 1]);
                    }
                    if (j >= 60) {
                        lines += "...";
                    }
                    ImGui::TextWrapped((("In lines: " + lines).c_str()));
                    ImGui::PopFont();
                    ImGui::EndTabItem();
                }
                ImGui::PopItemWidth();
                
            }
            ImGui::EndTabBar();
        }
    }
    ImGui::PopFont();


    ImGui::PushFont(font_path);
    ImGui::SetCursorPos({820, 180});
    char buf_top[32];
    std::sprintf(buf_top, "Top %d", top);
    if (ImGui::TreeNode(buf_top)) {   
        for (int n = 1; n < 11; n++) {
            char buf[32];
            sprintf(buf, "%d", n);
            ImGui::SetCursorPosX(920);
            if (ImGui::Selectable(buf, top == n))
                top = n;
        }
        ImGui::TreePop();
    }
    ImGui::PopFont();
    
    LoadHomeButton(show_searcher_window);

    ImGui::PopFont();

    ImGui::PopFont();
        
    ImGui::End();
}
// [Win32] Our example includes a copy of glfw3.lib pre-compiled with VS2010 to maximize ease of testing and compatibility with old VS compilers.
// To link with VS2010-era libraries, VS2015+ requires linking with legacy_stdio_definitions.lib, which we do using this pragma.
// Your own project should not be affected, as you are likely to link with a newer binary of GLFW that is adequate for your version of Visual Studio.
#if defined(_MSC_VER) && (_MSC_VER >= 1900) && !defined(IMGUI_DISABLE_WIN32_FUNCTIONS)
#pragma comment(lib, "legacy_stdio_definitions")
#endif

// This example can also compile and run with Emscripten! See 'Makefile.emscripten' for details.
#ifdef __EMSCRIPTEN__
#include "../libs/emscripten/emscripten_mainloop_stub.h"
#endif

static void glfw_error_callback(int error, const char* description)
{
    fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}

// Main code
int main(int, char**)
{
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit())
        return 1;

    // Decide GL+GLSL versions
#if defined(IMGUI_IMPL_OPENGL_ES2)
    // GL ES 2.0 + GLSL 100
    const char* glsl_version = "#version 100";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
#elif defined(__APPLE__)
    // GL 3.2 + GLSL 150
    const char* glsl_version = "#version 150";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // Required on Mac
#else
    // GL 3.0 + GLSL 130
    const char* glsl_version = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    //glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
    //glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // 3.0+ only
#endif

    // Create window with graphics context
    GLFWwindow* window = glfwCreateWindow(960, 720, "Simple searcher", nullptr, nullptr);
    if (window == nullptr)
        return 1;
    glfwMakeContextCurrent(window);
    glfwSetWindowSizeLimits(window, 960, 720, 960, 720);
    glfwSwapInterval(1); // Enable vsync

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    // Setup Dear ImGui style
    //ImGui::StyleColorsDark();
    ImGui::StyleColorsLight();

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window, true);

#ifdef __EMSCRIPTEN__
    ImGui_ImplGlfw_InstallEmscriptenCanvasResizeCallback("#canvas");
#endif
    ImGui_ImplOpenGL3_Init(glsl_version);

    //init fonts
    font_title = io.Fonts->AddFontFromFileTTF("fonts/intodotmatrix.ttf", 30.0f);
    font_regular = io.Fonts->AddFontFromFileTTF("fonts/LEMONMILK-Light.otf", 24.0f);
    font_mini = io.Fonts->AddFontFromFileTTF("fonts/LEMONMILK-Light.otf", 16.0f);
    font_path =  io.Fonts->AddFontFromFileTTF("fonts/LEMONMILK-Light.otf", 30.0f);
    
    //loading textures
    bool ret = LoadTextureFromFile("img/red_pill(1).png", &my_image_texture, &my_image_width, &my_image_height);
    IM_ASSERT(ret);
    bool ret2 = LoadTextureFromFile("img/matrix(1).jpg", &my_image_texture2, &my_image_width2, &my_image_height2);
    IM_ASSERT(ret2);
    bool ret3 = LoadTextureFromFile("img/keanu(1).png", &my_image_texture3, &my_image_width3, &my_image_height3);
    IM_ASSERT(ret3);

    std::ifstream cur_dir_f("cur_dir.txt");
    cur_dir_f >> cur_indexed_dir;
    cur_dir_f.close();

    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 0.00f);

    // Main loop
#ifdef __EMSCRIPTEN__
    // For an Emscripten build we are disabling file-system access, so let's not attempt to do a fopen() of the imgui.ini file.
    // You may manually call LoadIniSettingsFromMemory() to load settings from your own storage.
    io.IniFilename = nullptr;
    EMSCRIPTEN_MAINLOOP_BEGIN
#else
    while (!glfwWindowShouldClose(window))
#endif
    {
        
        glfwPollEvents();

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
        
        // windows
        LoadMainPage();
        if (show_index_window) {   
            LoadIndexPage();
        }
        if (show_searcher_window) {
            LoadSearcherPage();
        }

        // Rendering
        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }
    
#ifdef __EMSCRIPTEN__
    EMSCRIPTEN_MAINLOOP_END;
#endif

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    std::ofstream dir_name_fil("cur_dir.txt");
    dir_name_fil << cur_indexed_dir.c_str();
    dir_name_fil.close();

    return 0;
}

