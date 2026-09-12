#ifndef VISITOR_H
#define VISITOR_H
#include "ast.h"

typedef struct SCOPE_STRUCT
{
    AST_T** variable_definitions;
    size_t variable_definitions_size;
    struct SCOPE_STRUCT* parent;
} scope_T;

typedef struct VISITOR_STRUCT
{
    AST_T** function_definitions;
    size_t function_definitions_size;

    int in_return;
    AST_T* current_return_value;

    scope_T* current_scope;
} visitor_T;

visitor_T* init_visitor();
scope_T* init_scope(scope_T* parent);

AST_T* visitor_visit(visitor_T* visitor, AST_T* node);
AST_T* visitor_visit_variable_definition(visitor_T* visitor, AST_T* node);
AST_T* visitor_visit_variable(visitor_T* visitor, AST_T* node);
AST_T* visitor_visit_assignment(visitor_T* visitor, AST_T* node);
AST_T* visitor_visit_function_call(visitor_T* visitor, AST_T* node);
AST_T* visitor_visit_string(visitor_T* visitor, AST_T* node);
AST_T* visitor_visit_compound(visitor_T* visitor, AST_T* node);
AST_T* visitor_visit_number(visitor_T* visitor, AST_T* node);
AST_T* visitor_visit_binop(visitor_T* visitor, AST_T* node);
AST_T* visitor_visit_if(visitor_T* visitor, AST_T* node);
AST_T* visitor_visit_while(visitor_T* visitor, AST_T* node);
AST_T* visitor_visit_for(visitor_T* visitor, AST_T* node);
AST_T* visitor_visit_function(visitor_T* visitor, AST_T* node);
AST_T* visitor_visit_return(visitor_T* visitor, AST_T* node);

// Add these missing declarations to match AST.h types
AST_T* visitor_visit_array(visitor_T* visitor, AST_T* node);
AST_T* visitor_visit_access(visitor_T* visitor, AST_T* node);

#endif