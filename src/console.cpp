
enum StoredStringType
{
    StoredStringType_Default,

    StoredStringType_Info,
    StoredStringType_Error,
};

struct StoredString
{
    StoredString *next;
    StoredString *prev;

    StoredStringType type;
    u32 length;
    char *string;
};

struct Console
{
    char input_buffer[256];

    MemoryStack item_memory;
    StoredString items_sentinel;

    MemoryStack history_memory;
    StoredString history_sentinel;

    StoredString *last_history;
    b32 scroll_to_bottom;
};

static void init_console(Console *console)
{
    console->items_sentinel.next = &console->items_sentinel;
    console->items_sentinel.prev = &console->items_sentinel;

    console->history_sentinel.next = &console->history_sentinel;
    console->history_sentinel.prev = &console->history_sentinel;

    init_memory_stack(&console->item_memory, 1*MB);
    init_memory_stack(&console->history_memory, 1*MB);
}

static void clear_log(Console *console)
{
    console->items_sentinel.next = &console->items_sentinel;
    console->items_sentinel.prev = &console->items_sentinel;

    clear_memory_stack(&console->item_memory);
}

static void clear_history(Console *console)
{
    console->history_sentinel.next = &console->history_sentinel;
    console->history_sentinel.prev = &console->history_sentinel;

    clear_memory_stack(&console->history_memory);
}

static StoredString *push_string(MemoryStack *string_memory, StoredString *sentinel, char *string)
{
    StoredString *stored_string = push_struct(string_memory, StoredString);
    stored_string->next = sentinel;
    stored_string->prev = sentinel->prev;
    stored_string->next->prev = stored_string;
    stored_string->prev->next = stored_string;

    stored_string->length = 0;
    for (char *at = string; *at; ++at)
    {
        ++stored_string->length;
    }

    u32 size = stored_string->length + 1;
    stored_string->string = (char *)push_size(string_memory, size, no_clear());

    for (u32 char_index = 0; char_index < size; ++char_index)
    {
        stored_string->string[char_index] = string[char_index];
    }
    return stored_string;
}

#define push_console_info(console, format, ...) push_console(console, StoredStringType_Info, format, ## __VA_ARGS__)
#define push_console_error(console, format, ...) push_console(console, StoredStringType_Error, format, ## __VA_ARGS__)

static void push_console(Console *console, StoredStringType type, char *format, ...)
{
    char buffer[256];

    va_list args;
    va_start(args, format);
    vsnprintf(buffer, array_count(buffer), format, args);
    va_end(args);

    StoredString *string = push_string(&console->item_memory, &console->items_sentinel, buffer);
    string->type = type;
    console->scroll_to_bottom = true;
}

static void history_add(Console *console, char *command)
{
    StoredString *history = 0;
    StoredString *sentinel = &console->history_sentinel;
    for (StoredString *stored_string = sentinel->next; stored_string != sentinel; stored_string = stored_string->next)
    {
        if (strings_are_equal(stored_string->string, command))
        {
            history = stored_string;
            break;
        }
    }

    if (!history)
    {
        history = push_string(&console->history_memory, &console->history_sentinel, command);
    }
    else
    {
        history->prev->next = history->next;
        history->next->prev = history->prev;

        history->next = sentinel;
        history->prev = sentinel->prev;
        history->next->prev = history;
        history->prev->next = history;
    }

    console->last_history = history;
}

inline ImVec4 get_item_color_by_type(StoredStringType type)
{
    ImVec4 color = ImColor(1.0f, 1.0f, 1.0f, 1.0f);
    switch (type)
    {
        case StoredStringType_Error:
        {
            color = ImColor(1.0f, 0.4f, 0.4f, 1.0f);
        } break;
        case StoredStringType_Info:
        {
            color = ImColor(1.0f, 0.78f, 0.58f, 1.0f);
        } break;
    }
    return color;
}

static i32 text_edit_callback(ImGuiTextEditCallbackData *data)
{
    Console *console = (Console *)data->UserData;
    switch (data->EventFlag)
    {
        case ImGuiInputTextFlags_CallbackHistory:
        {
            if (console->last_history && console->last_history != &console->history_sentinel)
            {
                char *string = console->last_history->string;
                if (data->EventKey == ImGuiKey_UpArrow)
                {
                    if (console->last_history->prev != &console->history_sentinel)
                    {
                        console->last_history = console->last_history->prev;
                    }
                }
                else
                {
                    if (console->last_history->next != &console->history_sentinel)
                    {
                        console->last_history = console->last_history->next;
                    }
                }
                data->CursorPos = data->SelectionStart = data->SelectionEnd = data->BufTextLen = (i32)snprintf(data->Buf, data->BufSize, "%s", string);
                data->BufDirty = true;
            }
        } break;
    }
    return 0;
}

static void draw_console(UIState *ui_state, Console *console)
{
    if (begin_dock(ui_state, "Console", 0, 0))
    {
        static ImGuiTextFilter filter;
        filter.Draw("Filter (\"incl,-excl\") (\"error\")", 180);

        ImGui::Separator();

        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 1));
        ImGui::BeginChild("ScrollingRegion", ImVec2(0, -ImGui::GetItemsLineHeightWithSpacing()), false, ImGuiWindowFlags_HorizontalScrollbar);
        {
            StoredString *sentinel = &console->items_sentinel;
            for (StoredString *stored_string = sentinel->next; stored_string != sentinel; stored_string = stored_string->next)
            {
                char *item = stored_string->string;
                if (filter.PassFilter(item))
                {
                    ImGui::PushStyleColor(ImGuiCol_Text, get_item_color_by_type(stored_string->type));
                    if (stored_string->type == StoredStringType_Info)
                    {
                        ImGui::Text("> %s", item);
                    }
                    else if (stored_string->type == StoredStringType_Error)
                    {
                        ImGui::Text("! %s", item);
                    }
                    else
                    {
                        ImGui::TextUnformatted(item);
                    }
                    ImGui::PopStyleColor();
                }
            }

            if (console->scroll_to_bottom)
            {
                ImGui::SetScrollHere();
            }
            console->scroll_to_bottom = false;

        }
        ImGui::EndChild();
        ImGui::PopStyleVar();

        ImGui::Separator();

        u32 flags = ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CallbackCompletion | ImGuiInputTextFlags_CallbackHistory;
        if (ImGui::InputText("##console", console->input_buffer, array_count(console->input_buffer), flags, text_edit_callback, console))
        {
            char *command = console->input_buffer;
            if (command[0])
            {
                push_console_info(console, command);
                history_add(console, command);

                if (strings_are_equal(command, "clear"))
                {
                    clear_log(console);
                }
                else
                {
                    push_console_error(console, "Unknown command: %s", command);
                }

                console->input_buffer[0] = 0;
                ImGui::SetKeyboardFocusHere(-1);
            }
        }
    }
    end_dock(ui_state);
}
