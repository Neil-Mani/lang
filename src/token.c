#include "include/token.h"
#include <stdlib.h>

token_T* init_token(TOKEN_TYPE type, char* value)
{
    token_T* token = malloc(sizeof(struct TOKEN_STRUCT));
    token->type = type;
    token->value = value;
    return token;
}