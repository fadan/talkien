
inline i32 string_to_i32_(char **at_init)
{
    i32 value = 0;
    char *at = *at_init;
    while ((*at >= '0') && (*at <= '9'))
    {
        value *= 10;
        value += (*at - '0');
        ++at;
    }
    *at_init = at;
    return value;
}

inline i32 string_to_i32(char *at)
{  
    i32 value = string_to_i32_(&at);
    return value;
}

inline b32 strings_are_equal(char *a, char *b)
{
    b32 equals = (a == b);
    if (a && b)
    {
        while (*a && *b && (*a == *b))
        {
            ++a;
            ++b;
        }
        equals = ((*a == 0) && (*b == 0));
    }
    return equals;
}

inline void copy_string(char *src, char *dest, u32 dest_size)
{
    u32 dest_length = 0;
    while (*src && dest_length++ < dest_size)
    {
        *dest++ = *src++;
    }
}

inline void copy_string_and_null_terminate(char *src, char *dest, u32 dest_size)
{
    u32 dest_length = 0;
    while (*src && dest_length++ < dest_size)
    {
        *dest++ = *src++;
    }
    *dest++ = 0;
}

inline u32 string_length(char *string)
{
    u32 length = 0;
    while (*string++)
    {
        ++length;
    }
    return length;
}
