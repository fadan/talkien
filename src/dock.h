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
};

struct DockContext
{
    Dock *current;
    ImVector<Dock *> docks;

    // u32 num_docks;
    // Dock docks[32];

    i32 last_frame;
    ImVec2 drag_offset;
    EndAction end_action;
};
