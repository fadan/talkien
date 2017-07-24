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
    char *label;
    Dock *next_tab;
    Dock *prev_tab;
    Dock *parent;
    ImVec2 pos;
    ImVec2 size;
    b32 active;
    Status status;
    b32 opened;

    Dock *children[2];
    char location[16];
    i32 last_frame;
    i32 invalid_frames;
    b32 first;
};

struct DockContext
{
    ImVector<Dock *> docks;
    ImVec2 drag_offset;
    Dock *current;
    i32 last_frame;
    EndAction end_action;
};
