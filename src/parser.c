#include "include/parser.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// Forward declarations
AST_T* parser_parse_compound(parser_T* parser);
AST_T* parser_parse_statement(parser_T* parser);
AST_T* parser_parse_expr(parser_T* parser);
AST_T* parser_parse_term(parser_T* parser);
AST_T* parser_parse_factor(parser_T* parser);
AST_T* parser_parse_variable_definition(parser_T* parser);
AST_T* parser_parse_if(parser_T* parser);
AST_T* parser_parse_while(parser_T* parser);
AST_T* parser_parse_for(parser_T* parser);
AST_T* parser_parse_function(parser_T* parser);
AST_T* parser_parse_return(parser_T* parser);
AST_T* parser_parse_array(parser_T* parser);
AST_T* parser_parse_id(parser_T* parser);
AST_T* parser_parse_function_call(parser_T* parser, char* function_name);
AST_T* parser_parse_number(parser_T* parser);
AST_T* parser_parse_string(parser_T* parser);

parser_T* init_parser(lexer_T* lexer)
{
    parser_T* parser = calloc(1, sizeof(struct PARSER_STRUCT));
    parser->lexer = lexer;
    parser->current_token = lexer_get_next_token(lexer);
    return parser;
}

void parser_eat(parser_T* parser, int token_type)
{
    if (parser->current_token->type == token_type)
    {
        parser->current_token = lexer_get_next_token(parser->lexer);
    }
    else
    {
        printf(
            "Unexpected token '%s', with type %d (Expected type %d)\n",
            parser->current_token->value,
            parser->current_token->type,
            token_type
        );
        exit(1);
    }
}

AST_T* parser_parse(parser_T* parser)
{
    return parser_parse_compound(parser);
}

AST_T* parser_parse_compound(parser_T* parser)
{
    int compound_value_size = 0;
    AST_T** compound_value = calloc(1, sizeof(struct AST_STRUCT*));
    
    AST_T* compound_ast = init_ast(AST_COMPOUND);
    compound_ast->compound_value = compound_value;
    compound_ast->compound_size = 0;

    while (parser->current_token->type != TOKEN_EOF && parser->current_token->type != TOKEN_RBRACE)
    {
        AST_T* ast_statement = parser_parse_statement(parser);
        if (ast_statement)
        {
            compound_value_size += 1;
            compound_value = realloc(compound_value, compound_value_size * sizeof(struct AST_STRUCT*));
            compound_value[compound_value_size - 1] = ast_statement;
            compound_ast->compound_value = compound_value;
            compound_ast->compound_size = compound_value_size;
        }

        if (parser->current_token->type == TOKEN_SEMI)
        {
            parser_eat(parser, TOKEN_SEMI);
        }
    }

    return compound_ast;
}

AST_T* parser_parse_statement(parser_T* parser)
{
    switch (parser->current_token->type)
    {
        case TOKEN_ID:
        {
            // Check if it's an assignment (variable = expr)
            if (strcmp(parser->current_token->value, "var") != 0)
            {
                char* id_value = strdup(parser->current_token->value);
                
                // If the next token is an assignment equals, parse as assignment
                if (lexer_peek(parser->lexer, 1)->type == TOKEN_EQUALS)
                {
                    parser_eat(parser, TOKEN_ID);
                    parser_eat(parser, TOKEN_EQUALS);
                    AST_T* expr = parser_parse_expr(parser);
                    AST_T* ast = init_ast(AST_ASSIGNMENT);
                    ast->assignment_variable_name = id_value;
                    ast->assignment_value = expr;
                    return ast;
                }
            }
            return parser_parse_expr(parser);
        }
        case TOKEN_VAR: return parser_parse_variable_definition(parser);
        case TOKEN_IF: return parser_parse_if(parser);
        case TOKEN_WHILE: return parser_parse_while(parser);
        case TOKEN_FOR: return parser_parse_for(parser);
        case TOKEN_FN: return parser_parse_function(parser);
        case TOKEN_RETURN: return parser_parse_return(parser);
        default: return parser_parse_expr(parser);
    }
}

AST_T* parser_parse_variable_definition(parser_T* parser)
{
    parser_eat(parser, TOKEN_VAR);
    char* variable_name = strdup(parser->current_token->value);
    parser_eat(parser, TOKEN_ID);

    if (parser->current_token->type == TOKEN_LPAREN)
    {
        parser_eat(parser, TOKEN_LPAREN);
        size_t parameters_size = 0;
        char** parameters = calloc(1, sizeof(char*));

        if (parser->current_token->type != TOKEN_RPAREN)
        {
            parameters_size = 1;
            parameters[0] = strdup(parser->current_token->value);
            parser_eat(parser, TOKEN_ID);
            while (parser->current_token->type == TOKEN_COMMA)
            {
                parser_eat(parser, TOKEN_COMMA);
                parameters_size++;
                parameters = realloc(parameters, parameters_size * sizeof(char*));
                parameters[parameters_size - 1] = strdup(parser->current_token->value);
                parser_eat(parser, TOKEN_ID);
            }
        }

        parser_eat(parser, TOKEN_RPAREN);
        parser_eat(parser, TOKEN_EQUALS);

        AST_T* equation = init_ast(AST_FUNCTION);
        equation->function_name = variable_name;
        equation->function_parameters = parameters;
        equation->function_parameters_size = parameters_size;
        equation->function_body = parser_parse_expr(parser);
        return equation;
    }

    parser_eat(parser, TOKEN_EQUALS);
    AST_T* variable_value = parser_parse_expr(parser);

    AST_T* variable_definition = init_ast(AST_VARIABLE_DEFINITION);
    variable_definition->variable_definition_variable_name = variable_name;
    variable_definition->variable_definition_value = variable_value;
    return variable_definition;
}

AST_T* parser_parse_if(parser_T* parser)
{
    parser_eat(parser, TOKEN_IF);
    parser_eat(parser, TOKEN_LPAREN);
    AST_T* condition = parser_parse_expr(parser);
    parser_eat(parser, TOKEN_RPAREN);
    parser_eat(parser, TOKEN_LBRACE);
    AST_T* body = parser_parse_compound(parser);
    parser_eat(parser, TOKEN_RBRACE);

    AST_T* else_body = NULL;
    if (parser->current_token->type == TOKEN_ELSE)
    {
        parser_eat(parser, TOKEN_ELSE);
        parser_eat(parser, TOKEN_LBRACE);
        else_body = parser_parse_compound(parser);
        parser_eat(parser, TOKEN_RBRACE);
    }

    AST_T* ast = init_ast(AST_IF);
    ast->if_condition = condition;
    ast->if_body = body;
    ast->if_else_body = else_body;
    return ast;
}

AST_T* parser_parse_while(parser_T* parser)
{
    parser_eat(parser, TOKEN_WHILE);
    parser_eat(parser, TOKEN_LPAREN);
    AST_T* condition = parser_parse_expr(parser);
    parser_eat(parser, TOKEN_RPAREN);
    parser_eat(parser, TOKEN_LBRACE);
    AST_T* body = parser_parse_compound(parser);
    parser_eat(parser, TOKEN_RBRACE);

    AST_T* ast = init_ast(AST_WHILE);
    ast->while_condition = condition;
    ast->while_body = body;
    return ast;
}

AST_T* parser_parse_for(parser_T* parser)
{
    parser_eat(parser, TOKEN_FOR);
    parser_eat(parser, TOKEN_LPAREN);

    AST_T* init = parser_parse_statement(parser);
    if (parser->current_token->type == TOKEN_SEMI)
    {
        parser_eat(parser, TOKEN_SEMI);
    }

    AST_T* condition = parser_parse_expr(parser);
    parser_eat(parser, TOKEN_SEMI);

    AST_T* increment = parser_parse_statement(parser);
    parser_eat(parser, TOKEN_RPAREN);

    parser_eat(parser, TOKEN_LBRACE);
    AST_T* body = parser_parse_compound(parser);
    parser_eat(parser, TOKEN_RBRACE);

    AST_T* ast = init_ast(AST_FOR);
    ast->for_init = init;
    ast->for_condition = condition;
    ast->for_increment = increment;
    ast->for_body = body;
    return ast;
}

AST_T* parser_parse_function(parser_T* parser)
{
    parser_eat(parser, TOKEN_FN);
    char* function_name = strdup(parser->current_token->value);
    parser_eat(parser, TOKEN_ID);

    parser_eat(parser, TOKEN_LPAREN);

    size_t parameters_size = 0;
    char** parameters = calloc(1, sizeof(char*));

    if (parser->current_token->type != TOKEN_RPAREN)
    {
        parameters_size = 1;
        parameters[0] = strdup(parser->current_token->value);
        parser_eat(parser, TOKEN_ID);

        while (parser->current_token->type == TOKEN_COMMA)
        {
            parser_eat(parser, TOKEN_COMMA);
            parameters_size += 1;
            parameters = realloc(parameters, parameters_size * sizeof(char*));
            parameters[parameters_size - 1] = strdup(parser->current_token->value);
            parser_eat(parser, TOKEN_ID);
        }
    }

    parser_eat(parser, TOKEN_RPAREN);
    parser_eat(parser, TOKEN_LBRACE);

    AST_T* body = parser_parse_compound(parser);

    parser_eat(parser, TOKEN_RBRACE);

    AST_T* ast = init_ast(AST_FUNCTION);
    ast->function_name = function_name;
    ast->function_body = body;
    ast->function_parameters = parameters;
    ast->function_parameters_size = parameters_size;
    return ast;
}

AST_T* parser_parse_return(parser_T* parser)
{
    parser_eat(parser, TOKEN_RETURN);
    if (parser->current_token->type == TOKEN_SEMI)
    {
        AST_T* ast = init_ast(AST_RETURN);
        ast->function_return_value = NULL;
        return ast;
    }

    AST_T* val = parser_parse_expr(parser);
    AST_T* ast = init_ast(AST_RETURN);
    ast->function_return_value = val;
    return ast;
}

AST_T* parser_parse_array(parser_T* parser)
{
    parser_eat(parser, TOKEN_LBRACKET);

    AST_T* ast = init_ast(AST_ARRAY);
    ast->array_value = NULL;
    ast->array_size = 0;

    if (parser->current_token->type != TOKEN_RBRACKET)
    {
        AST_T* item = parser_parse_expr(parser);
        ast->array_size = 1;
        ast->array_value = calloc(1, sizeof(struct AST_STRUCT*));
        ast->array_value[0] = item;

        while (parser->current_token->type == TOKEN_COMMA)
        {
            parser_eat(parser, TOKEN_COMMA);
            AST_T* next_item = parser_parse_expr(parser);
            ast->array_size += 1;
            ast->array_value = realloc(
                ast->array_value,
                ast->array_size * sizeof(struct AST_STRUCT*)
            );
            ast->array_value[ast->array_size - 1] = next_item;
        }
    }

    parser_eat(parser, TOKEN_RBRACKET);
    return ast;
}

AST_T* parser_parse_id(parser_T* parser)
{
    char* token_value = strdup(parser->current_token->value);
    parser_eat(parser, TOKEN_ID);

    if (parser->current_token->type == TOKEN_LPAREN)
    {
        return parser_parse_function_call(parser, token_value);
    }

    AST_T* ast = init_ast(AST_VARIABLE);
    ast->variable_name = token_value;

    if (parser->current_token->type == TOKEN_LBRACKET)
    {
        parser_eat(parser, TOKEN_LBRACKET);
        AST_T* index_expr = parser_parse_expr(parser);
        parser_eat(parser, TOKEN_RBRACKET);

        AST_T* access_ast = init_ast(AST_ACCESS);
        access_ast->access_target = ast;
        access_ast->access_index = index_expr;
        return access_ast;
    }

    return ast;
}

AST_T* parser_parse_function_call(parser_T* parser, char* function_name)
{
    parser_eat(parser, TOKEN_LPAREN);

    AST_T* ast = init_ast(AST_FUNCTION_CALL);
    ast->function_call_name = function_name;

    AST_T** function_call_arguments = calloc(1, sizeof(struct AST_STRUCT*));
    size_t function_call_arguments_size = 0;

    if (parser->current_token->type != TOKEN_RPAREN)
    {
        AST_T* argument = parser_parse_expr(parser);
        function_call_arguments_size += 1;
        function_call_arguments[function_call_arguments_size - 1] = argument;

        while (parser->current_token->type == TOKEN_COMMA)
        {
            parser_eat(parser, TOKEN_COMMA);

            AST_T* next_argument = parser_parse_expr(parser);
            function_call_arguments_size += 1;
            function_call_arguments = realloc(
                function_call_arguments,
                function_call_arguments_size * sizeof(struct AST_STRUCT*)
            );
            function_call_arguments[function_call_arguments_size - 1] = next_argument;
        }
    }

    parser_eat(parser, TOKEN_RPAREN);

    ast->function_call_arguments = function_call_arguments;
    ast->function_call_arguments_size = function_call_arguments_size;

    return ast;
}

AST_T* parser_parse_number(parser_T* parser)
{
    char* value = strdup(parser->current_token->value);
    parser_eat(parser, TOKEN_NUMBER);
    AST_T* ast = init_ast(AST_NUMBER);
    ast->number_value = atof(value);
    return ast;
}

AST_T* parser_parse_string(parser_T* parser)
{
    AST_T* ast = init_ast(AST_STRING);
    ast->string_value = strdup(parser->current_token->value);
    parser_eat(parser, TOKEN_STRING);
    return ast;
}

AST_T* parser_parse_factor(parser_T* parser)
{
    switch (parser->current_token->type)
    {
        case TOKEN_ID: return parser_parse_id(parser);
        case TOKEN_NUMBER: return parser_parse_number(parser);
        case TOKEN_STRING: return parser_parse_string(parser);
        case TOKEN_LPAREN:
        {
            parser_eat(parser, TOKEN_LPAREN);
            AST_T* expr = parser_parse_expr(parser);
            parser_eat(parser, TOKEN_RPAREN);
            return expr;
        }
        case TOKEN_LBRACKET: return parser_parse_array(parser);
        default:
            printf("Unexpected factor token '%s' with type %d\n", parser->current_token->value, parser->current_token->type);
            exit(1);
    }
}

AST_T* parser_parse_term(parser_T* parser)
{
    AST_T* left = parser_parse_factor(parser);

    while (
        parser->current_token->type == TOKEN_STAR ||
        parser->current_token->type == TOKEN_SLASH ||
        parser->current_token->type == TOKEN_CARET
    )
    {
        int op = parser->current_token->type;
        parser_eat(parser, op);
        AST_T* right = parser_parse_factor(parser);

        AST_T* ast = init_ast(AST_BINOP);
        ast->binop_left = left;
        ast->binop_right = right;
        ast->binop_op = op;

        left = ast;
    }

    return left;
}

AST_T* parser_parse_expr(parser_T* parser)
{
    AST_T* left = parser_parse_term(parser);

    while (
        parser->current_token->type == TOKEN_PLUS ||
        parser->current_token->type == TOKEN_MINUS ||
        parser->current_token->type == TOKEN_EQ ||
        parser->current_token->type == TOKEN_LT ||
        parser->current_token->type == TOKEN_GT ||
        parser->current_token->type == TOKEN_AND ||
        parser->current_token->type == TOKEN_OR
    )
    {
        int op = parser->current_token->type;
        parser_eat(parser, op);
        AST_T* right = parser_parse_term(parser);

        AST_T* ast = init_ast(AST_BINOP);
        ast->binop_left = left;
        ast->binop_right = right;
        ast->binop_op = op;

        left = ast;
    }

    return left;
}