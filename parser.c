#include "parser.h"

#include <ctype.h>

int parser_is_letter(unsigned char c)
{
    return isalpha(c);
}

int parser_is_digit(unsigned char c)
{
    return isdigit(c);
}

int parser_is_word_char(unsigned char c)
{
    return isalpha(c) || isdigit(c) || c == '_';
}

int parser_is_hyphen(unsigned char c)
{
    return c == '-';
}

int parser_is_apostrophe(unsigned char c)
{
    return c == '\'';
}

int parser_is_hyphenated(const char *word, size_t len)
{
    if (!word || len < 3)
        return 0;

    for (size_t i = 1; i + 1 < len; i++) {
        if (word[i] == '-') {
            if (isalpha((unsigned char)word[i - 1]) &&
                isalpha((unsigned char)word[i + 1])) {
                return 1;
            }
        }
    }

    return 0;
}

int parser_is_camel_transition(unsigned char previous,
                               unsigned char current)
{
    return islower(previous) && isupper(current);
}

int parser_is_camel_case(const char *word, size_t len)
{
    if (!word || len < 2)
        return 0;

    for (size_t i = 1; i < len; i++) {
        if (parser_is_camel_transition(
                (unsigned char)word[i - 1],
                (unsigned char)word[i])) {
            return 1;
        }
    }

    return 0;
}

int parser_can_continue_hyphen(const char *buf, size_t len)
{
    if (!buf || len < 2)
        return 0;

    return buf[0] == '-' &&
           isalpha((unsigned char)buf[1]);
}