#ifndef TOKEN_H
#define TOKEN_H

typedef enum {
    TOKEN_ID,
    TOKEN_VAR,
    TOKEN_EQUALS,
    TOKEN_EQ,
    TOKEN_LT,
    TOKEN_GT,
    TOKEN_AND,
    TOKEN_OR,
    TOKEN_LPAREN,
    TOKEN_RPAREN,
    TOKEN_LBRACE,
    TOKEN_RBRACE,
    TOKEN_LBRACKET,
    TOKEN_RBRACKET,
    TOKEN_COMMA,
    TOKEN_SEMI,
    TOKEN_PLUS,
    TOKEN_MINUS,
    TOKEN_STAR,
    TOKEN_SLASH,
    TOKEN_CARET,
    TOKEN_NUMBER,
    TOKEN_STRING,
    TOKEN_IF,
    TOKEN_ELSE,
    TOKEN_WHILE,
    TOKEN_FOR,
    TOKEN_FN,
    TOKEN_RETURN,
    TOKEN_EOF
} TOKEN_TYPE;

typedef struct TOKEN_STRUCT
{
    TOKEN_TYPE type;
    char* value;
} token_T;

token_T* init_token(TOKEN_TYPE type, char* value);

#endif