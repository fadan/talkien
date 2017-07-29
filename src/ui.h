#define IMGUI_DISABLE_INCLUDE_IMCONFIG_H
#include "imgui.h"

#define UI_SETTINGS_ID(a, b, c, d)  (((u32)(a) << 0) | ((u32)(b) << 8) | ((u32)(c) << 16) | ((u32)(d) << 24))
#define UI_SETTINGS_FILE_ID         UI_SETTINGS_ID('u', 'i', 's', 'f')
#define UI_SETTINGS_FILE_VERSION    1

struct UISettingsFileHeader
{
    u32 file_id;
    u32 file_version;
    u32 dock_size;
    u32 num_docks;
};

struct UISettingsFileData
{
    i8 parent_index;           //  1     1
    i8 prev_index;             //  1     2
    i8 next_index;             //  1     3
    i8 children_index[2];      //  2     5
    i16 pos[2];                //  4     9
    i16 size[2];               //  4    13
    i8 active;                 //  1    14
    i8 opened;                 //  1    15
    u8 status;                 //  1    16
    char location[16];         // 16    32
    char label[32];            // 32    64 bytes
};

struct UISettingsFile
{
    UISettingsFileHeader *header;
    UISettingsFileData *data;
};

enum
{
    attrib_pos,
    attrib_uv,
    attrib_color,

    attrib_count,
};

enum
{
    uniform_tex,
    uniform_proj_mat,

    uniform_count,
};

enum Slot
{
    Slot_Left,
    Slot_Right,
    Slot_Top,
    Slot_Bottom,
    Slot_Tab,

    Slot_Float,
    Slot_None
};

enum EndAction
{
    EndAction_None,
    EndAction_Panel,
    EndAction_End,
    EndAction_EndChild
};

enum Status
{
    Status_Docked,
    Status_Float,
    Status_Dragged
};

struct Dock
{
    ImU32 id;

    Dock *parent;
    Dock *next_tab;
    Dock *prev_tab;
    Dock *children[2];

    ImVec2 pos;
    ImVec2 size;
    Status status;
    b32 active;
    b32 opened;
    b32 first;
    i32 last_frame;
    i32 invalid_frames;
    
    char label[32];
    char location[16];

    union
    {
        Dock *next;
        Dock *next_free;
    };
};

struct UIState
{
    GLint attribs[attrib_count];
    GLint uniforms[uniform_count];

    GLuint program;
    GLuint vbo;
    GLuint vao;
    GLuint elements;
    GLuint font_texture;

    ImGuiIO *imgui_io;
    MemoryStack ui_memory;

    // NOTE(dan): docks

    ImVec2 drag_offset;
    EndAction end_action;

    i32 last_frame;

    Dock *current;
    Dock *first_dock;
    Dock *first_free_dock;
};
