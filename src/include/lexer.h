#ifndef LEXER_H
#define LEXER_H
#include "token.h"
#include <stddef.h>

typedef struct LEXER_STRUCT
{
    char* src;
    size_t src_size;
    char c;
    unsigned int i;
} lexer_T;

lexer_T* init_lexer(char* src);
void lexer_advance(lexer_T* lexer);
token_T* lexer_peek(lexer_T* lexer, int offset);
token_T* lexer_get_next_token(lexer_T* lexer);
token_T* lexer_collect_id(lexer_T* lexer);
token_T* lexer_collect_string(lexer_T* lexer);
token_T* lexer_advance_with_token(lexer_T* lexer, token_T* token);
void lexer_skip_whitespace(lexer_T* lexer);

#endif