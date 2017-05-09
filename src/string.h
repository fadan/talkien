
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

inline b32 strings_are_equals(char *a, char *b)
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
