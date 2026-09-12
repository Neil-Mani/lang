#include "include/lexer.h"
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdio.h>

lexer_T* init_lexer(char* src)
{
    lexer_T* lexer = calloc(1, sizeof(struct LEXER_STRUCT));
    lexer->src = src;
    lexer->src_size = strlen(src);
    lexer->i = 0;
    lexer->c = src[lexer->i];
    return lexer;
}

void lexer_advance(lexer_T* lexer)
{
    if (lexer->i < lexer->src_size && lexer->c != '\0')
    {
        lexer->i += 1;
        lexer->c = lexer->src[lexer->i];
    }
}

token_T* lexer_peek(lexer_T* lexer, int offset)
{
    unsigned int saved_i = lexer->i;
    char saved_c = lexer->c;

    token_T* token = NULL;
    for (int j = 0; j < offset; j++)
    {
        token = lexer_get_next_token(lexer);
    }

    lexer->i = saved_i;
    lexer->c = saved_c;

    return token;
}

void lexer_skip_whitespace(lexer_T* lexer)
{
    while (lexer->c == ' ' || lexer->c == '\t' || lexer->c == '\n' || lexer->c == '\r')
    {
        lexer_advance(lexer);
    }
}

token_T* lexer_get_next_token(lexer_T* lexer)
{
    while (lexer->c != '\0')
    {
        lexer_skip_whitespace(lexer);

        if (lexer->c == '#')
        {
            // Skip comments
            while (lexer->c != '\n' && lexer->c != '\0')
            {
                lexer_advance(lexer);
            }
            continue;
        }

        if (isalpha(lexer->c))
        {
            return lexer_collect_id(lexer);
        }

        if (isdigit(lexer->c) ||
            (lexer->c == '.' && lexer->i + 1 < lexer->src_size &&
             isdigit(lexer->src[lexer->i + 1])))
        {
            char* value = calloc(1, sizeof(char));
            int length = 0;
            int has_decimal = 0;

            while (isdigit(lexer->c) || lexer->c == '.')
            {
                if (lexer->c == '.' && has_decimal)
                {
                    fprintf(stderr, "Invalid number literal\n");
                    exit(1);
                }
                if (lexer->c == '.') has_decimal = 1;
                value = realloc(value, length + 2);
                value[length] = lexer->c;
                length += 1;
                lexer_advance(lexer);
            }
            value[length] = '\0';
            return init_token(TOKEN_NUMBER, value);
        }

        if (lexer->c == '"')
        {
            return lexer_collect_string(lexer);
        }

        switch (lexer->c)
        {
            case '=':
                lexer_advance(lexer);
                if (lexer->c == '=')
                {
                    return lexer_advance_with_token(lexer, init_token(TOKEN_EQ, "=="));
                }
                return init_token(TOKEN_EQUALS, "=");
            
            case '>':
                return lexer_advance_with_token(lexer, init_token(TOKEN_GT, ">"));
            
            case '<':
                return lexer_advance_with_token(lexer, init_token(TOKEN_LT, "<"));
            
            case '&':
                lexer_advance(lexer);
                if (lexer->c == '&') {
                    return lexer_advance_with_token(lexer, init_token(TOKEN_AND, "&&"));
                }
                fprintf(stderr, "Unexpected character: '&'\n");
                exit(1);

            case '|':
                lexer_advance(lexer);
                if (lexer->c == '|') {
                    return lexer_advance_with_token(lexer, init_token(TOKEN_OR, "||"));
                }
                fprintf(stderr, "Unexpected character: '|'\n");
                exit(1);

            case '(': return lexer_advance_with_token(lexer, init_token(TOKEN_LPAREN, "("));
            case ')': return lexer_advance_with_token(lexer, init_token(TOKEN_RPAREN, ")"));
            case '{': return lexer_advance_with_token(lexer, init_token(TOKEN_LBRACE, "{"));
            case '}': return lexer_advance_with_token(lexer, init_token(TOKEN_RBRACE, "}"));
            case '[': return lexer_advance_with_token(lexer, init_token(TOKEN_LBRACKET, "["));
            case ']': return lexer_advance_with_token(lexer, init_token(TOKEN_RBRACKET, "]"));
            case ',': return lexer_advance_with_token(lexer, init_token(TOKEN_COMMA, ","));
            case ';': return lexer_advance_with_token(lexer, init_token(TOKEN_SEMI, ";"));
            case '+': return lexer_advance_with_token(lexer, init_token(TOKEN_PLUS, "+"));
            case '-': return lexer_advance_with_token(lexer, init_token(TOKEN_MINUS, "-"));
            case '*': return lexer_advance_with_token(lexer, init_token(TOKEN_STAR, "*"));
            case '/': return lexer_advance_with_token(lexer, init_token(TOKEN_SLASH, "/"));
            case '^': return lexer_advance_with_token(lexer, init_token(TOKEN_CARET, "^"));
            case '\0': break;
            default:
                fprintf(stderr, "Unexpected character: '%c'\n", lexer->c);
                exit(1);
        }
    }

    return init_token(TOKEN_EOF, "\0");
}

token_T* lexer_collect_id(lexer_T* lexer)
{
    char* value = calloc(1, sizeof(char));
    int length = 0;

    while (isalnum(lexer->c) || lexer->c == '_')
    {
        value = realloc(value, length + 2);
        value[length] = lexer->c;
        length += 1;
        lexer_advance(lexer);
    }
    value[length] = '\0';

    // Handle keywords
    if (strcmp(value, "var") == 0) return init_token(TOKEN_VAR, value);
    if (strcmp(value, "if") == 0) return init_token(TOKEN_IF, value);
    if (strcmp(value, "else") == 0) return init_token(TOKEN_ELSE, value);
    if (strcmp(value, "while") == 0) return init_token(TOKEN_WHILE, value);
    if (strcmp(value, "for") == 0) return init_token(TOKEN_FOR, value);
    if (strcmp(value, "fn") == 0) return init_token(TOKEN_FN, value);
    if (strcmp(value, "return") == 0) return init_token(TOKEN_RETURN, value);

    return init_token(TOKEN_ID, value);
}

token_T* lexer_collect_string(lexer_T* lexer)
{
    lexer_advance(lexer); // Skip opening quote
    char* value = calloc(1, sizeof(char));
    int length = 0;

    while (lexer->c != '"')
    {
        if (lexer->c == '\0')
        {
            fprintf(stderr, "Unterminated string literal\n");
            exit(1);
        }
        value = realloc(value, length + 2);
        value[length] = lexer->c;
        length += 1;
        lexer_advance(lexer);
    }

    lexer_advance(lexer); // Skip closing quote
    value[length] = '\0';
    return init_token(TOKEN_STRING, value);
}

token_T* lexer_advance_with_token(lexer_T* lexer, token_T* token)
{
    lexer_advance(lexer);
    return token;
}