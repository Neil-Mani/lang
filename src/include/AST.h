#ifndef AST_H
#define AST_H
#include <stdlib.h>

typedef struct AST_STRUCT
{
    enum{
        AST_VARIABLE_DEFINITION,
        AST_VARIABLE,
        AST_ASSIGNMENT,
        AST_FUNCTION_CALL,
        AST_STRING,
        AST_COMPOUND,
        AST_NUMBER,
        AST_BINOP,
        AST_IF,
        AST_WHILE,
        AST_FUNCTION,
        AST_RETURN,
        AST_NOOP,
        AST_FOR,
        AST_ARRAY,
        AST_ACCESS, 
    } type;

    /*AST_VARIABLE_DEFINITION*/
    char* variable_definition_variable_name;
    struct AST_STRUCT* variable_definition_value;

    /*AST_VARIABLE*/
    char* variable_name;

    /*AST_ASSIGNMENT*/
    char* assignment_variable_name;
    struct AST_STRUCT* assignment_value;

    /*AST_FUNCTION_CALL*/
    char* function_call_name;
    struct AST_STRUCT** function_call_arguments;
    size_t function_call_arguments_size;

    /*AST_STRING*/
    char* string_value;

    /*AST_COMPOUND*/
    struct AST_STRUCT** compound_value;
    size_t compound_size;

    /*AST_NUMBER*/
    double number_value;

    /*AST_BINOP*/
    struct AST_STRUCT* binop_left;
    int binop_op; 
    struct AST_STRUCT* binop_right;

    /*AST_IF*/
    struct AST_STRUCT* if_condition;
    struct AST_STRUCT* if_body;
    struct AST_STRUCT* if_else_body;

    /*AST_WHILE*/
    struct AST_STRUCT* while_condition;
    struct AST_STRUCT* while_body;

    /*AST_FUNCTION*/
    char* function_name;
    char** function_parameters;
    size_t function_parameters_size;
    struct AST_STRUCT* function_body;

    /*AST_RETURN*/
    struct AST_STRUCT* function_return_value;

    // For loop fields
    struct AST_STRUCT* for_init;
    struct AST_STRUCT* for_condition;
    struct AST_STRUCT* for_increment;
    struct AST_STRUCT* for_body;

    // Array fields
    struct AST_STRUCT** array_value;
    size_t array_size;

    // Index access fields (e.g., list[0])
    struct AST_STRUCT* access_target;
    struct AST_STRUCT* access_index;
} AST_T;

AST_T* init_ast(int type);
void display_ast(AST_T* type);
#endif