
inline void copy_packet_data(void *buffer, void *data, u32 data_size)
{
    char *src = (char *)data;
    char *dest = (char *)buffer;
    while (data_size--)
    {
        *dest++ = *src++;
    }
}

inline void write_to_packet_u32(char *buffer, i32 *offset, u32 value)
{
    u32 value_size = value;
    if (*offset + value_size <=MAX_PACKET_SIZE)
    {
        u32 *dest = (u32 *)(buffer + *offset);
        *dest = value;
        *offset += value_size;
    }
}

inline void write_to_packet(char *buffer, i32 *offset, void *data, i32 data_size)
{
    if (*offset + data_size <= MAX_PACKET_SIZE)
    {
        copy_packet_data(buffer + *offset, data, data_size);        
        *offset += data_size;
    }
}

static b32 net_send(NetworkState *state, Connection *connection, Address *address, void *buffer, i32 buffer_size)
{
    i32 header_size = sizeof(u32);
    i32 packet_size = header_size + buffer_size;

    b32 sent = false;
    if (packet_size < MAX_PACKET_SIZE)
    {
        TempMemoryStack temp_memory = begin_temp_memory(&state->temp_packet_memory);
        {
            char *packet = (char *)push_size(&state->temp_packet_memory, packet_size);
            i32 offset = 0;

            u32 header = host_to_network_u32(PROTOCOL_ID);
            write_to_packet_u32(packet, &offset, header);
            write_to_packet(packet, &offset, buffer, buffer_size);

            i32 sent_bytes = platform.socket_send(connection->socket, address, packet, packet_size);
            sent = (sent_bytes == packet_size);
        }
        end_temp_memory(temp_memory);
    }
    return sent;
}

static i32 net_receive(NetworkState *state, Connection *connection, Address *from_address, void *buffer, i32 buffer_size)
{
    i32 header_size = sizeof(u32);
    i32 received_bytes = 0;

    TempMemoryStack temp_memory = begin_temp_memory(&state->temp_packet_memory);
    {
        void *packet = push_size(&state->temp_packet_memory, buffer_size + header_size);
        i32 packet_size = platform.socket_recv(connection->socket, from_address, buffer, buffer_size);

        if (packet_size > header_size)
        {
            u32 expected_protocol_id = host_to_network_u32(PROTOCOL_ID);
            u32 protocol_id = *(u32 *)packet;

            if (protocol_id == expected_protocol_id)
            {

            }
        }
    }
    end_temp_memory(temp_memory);

    return received_bytes;
}

static void start_listen(Connection *connection, u16 port)
{
    assert(connection->state == ConnectionState_Disconnected);

    connection->socket = platform.socket_create_udp(port);
    connection->state = ConnectionState_Listening;
}

static void start_join(Connection *connection, u16 port, Address *address)
{
    connection->socket = platform.socket_create_udp(port);
    connection->remote_address.ip = address->ip;
    connection->remote_address.port = address->port;
    connection->state = ConnectionState_Connecting;
}

static void init_net(NetworkState *state)
{
    init_memory_stack(&state->temp_packet_memory, 1*KB);
}

static void update_net(NetworkState *state)
{
    Connection *server = &state->server;
    Connection *client = &state->client;

    
}
