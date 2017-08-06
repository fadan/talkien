
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

inline char char_uppercase(char c)
{
    if (c >= 'a' && c <= 'z')
    {
        c = c - 'a' + 'A';
    }
    return c;
}

inline i32 string_i_compare(char *a, char *b)
{
    i32 diff = 0;
    while (*a && *b)
    {
        diff = char_uppercase(*b) - char_uppercase(*a);
        if (diff)
        {
            break;
        }

        ++a;
        ++b;
    }
    return diff;
}

inline i32 string_i_compare(char *a, char *b, i32 a_length)
{
    i32 diff = 0;
    while (a_length > 0 && *a && *b)
    {
        diff = char_uppercase(*b) - char_uppercase(*a);
        if (diff)
        {
            break;
        }

        ++a;
        ++b;
        --a_length;
    }
    return diff;
}

// inline char *string_dup(char *string)
// {
//     u32 length = string_length(string) + 1;
//     void *buffer = malloc(length);
//     char *dup = (char *)memcpy(buffer, string, length);
//     return dup;
// }
