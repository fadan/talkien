struct SortItem
{
    f32 key;
    u32 index;
};

inline u32 sort_key_to_u32(f32 key)
{
    u32 value = *(u32 *)&key;
    if (value & 0x80000000)
    {
        value = ~value;
    }
    else
    {
        value |= 0x80000000;
    }
    return value;
}

static void radix_sort(u32 num_entries, SortItem *first, SortItem *temp)
{
    SortItem *src = first;
    SortItem *dest = temp;

    for (u32 byte_index = 0; byte_index < 32; byte_index += 8)
    {
        u32 sort_key_offsets[256] = {};

        for (u32 index = 0; index < num_entries; ++index)
        {
            u32 radix_value = sort_key_to_u32(src[index].key);
            u32 radix_piece = (radix_value >> byte_index) & 0xFF;
            ++sort_key_offsets[radix_piece];
        }

        u32 total = 0;
        for (u32 sort_key_index = 0; sort_key_index < array_count(sort_key_offsets); ++sort_key_index)
        {
            u32 count = sort_key_offsets[sort_key_index];
            sort_key_offsets[sort_key_index] = total;
            total += count;
        }

        for (u32 index = 0; index < num_entries; ++index)
        {
            u32 radix_value = sort_key_to_u32(src[index].key);
            u32 radix_piece = (radix_value >> byte_index) & 0xFF;
            dest[sort_key_offsets[radix_piece]++] = src[index];
        }

        SortItem *swap_temp = dest;
        dest = src;
        src = swap_temp;
    }
}
