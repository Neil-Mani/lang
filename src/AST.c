#include "include/AST.h"
#include "stdio.h"

AST_T* init_ast(int type)
{
    AST_T* ast = calloc(1, sizeof(struct AST_STRUCT));
    ast->type = type;

    ast->variable_definition_variable_name = (void*) 0;
    ast->variable_definition_value = (void*) 0;

    ast->variable_name = (void*) 0;

    /*AST_ASSIGNMENT*/
    ast->assignment_variable_name = (void*) 0;
    ast->assignment_value = (void*) 0;

    ast->function_call_name = (void*) 0;
    ast->function_call_arguments = (void*) 0;
    ast->function_call_arguments_size = 0;

    ast->string_value = (void*) 0;

    ast->compound_value = (void*) 0;
    ast->compound_size = 0;

    ast->number_value = 0;

    ast->binop_left = (void*) 0;
    ast->binop_op = 0;
    ast->binop_right = (void*) 0;

    /* AST_IF */
    ast->if_condition = (void*) 0;
    ast->if_body = (void*) 0;
    ast->if_else_body = (void*) 0;

    /* AST_WHILE */
    ast->while_condition = (void*) 0;
    ast->while_body = (void*) 0;

    return ast;
}

void display_ast(AST_T* node) {
    if (node && node->variable_name)
        printf("%s\n", node->variable_name);
}