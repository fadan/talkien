Profiler *profiler;
Platform platform;
OpenGL gl;

struct AudioRecord
{
    u64 total_samples_written;
    u64 total_samples_read;
    u32 num_buffer_samples;
    f32 *samples;

    f32 volume[2];
    b32 muted;

    union
    {
        AudioRecord *next;
        AudioRecord *next_free;
    };
};

struct AudioState
{
    b32 initialized;

    MemoryStack audio_memory;

    AudioRecord *local_record;
    AudioRecord *first_record;
    AudioRecord *first_free_record;

    f32 master_volume[2];
};

struct AppState
{
    b32 initialized;

    MemoryStack app_memory;
    AudioState audio_state;
};

// NOTE(dan): test audio
#define FOURCC(a, b, c, d) (((u32)(a) << 0) | ((u32)(b) << 8) | ((u32)(c) << 16) | ((u32)(d) << 24))
#define WAVE_FORMAT_PCM 0x0001

enum RiffChunkID
{
    RiffChunkID_Riff = FOURCC('R', 'I', 'F', 'F'),
    RiffChunkID_Wave = FOURCC('W', 'A', 'V', 'E'),
    RiffChunkID_Fmt  = FOURCC('f', 'm', 't', ' '),
    RiffChunkID_Data = FOURCC('d', 'a', 't', 'a'),
};

#pragma pack(push, 1)
struct RiffChunk
{
    u32 id;
    u32 size;
};

struct RiffHeader
{
    RiffChunk chunk;
    u32 format;
};

struct WaveFormatChunk
{
    u16 format_tag;
    u16 num_channels;
    u32 samples_per_sec;
    u32 avg_bytes_per_sec;
    u16 block_align;
};

struct WaveFormatPCMChunk
{
    u16 bits_per_sample;
};
#pragma pack(pop)

struct RiffIterator
{
    u8 *at;
    u8 *end;
};

inline RiffIterator get_riff_iterator(RiffHeader *header)
{
    RiffIterator iterator = {0};
    iterator.at = (u8 *)(header + 1);
    iterator.end = iterator.at + header->chunk.size - 4;
    return iterator;
}

inline b32 riff_iterator_valid(RiffIterator iterator)
{
    b32 valid = (iterator.at < iterator.end);
    return valid;
}

inline RiffIterator next_riff_iterator(RiffIterator iterator)
{
    RiffChunk *chunk = (RiffChunk *)iterator.at;
    u32 chunk_size = (chunk->size + 1) & ~1;

    iterator.at += sizeof(RiffChunk) + chunk_size;
    return iterator;
}

struct LoadedWav
{
    u16 num_channels;
    u16 bits_per_sample;
    u32 samples_per_sec;
    u32 num_samples;
    i16 *samples;
};
