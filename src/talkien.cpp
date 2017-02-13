#include "ui.cpp"

static void update_and_render(PlatformInput *input)
{
    static b32 initialized = false;
    if (!initialized)
    {
        init_ui();
        initialized = true;
    }

    begin_ui(input, 1280, 720);
    {
        static bool show_test_window = true;
        ImGui::ShowTestWindow(&show_test_window);
    }
    end_ui();
}
