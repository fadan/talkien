
// NOTE(dan): djb2 hash of "talkien"
#define PROTOCOL_ID     0xcf4658ed
#define TIMEOUT_SECS    10.0f

#define MAX_PACKET_SIZE 512

enum ConnectionState
{
    ConnectionState_Disconnected,

    ConnectionState_Listening,
    ConnectionState_Connecting,
    ConnectionState_ConnectionFailed,
    ConnectionState_Connected,
};

struct Connection
{
    ConnectionState state;

    PlatformSocket socket;
    Address remote_address;

    f32 inactive_secs;

    // union
    // {
    //     Connection *next;
    //     Connection *next_free;
    // };
};

// struct Server
// {
//     Connection *first_connection;
//     Connection *first_free_connection;
// };

// struct Client
// {
//     Connection *connection;
// };

struct NetworkState
{
    b32 initialized;

    MemoryStack temp_packet_memory;

    Connection server;
    Connection client;
};
