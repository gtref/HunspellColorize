#ifndef HUNCOLOR_PARSER_H
#define HUNCOLOR_PARSER_H

#include <stddef.h>

/*
 * Character classification
 */
int parser_is_letter(unsigned char c);
int parser_is_digit(unsigned char c);
int parser_is_word_char(unsigned char c);

/*
 * Compound words
 */
int parser_is_hyphen(unsigned char c);
int parser_is_apostrophe(unsigned char c);
int parser_is_hyphenated(const char *word, size_t len);

/*
 * CamelCase
 */
int parser_is_camel_transition(unsigned char previous,
                               unsigned char current);
int parser_is_camel_case(const char *word, size_t len);

/*
* Random stuff
*/
int parser_can_continue_hyphen(const char *buf, size_t len);


#endif