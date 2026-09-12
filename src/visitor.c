#include "include/visitor.h"
#include "include/token.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <math.h>

// Forward declarations for array and access visitors
AST_T* visitor_visit_array(visitor_T* visitor, AST_T* node);
AST_T* visitor_visit_access(visitor_T* visitor, AST_T* node);

static char* visitor_read_input(const char* prompt)
{
    char buffer[1024];

    if (prompt)
    {
        printf("%s", prompt);
        fflush(stdout);
    }

    if (!fgets(buffer, sizeof(buffer), stdin))
    {
        fprintf(stderr, "Could not read input\n");
        exit(1);
    }

    buffer[strcspn(buffer, "\r\n")] = '\0';
    return strdup(buffer);
}

static AST_T* visitor_make_string(char* value)
{
    AST_T* ast = calloc(1, sizeof(struct AST_STRUCT));
    ast->type = AST_STRING;
    ast->string_value = value;
    return ast;
}

static AST_T* visitor_make_number(double value)
{
    AST_T* ast = calloc(1, sizeof(struct AST_STRUCT));
    ast->type = AST_NUMBER;
    ast->number_value = value;
    return ast;
}

static AST_T* visitor_make_variable(const char* name)
{
    AST_T* ast = calloc(1, sizeof(struct AST_STRUCT));
    ast->type = AST_VARIABLE;
    ast->variable_name = strdup(name);
    return ast;
}

static AST_T* visitor_make_array(const double* values, size_t size)
{
    AST_T* ast = calloc(1, sizeof(struct AST_STRUCT));
    ast->type = AST_ARRAY;
    ast->array_size = size;
    ast->array_value = malloc(sizeof(AST_T*) * size);
    for (size_t i = 0; i < size; i++) ast->array_value[i] = visitor_make_number(values[i]);
    return ast;
}

static AST_T* visitor_math_argument(visitor_T* visitor, AST_T* node, size_t index)
{
    AST_T* value = visitor_visit(visitor, node->function_call_arguments[index]);
    if (!value || value->type != AST_NUMBER)
    {
        fprintf(stderr, "%s argument %zu must be a number\n", node->function_call_name, index + 1);
        exit(1);
    }
    return value;
}

static int visitor_compare_numbers(const void* left, const void* right)
{
    double left_value = *(const double*) left;
    double right_value = *(const double*) right;
    return (left_value > right_value) - (left_value < right_value);
}

static double* visitor_math_values(visitor_T* visitor, AST_T* node, size_t* size)
{
    if (node->function_call_arguments_size != 1)
    {
        fprintf(stderr, "%s expects exactly one array argument\n", node->function_call_name);
        exit(1);
    }

    AST_T* array = visitor_visit(visitor, node->function_call_arguments[0]);
    if (!array || array->type != AST_ARRAY || array->array_size == 0)
    {
        fprintf(stderr, "%s expects a non-empty numeric array\n", node->function_call_name);
        exit(1);
    }

    double* values = malloc(sizeof(double) * array->array_size);
    for (size_t i = 0; i < array->array_size; i++)
    {
        AST_T* value = array->array_value[i];
        if (!value || value->type != AST_NUMBER)
        {
            fprintf(stderr, "%s array must contain only numbers\n", node->function_call_name);
            exit(1);
        }
        values[i] = value->number_value;
    }

    *size = array->array_size;
    return values;
}

static double* visitor_vector_argument(visitor_T* visitor, AST_T* node, size_t index, size_t* size)
{
    AST_T* array = visitor_visit(visitor, node->function_call_arguments[index]);
    if (!array || array->type != AST_ARRAY || array->array_size == 0)
    {
        fprintf(stderr, "%s argument %zu must be a non-empty numeric vector\n",
            node->function_call_name, index + 1);
        exit(1);
    }

    double* values = malloc(sizeof(double) * array->array_size);
    for (size_t i = 0; i < array->array_size; i++)
    {
        AST_T* value = array->array_value[i];
        if (!value || value->type != AST_NUMBER)
        {
            fprintf(stderr, "%s vector must contain only numbers\n", node->function_call_name);
            exit(1);
        }
        values[i] = value->number_value;
    }

    *size = array->array_size;
    return values;
}

static double* visitor_matrix_argument(
    visitor_T* visitor, AST_T* node, size_t index, size_t* rows, size_t* columns
)
{
    AST_T* matrix = visitor_visit(visitor, node->function_call_arguments[index]);
    if (!matrix || matrix->type != AST_ARRAY || matrix->array_size == 0 ||
        !matrix->array_value[0] || matrix->array_value[0]->type != AST_ARRAY ||
        matrix->array_value[0]->array_size == 0)
    {
        fprintf(stderr, "%s argument %zu must be a non-empty numeric matrix\n",
            node->function_call_name, index + 1);
        exit(1);
    }

    size_t row_count = matrix->array_size;
    size_t column_count = matrix->array_value[0]->array_size;
    double* values = malloc(sizeof(double) * row_count * column_count);
    for (size_t row = 0; row < row_count; row++)
    {
        AST_T* matrix_row = matrix->array_value[row];
        if (!matrix_row || matrix_row->type != AST_ARRAY ||
            matrix_row->array_size != column_count)
        {
            fprintf(stderr, "%s requires rows with matching dimensions\n", node->function_call_name);
            exit(1);
        }
        for (size_t column = 0; column < column_count; column++)
        {
            AST_T* value = matrix_row->array_value[column];
            if (!value || value->type != AST_NUMBER)
            {
                fprintf(stderr, "%s matrix must contain only numbers\n", node->function_call_name);
                exit(1);
            }
            values[row * column_count + column] = value->number_value;
        }
    }

    *rows = row_count;
    *columns = column_count;
    return values;
}

static AST_T* visitor_make_matrix(const double* values, size_t rows, size_t columns)
{
    AST_T* matrix = calloc(1, sizeof(struct AST_STRUCT));
    matrix->type = AST_ARRAY;
    matrix->array_size = rows;
    matrix->array_value = malloc(sizeof(AST_T*) * rows);
    for (size_t row = 0; row < rows; row++)
    {
        matrix->array_value[row] = visitor_make_array(values + row * columns, columns);
    }
    return matrix;
}

static const char* visitor_math_function_name(visitor_T* visitor, AST_T* node, size_t index)
{
    AST_T* argument = node->function_call_arguments[index];
    if (argument->type == AST_VARIABLE) return argument->variable_name;

    AST_T* value = visitor_visit(visitor, node->function_call_arguments[index]);
    if (!value || value->type != AST_STRING)
    {
        fprintf(stderr, "%s function name must be a string\n", node->function_call_name);
        exit(1);
    }
    return value->string_value;
}

static AST_T* visitor_symbolic_binary(int operation, AST_T* left, AST_T* right)
{
    if (operation == TOKEN_STAR)
    {
        if (left->type == AST_NUMBER && left->number_value == 0) return left;
        if (right->type == AST_NUMBER && right->number_value == 0) return right;
        if (left->type == AST_NUMBER && left->number_value == 1) return right;
        if (right->type == AST_NUMBER && right->number_value == 1) return left;
    }
    if (operation == TOKEN_PLUS && left->type == AST_NUMBER && left->number_value == 0) return right;
    if (operation == TOKEN_MINUS && right->type == AST_NUMBER && right->number_value == 0) return left;

    AST_T* node = init_ast(AST_BINOP);
    node->binop_op = operation;
    node->binop_left = left;
    node->binop_right = right;
    return node;
}

static AST_T* visitor_symbolic_call(const char* name, AST_T* argument)
{
    AST_T* node = init_ast(AST_FUNCTION_CALL);
    node->function_call_name = strdup(name);
    node->function_call_arguments_size = 1;
    node->function_call_arguments = malloc(sizeof(AST_T*));
    node->function_call_arguments[0] = argument;
    return node;
}

static AST_T* visitor_symbolic_derivative(AST_T* node, const char* variable)
{
    if (node->type == AST_NUMBER) return visitor_make_number(0);
    if (node->type == AST_VARIABLE)
    {
        return visitor_make_number(strcmp(node->variable_name, variable) == 0 ? 1 : 0);
    }
    if (node->type == AST_BINOP)
    {
        AST_T* left_derivative = visitor_symbolic_derivative(node->binop_left, variable);
        AST_T* right_derivative = visitor_symbolic_derivative(node->binop_right, variable);
        switch (node->binop_op)
        {
            case TOKEN_PLUS: return visitor_symbolic_binary(TOKEN_PLUS, left_derivative, right_derivative);
            case TOKEN_MINUS: return visitor_symbolic_binary(TOKEN_MINUS, left_derivative, right_derivative);
            case TOKEN_STAR:
                return visitor_symbolic_binary(
                    TOKEN_PLUS,
                    visitor_symbolic_binary(TOKEN_STAR, left_derivative, node->binop_right),
                    visitor_symbolic_binary(TOKEN_STAR, node->binop_left, right_derivative)
                );
            case TOKEN_SLASH:
                return visitor_symbolic_binary(
                    TOKEN_SLASH,
                    visitor_symbolic_binary(
                        TOKEN_MINUS,
                        visitor_symbolic_binary(TOKEN_STAR, left_derivative, node->binop_right),
                        visitor_symbolic_binary(TOKEN_STAR, node->binop_left, right_derivative)
                    ),
                    visitor_symbolic_binary(TOKEN_CARET, node->binop_right, visitor_make_number(2))
                );
            case TOKEN_CARET:
                if (node->binop_right->type == AST_NUMBER)
                {
                    double exponent = node->binop_right->number_value;
                    return visitor_symbolic_binary(
                        TOKEN_STAR,
                        visitor_symbolic_binary(
                            TOKEN_STAR,
                            visitor_make_number(exponent),
                            visitor_symbolic_binary(
                                TOKEN_CARET,
                                node->binop_left,
                                visitor_make_number(exponent - 1)
                            )
                        ),
                        left_derivative
                    );
                }
                break;
        }
        return NULL;
    }
    if (node->type == AST_FUNCTION_CALL && node->function_call_arguments_size == 1)
    {
        AST_T* argument = node->function_call_arguments[0];
        AST_T* argument_derivative = visitor_symbolic_derivative(argument, variable);
        if (strcmp(node->function_call_name, "sin") == 0)
        {
            return visitor_symbolic_binary(TOKEN_STAR, visitor_symbolic_call("cos", argument), argument_derivative);
        }
        if (strcmp(node->function_call_name, "cos") == 0)
        {
            return visitor_symbolic_binary(
                TOKEN_STAR,
                visitor_symbolic_binary(TOKEN_MINUS, visitor_make_number(0), visitor_symbolic_call("sin", argument)),
                argument_derivative
            );
        }
        if (strcmp(node->function_call_name, "exp") == 0)
        {
            return visitor_symbolic_binary(TOKEN_STAR, visitor_symbolic_call("exp", argument), argument_derivative);
        }
    }
    return NULL;
}

static AST_T* visitor_symbolic_integral(AST_T* node, const char* variable)
{
    if (node->type == AST_NUMBER)
    {
        return visitor_symbolic_binary(TOKEN_STAR, node, visitor_make_variable(variable));
    }
    if (node->type == AST_VARIABLE && strcmp(node->variable_name, variable) == 0)
    {
        return visitor_symbolic_binary(
            TOKEN_SLASH,
            visitor_symbolic_binary(TOKEN_CARET, node, visitor_make_number(2)),
            visitor_make_number(2)
        );
    }
    if (node->type == AST_BINOP)
    {
        if (node->binop_op == TOKEN_PLUS || node->binop_op == TOKEN_MINUS)
        {
            AST_T* left = visitor_symbolic_integral(node->binop_left, variable);
            AST_T* right = visitor_symbolic_integral(node->binop_right, variable);
            if (!left || !right) return NULL;
            return visitor_symbolic_binary(node->binop_op, left, right);
        }
        if (node->binop_op == TOKEN_STAR && node->binop_left->type == AST_NUMBER)
        {
            AST_T* integral = visitor_symbolic_integral(node->binop_right, variable);
            if (!integral) return NULL;
            return visitor_symbolic_binary(TOKEN_STAR, node->binop_left, integral);
        }
        if (node->binop_op == TOKEN_CARET &&
            node->binop_left->type == AST_VARIABLE &&
            strcmp(node->binop_left->variable_name, variable) == 0 &&
            node->binop_right->type == AST_NUMBER)
        {
            double exponent = node->binop_right->number_value;
            return visitor_symbolic_binary(
                TOKEN_SLASH,
                visitor_symbolic_binary(TOKEN_CARET, node->binop_left, visitor_make_number(exponent + 1)),
                visitor_make_number(exponent + 1)
            );
        }
    }
    if (node->type == AST_FUNCTION_CALL && node->function_call_arguments_size == 1 &&
        node->function_call_arguments[0]->type == AST_VARIABLE &&
        strcmp(node->function_call_arguments[0]->variable_name, variable) == 0)
    {
        if (strcmp(node->function_call_name, "sin") == 0)
        {
            return visitor_symbolic_binary(TOKEN_MINUS, visitor_make_number(0), visitor_symbolic_call("cos", node->function_call_arguments[0]));
        }
        if (strcmp(node->function_call_name, "cos") == 0 || strcmp(node->function_call_name, "exp") == 0)
        {
            return visitor_symbolic_call(node->function_call_name, node->function_call_arguments[0]);
        }
    }
    return NULL;
}

static AST_T* visitor_find_function(visitor_T* visitor, const char* name)
{
    for (size_t i = 0; i < visitor->function_definitions_size; i++)
    {
        if (strcmp(visitor->function_definitions[i]->function_name, name) == 0)
        {
            return visitor->function_definitions[i];
        }
    }
    return NULL;
}

static char* visitor_symbolic_format(AST_T* node)
{
    char buffer[64];
    if (node->type == AST_NUMBER)
    {
        snprintf(buffer, sizeof(buffer), "%g", node->number_value);
        return strdup(buffer);
    }
    if (node->type == AST_VARIABLE) return strdup(node->variable_name);
    if (node->type == AST_FUNCTION_CALL && node->function_call_arguments_size == 1)
    {
        char* argument = visitor_symbolic_format(node->function_call_arguments[0]);
        size_t length = strlen(node->function_call_name) + strlen(argument) + 3;
        char* result = malloc(length);
        snprintf(result, length, "%s(%s)", node->function_call_name, argument);
        free(argument);
        return result;
    }
    if (node->type == AST_BINOP)
    {
        char* left = visitor_symbolic_format(node->binop_left);
        char* right = visitor_symbolic_format(node->binop_right);
        const char* operation = node->binop_op == TOKEN_PLUS ? " + " :
            node->binop_op == TOKEN_MINUS ? " - " :
            node->binop_op == TOKEN_STAR ? " * " :
            node->binop_op == TOKEN_SLASH ? " / " : " ^ ";
        size_t length = strlen(left) + strlen(right) + strlen(operation) + 5;
        char* result = malloc(length);
        snprintf(result, length, "(%s%s%s)", left, operation, right);
        free(left);
        free(right);
        return result;
    }
    return strdup("<?> ");
}

static double visitor_call_math_function(visitor_T* visitor, const char* name, double argument)
{
    AST_T* call = calloc(1, sizeof(struct AST_STRUCT));
    call->type = AST_FUNCTION_CALL;
    call->function_call_name = (char*) name;
    call->function_call_arguments_size = 1;
    call->function_call_arguments = malloc(sizeof(AST_T*));
    call->function_call_arguments[0] = visitor_make_number(argument);

    AST_T* result = visitor_visit_function_call(visitor, call);
    if (!result || result->type != AST_NUMBER)
    {
        fprintf(stderr, "Calculus function '%s' must return a number\n", name);
        exit(1);
    }
    return result->number_value;
}

static void visitor_print_value(AST_T* value)
{
    if (value->type == AST_STRING)
    {
        printf("%s", value->string_value);
    }
    else if (value->type == AST_NUMBER)
    {
        printf("%g", value->number_value);
    }
    else if (value->type == AST_ARRAY)
    {
        printf("[");
        for (size_t i = 0; i < value->array_size; i++)
        {
            if (i > 0) printf(", ");
            visitor_print_value(value->array_value[i]);
        }
        printf("]");
    }
    else
    {
        printf("[Complex Object]");
    }
}

visitor_T* init_visitor()
{
    visitor_T* visitor = calloc(1, sizeof(struct VISITOR_STRUCT));
    visitor->function_definitions = NULL;
    visitor->function_definitions_size = 0;
    visitor->in_return = 0;
    visitor->current_return_value = NULL;
    visitor->current_scope = init_scope(NULL);
    return visitor;
}

scope_T* init_scope(scope_T* parent)
{
    scope_T* scope = calloc(1, sizeof(struct SCOPE_STRUCT));
    scope->variable_definitions = NULL;
    scope->variable_definitions_size = 0;
    scope->parent = parent;
    return scope;
}

AST_T* visitor_visit(visitor_T* visitor, AST_T* node)
{
    if (!node) return NULL;

    switch (node->type)
    {
        case AST_VARIABLE_DEFINITION: return visitor_visit_variable_definition(visitor, node);
        case AST_VARIABLE: return visitor_visit_variable(visitor, node);
        case AST_ASSIGNMENT: return visitor_visit_assignment(visitor, node);
        case AST_FUNCTION_CALL: return visitor_visit_function_call(visitor, node);
        case AST_STRING: return visitor_visit_string(visitor, node);
        case AST_COMPOUND: return visitor_visit_compound(visitor, node);
        case AST_NUMBER: return visitor_visit_number(visitor, node);
        case AST_BINOP: return visitor_visit_binop(visitor, node);
        case AST_IF: return visitor_visit_if(visitor, node);
        case AST_WHILE: return visitor_visit_while(visitor, node);
        case AST_FOR: return visitor_visit_for(visitor, node);
        case AST_FUNCTION: return visitor_visit_function(visitor, node);
        case AST_RETURN: return visitor_visit_return(visitor, node);
        case AST_ARRAY: return visitor_visit_array(visitor, node);
        case AST_ACCESS: return visitor_visit_access(visitor, node);
        case AST_NOOP: return node;
    }

    fprintf(stderr, "Uncaught statement type: %d\n", node->type);
    exit(1);
}

AST_T* visitor_visit_variable_definition(visitor_T* visitor, AST_T* node)
{
    AST_T* evaluated_value = visitor_visit(visitor, node->variable_definition_value);
    
    // Store in current scope
    scope_T* scope = visitor->current_scope;
    scope->variable_definitions_size += 1;
    scope->variable_definitions = realloc(
        scope->variable_definitions,
        sizeof(AST_T*) * scope->variable_definitions_size
    );

    // Create a variable definition copy containing the evaluated value
    AST_T* def = calloc(1, sizeof(struct AST_STRUCT));
    def->type = AST_VARIABLE_DEFINITION;
    def->variable_definition_variable_name = strdup(node->variable_definition_variable_name);
    def->variable_definition_value = evaluated_value;

    scope->variable_definitions[scope->variable_definitions_size - 1] = def;

    return def;
}

AST_T* visitor_visit_variable(visitor_T* visitor, AST_T* node)
{
    scope_T* scope = visitor->current_scope;
    while (scope != NULL)
    {
        for (size_t i = 0; i < scope->variable_definitions_size; i++)
        {
            AST_T* def = scope->variable_definitions[i];
            if (strcmp(def->variable_definition_variable_name, node->variable_name) == 0)
            {
                return visitor_visit(visitor, def->variable_definition_value);
            }
        }
        scope = scope->parent;
    }

    if (strcmp(node->variable_name, "PI") == 0) return visitor_make_number(M_PI);
    if (strcmp(node->variable_name, "E") == 0) return visitor_make_number(M_E);
    if (strcmp(node->variable_name, "TAU") == 0) return visitor_make_number(2.0 * M_PI);

    fprintf(stderr, "Undefined variable '%s'\n", node->variable_name);
    exit(1);
}

AST_T* visitor_visit_assignment(visitor_T* visitor, AST_T* node)
{
    AST_T* evaluated_value = visitor_visit(visitor, node->assignment_value);

    scope_T* scope = visitor->current_scope;
    while (scope != NULL)
    {
        for (size_t i = 0; i < scope->variable_definitions_size; i++)
        {
            AST_T* def = scope->variable_definitions[i];
            if (strcmp(def->variable_definition_variable_name, node->assignment_variable_name) == 0)
            {
                def->variable_definition_value = evaluated_value;
                return evaluated_value;
            }
        }
        scope = scope->parent;
    }

    fprintf(stderr, "Undefined variable for assignment '%s'\n", node->assignment_variable_name);
    exit(1);
}

AST_T* visitor_visit_function_call(visitor_T* visitor, AST_T* node)
{
    // Handle Built-in 'output' or 'print'
    if (strcmp(node->function_call_name, "output") == 0 || strcmp(node->function_call_name, "print") == 0)
    {
        for (size_t i = 0; i < node->function_call_arguments_size; i++)
        {
            AST_T* evaluated_arg = visitor_visit(visitor, node->function_call_arguments[i]);
            if (evaluated_arg)
            {
                visitor_print_value(evaluated_arg);
                printf("\n");
            }
        }
        return node;
    }

    if (strcmp(node->function_call_name, "input") == 0 ||
        strcmp(node->function_call_name, "read") == 0)
    {
        if (node->function_call_arguments_size > 1)
        {
            fprintf(stderr, "%s expects zero or one argument\n", node->function_call_name);
            exit(1);
        }

        char* prompt = NULL;
        if (node->function_call_arguments_size == 1)
        {
            AST_T* evaluated_prompt = visitor_visit(visitor, node->function_call_arguments[0]);
            if (!evaluated_prompt || evaluated_prompt->type != AST_STRING)
            {
                fprintf(stderr, "%s prompt must be a string\n", node->function_call_name);
                exit(1);
            }
            prompt = evaluated_prompt->string_value;
        }

        return visitor_make_string(visitor_read_input(prompt));
    }

    if (strcmp(node->function_call_name, "input_number") == 0)
    {
        if (node->function_call_arguments_size > 1)
        {
            fprintf(stderr, "input_number expects zero or one argument\n");
            exit(1);
        }

        char* prompt = NULL;
        if (node->function_call_arguments_size == 1)
        {
            AST_T* evaluated_prompt = visitor_visit(visitor, node->function_call_arguments[0]);
            if (!evaluated_prompt || evaluated_prompt->type != AST_STRING)
            {
                fprintf(stderr, "input_number prompt must be a string\n");
                exit(1);
            }
            prompt = evaluated_prompt->string_value;
        }

        char* input = visitor_read_input(prompt);
        char* end = NULL;
        errno = 0;
        double number = strtod(input, &end);
        if (end == input || *end != '\0' || errno == ERANGE)
        {
            fprintf(stderr, "Invalid number input\n");
            exit(1);
        }
        free(input);
        return visitor_make_number(number);
    }

    if (strcmp(node->function_call_name, "abs") == 0 ||
        strcmp(node->function_call_name, "sqrt") == 0 ||
        strcmp(node->function_call_name, "cbrt") == 0 ||
        strcmp(node->function_call_name, "exp") == 0 ||
        strcmp(node->function_call_name, "ln") == 0 ||
        strcmp(node->function_call_name, "log") == 0 ||
        strcmp(node->function_call_name, "log10") == 0 ||
        strcmp(node->function_call_name, "log2") == 0 ||
        strcmp(node->function_call_name, "floor") == 0 ||
        strcmp(node->function_call_name, "ceil") == 0 ||
        strcmp(node->function_call_name, "round") == 0 ||
        strcmp(node->function_call_name, "sin") == 0 ||
        strcmp(node->function_call_name, "cos") == 0 ||
        strcmp(node->function_call_name, "tan") == 0 ||
        strcmp(node->function_call_name, "asin") == 0 ||
        strcmp(node->function_call_name, "acos") == 0 ||
        strcmp(node->function_call_name, "atan") == 0 ||
        strcmp(node->function_call_name, "rad") == 0 ||
        strcmp(node->function_call_name, "deg") == 0)
    {
        if (node->function_call_arguments_size != 1)
        {
            fprintf(stderr, "%s expects exactly one argument\n", node->function_call_name);
            exit(1);
        }

        double value = visitor_math_argument(visitor, node, 0)->number_value;
        double result;

        if (strcmp(node->function_call_name, "abs") == 0) result = fabs(value);
        else if (strcmp(node->function_call_name, "sqrt") == 0)
        {
            if (value < 0) { fprintf(stderr, "sqrt domain error\n"); exit(1); }
            result = sqrt(value);
        }
        else if (strcmp(node->function_call_name, "cbrt") == 0) result = cbrt(value);
        else if (strcmp(node->function_call_name, "exp") == 0) result = exp(value);
        else if (strcmp(node->function_call_name, "ln") == 0 || strcmp(node->function_call_name, "log") == 0)
        {
            if (value <= 0) { fprintf(stderr, "%s domain error\n", node->function_call_name); exit(1); }
            result = log(value);
        }
        else if (strcmp(node->function_call_name, "log10") == 0)
        {
            if (value <= 0) { fprintf(stderr, "log10 domain error\n"); exit(1); }
            result = log10(value);
        }
        else if (strcmp(node->function_call_name, "log2") == 0)
        {
            if (value <= 0) { fprintf(stderr, "log2 domain error\n"); exit(1); }
            result = log2(value);
        }
        else if (strcmp(node->function_call_name, "floor") == 0) result = floor(value);
        else if (strcmp(node->function_call_name, "ceil") == 0) result = ceil(value);
        else if (strcmp(node->function_call_name, "round") == 0) result = round(value);
        else if (strcmp(node->function_call_name, "sin") == 0) result = sin(value);
        else if (strcmp(node->function_call_name, "cos") == 0) result = cos(value);
        else if (strcmp(node->function_call_name, "tan") == 0) result = tan(value);
        else if (strcmp(node->function_call_name, "asin") == 0)
        {
            if (value < -1 || value > 1) { fprintf(stderr, "asin domain error\n"); exit(1); }
            result = asin(value);
        }
        else if (strcmp(node->function_call_name, "acos") == 0)
        {
            if (value < -1 || value > 1) { fprintf(stderr, "acos domain error\n"); exit(1); }
            result = acos(value);
        }
        else if (strcmp(node->function_call_name, "atan") == 0) result = atan(value);
        else if (strcmp(node->function_call_name, "rad") == 0) result = value * M_PI / 180.0;
        else result = value * 180.0 / M_PI;

        return visitor_make_number(result);
    }

    if (strcmp(node->function_call_name, "atan2") == 0)
    {
        if (node->function_call_arguments_size != 2)
        {
            fprintf(stderr, "atan2 expects exactly two arguments\n");
            exit(1);
        }

        double y = visitor_math_argument(visitor, node, 0)->number_value;
        double x = visitor_math_argument(visitor, node, 1)->number_value;
        return visitor_make_number(atan2(y, x));
    }

    if (strcmp(node->function_call_name, "pow") == 0)
    {
        if (node->function_call_arguments_size != 2)
        {
            fprintf(stderr, "pow expects exactly two arguments\n");
            exit(1);
        }

        double base = visitor_math_argument(visitor, node, 0)->number_value;
        double exponent = visitor_math_argument(visitor, node, 1)->number_value;
        return visitor_make_number(pow(base, exponent));
    }

    if (strcmp(node->function_call_name, "root") == 0)
    {
        if (node->function_call_arguments_size != 2)
        {
            fprintf(stderr, "root expects a value and a degree\n");
            exit(1);
        }

        double value = visitor_math_argument(visitor, node, 0)->number_value;
        double degree = visitor_math_argument(visitor, node, 1)->number_value;
        if (degree <= 0 || degree != floor(degree))
        {
            fprintf(stderr, "root degree must be a positive integer\n");
            exit(1);
        }
        if (value < 0 && fmod(degree, 2) == 0)
        {
            fprintf(stderr, "even root of a negative number is not real\n");
            exit(1);
        }

        double result = pow(fabs(value), 1.0 / degree);
        if (value < 0) result = -result;
        return visitor_make_number(result);
    }

    if (strcmp(node->function_call_name, "min") == 0 ||
        strcmp(node->function_call_name, "max") == 0)
    {
        if (node->function_call_arguments_size < 2)
        {
            fprintf(stderr, "%s expects at least two arguments\n", node->function_call_name);
            exit(1);
        }

        double result = visitor_math_argument(visitor, node, 0)->number_value;
        for (size_t i = 1; i < node->function_call_arguments_size; i++)
        {
            double value = visitor_math_argument(visitor, node, i)->number_value;
            if ((strcmp(node->function_call_name, "min") == 0 && value < result) ||
                (strcmp(node->function_call_name, "max") == 0 && value > result))
            {
                result = value;
            }
        }
        return visitor_make_number(result);
    }

    if (strcmp(node->function_call_name, "vector") == 0)
    {
        if (node->function_call_arguments_size == 0)
        {
            fprintf(stderr, "vector expects at least one number or numeric array\n");
            exit(1);
        }

        size_t size = node->function_call_arguments_size;
        double* values = NULL;
        if (size == 1)
        {
            AST_T* argument = visitor_visit(visitor, node->function_call_arguments[0]);
            if (argument && argument->type == AST_ARRAY)
            {
                values = visitor_vector_argument(visitor, node, 0, &size);
            }
        }

        if (!values)
        {
            values = malloc(sizeof(double) * size);
            for (size_t i = 0; i < size; i++)
            {
                values[i] = visitor_math_argument(visitor, node, i)->number_value;
            }
        }

        AST_T* result = visitor_make_array(values, size);
        free(values);
        return result;
    }

    if (strcmp(node->function_call_name, "magnitude") == 0 ||
        strcmp(node->function_call_name, "normalize") == 0)
    {
        if (node->function_call_arguments_size != 1)
        {
            fprintf(stderr, "%s expects exactly one vector argument\n", node->function_call_name);
            exit(1);
        }

        size_t size;
        double* values = visitor_vector_argument(visitor, node, 0, &size);
        double magnitude = 0;
        for (size_t i = 0; i < size; i++) magnitude += values[i] * values[i];
        magnitude = sqrt(magnitude);

        if (strcmp(node->function_call_name, "magnitude") == 0)
        {
            free(values);
            return visitor_make_number(magnitude);
        }
        if (magnitude == 0)
        {
            fprintf(stderr, "normalize cannot operate on a zero vector\n");
            exit(1);
        }
        for (size_t i = 0; i < size; i++) values[i] /= magnitude;
        AST_T* result = visitor_make_array(values, size);
        free(values);
        return result;
    }

    if (strcmp(node->function_call_name, "dot") == 0 ||
        strcmp(node->function_call_name, "angle_between") == 0)
    {
        if (node->function_call_arguments_size != 2)
        {
            fprintf(stderr, "%s expects exactly two vector arguments\n", node->function_call_name);
            exit(1);
        }

        size_t left_size;
        size_t right_size;
        double* left = visitor_vector_argument(visitor, node, 0, &left_size);
        double* right = visitor_vector_argument(visitor, node, 1, &right_size);
        if (left_size != right_size)
        {
            fprintf(stderr, "%s requires vectors with matching dimensions\n", node->function_call_name);
            exit(1);
        }

        double dot_product = 0;
        double left_magnitude = 0;
        double right_magnitude = 0;
        for (size_t i = 0; i < left_size; i++)
        {
            dot_product += left[i] * right[i];
            left_magnitude += left[i] * left[i];
            right_magnitude += right[i] * right[i];
        }
        free(left);
        free(right);

        if (strcmp(node->function_call_name, "dot") == 0)
        {
            return visitor_make_number(dot_product);
        }
        double denominator = sqrt(left_magnitude * right_magnitude);
        if (denominator == 0)
        {
            fprintf(stderr, "angle_between cannot use a zero vector\n");
            exit(1);
        }
        double cosine = dot_product / denominator;
        if (cosine > 1) cosine = 1;
        if (cosine < -1) cosine = -1;
        return visitor_make_number(acos(cosine));
    }

    if (strcmp(node->function_call_name, "cross") == 0)
    {
        if (node->function_call_arguments_size != 2)
        {
            fprintf(stderr, "cross expects exactly two vector arguments\n");
            exit(1);
        }

        size_t left_size;
        size_t right_size;
        double* left = visitor_vector_argument(visitor, node, 0, &left_size);
        double* right = visitor_vector_argument(visitor, node, 1, &right_size);
        if (left_size != 3 || right_size != 3)
        {
            fprintf(stderr, "cross only supports three-dimensional vectors\n");
            exit(1);
        }

        double result_values[3] = {
            left[1] * right[2] - left[2] * right[1],
            left[2] * right[0] - left[0] * right[2],
            left[0] * right[1] - left[1] * right[0]
        };
        free(left);
        free(right);
        return visitor_make_array(result_values, 3);
    }

    if (strcmp(node->function_call_name, "matrix") == 0)
    {
        if (node->function_call_arguments_size != 1)
        {
            fprintf(stderr, "matrix expects exactly one nested array argument\n");
            exit(1);
        }
        size_t rows;
        size_t columns;
        double* values = visitor_matrix_argument(visitor, node, 0, &rows, &columns);
        AST_T* result = visitor_make_matrix(values, rows, columns);
        free(values);
        return result;
    }

    if (strcmp(node->function_call_name, "transpose") == 0 ||
        strcmp(node->function_call_name, "det") == 0 ||
        strcmp(node->function_call_name, "inverse") == 0 ||
        strcmp(node->function_call_name, "eigenvalues") == 0)
    {
        if (node->function_call_arguments_size != 1)
        {
            fprintf(stderr, "%s expects exactly one matrix argument\n", node->function_call_name);
            exit(1);
        }

        size_t rows;
        size_t columns;
        double* values = visitor_matrix_argument(visitor, node, 0, &rows, &columns);

        if (strcmp(node->function_call_name, "transpose") == 0)
        {
            double* result_values = malloc(sizeof(double) * rows * columns);
            for (size_t row = 0; row < rows; row++)
            {
                for (size_t column = 0; column < columns; column++)
                {
                    result_values[column * rows + row] = values[row * columns + column];
                }
            }
            AST_T* result = visitor_make_matrix(result_values, columns, rows);
            free(result_values);
            free(values);
            return result;
        }

        if (rows != columns)
        {
            fprintf(stderr, "%s requires a square matrix\n", node->function_call_name);
            exit(1);
        }

        if (strcmp(node->function_call_name, "det") == 0)
        {
            double determinant = 1;
            int sign = 1;
            for (size_t column = 0; column < rows; column++)
            {
                size_t pivot = column;
                for (size_t row = column + 1; row < rows; row++)
                {
                    if (fabs(values[row * columns + column]) >
                        fabs(values[pivot * columns + column])) pivot = row;
                }
                if (fabs(values[pivot * columns + column]) < 1e-12)
                {
                    free(values);
                    return visitor_make_number(0);
                }
                if (pivot != column)
                {
                    for (size_t j = 0; j < columns; j++)
                    {
                        double temp = values[column * columns + j];
                        values[column * columns + j] = values[pivot * columns + j];
                        values[pivot * columns + j] = temp;
                    }
                    sign = -sign;
                }
                double pivot_value = values[column * columns + column];
                determinant *= pivot_value;
                for (size_t row = column + 1; row < rows; row++)
                {
                    double factor = values[row * columns + column] / pivot_value;
                    for (size_t j = column + 1; j < columns; j++)
                    {
                        values[row * columns + j] -= factor * values[column * columns + j];
                    }
                }
            }
            free(values);
            return visitor_make_number(sign * determinant);
        }

        if (strcmp(node->function_call_name, "inverse") == 0)
        {
            double* augmented = calloc(rows * rows * 2, sizeof(double));
            for (size_t row = 0; row < rows; row++)
            {
                for (size_t column = 0; column < rows; column++)
                {
                    augmented[row * rows * 2 + column] = values[row * columns + column];
                    augmented[row * rows * 2 + rows + column] = row == column ? 1 : 0;
                }
            }
            for (size_t column = 0; column < rows; column++)
            {
                size_t pivot = column;
                for (size_t row = column + 1; row < rows; row++)
                {
                    if (fabs(augmented[row * rows * 2 + column]) >
                        fabs(augmented[pivot * rows * 2 + column])) pivot = row;
                }
                if (fabs(augmented[pivot * rows * 2 + column]) < 1e-12)
                {
                    free(augmented);
                    free(values);
                    fprintf(stderr, "inverse requires a non-singular matrix\n");
                    exit(1);
                }
                if (pivot != column)
                {
                    for (size_t j = 0; j < rows * 2; j++)
                    {
                        double temp = augmented[column * rows * 2 + j];
                        augmented[column * rows * 2 + j] = augmented[pivot * rows * 2 + j];
                        augmented[pivot * rows * 2 + j] = temp;
                    }
                }
                double pivot_value = augmented[column * rows * 2 + column];
                for (size_t j = 0; j < rows * 2; j++)
                {
                    augmented[column * rows * 2 + j] /= pivot_value;
                }
                for (size_t row = 0; row < rows; row++)
                {
                    if (row == column) continue;
                    double factor = augmented[row * rows * 2 + column];
                    for (size_t j = 0; j < rows * 2; j++)
                    {
                        augmented[row * rows * 2 + j] -= factor * augmented[column * rows * 2 + j];
                    }
                }
            }
            double* inverse_values = malloc(sizeof(double) * rows * rows);
            for (size_t row = 0; row < rows; row++)
            {
                for (size_t column = 0; column < rows; column++)
                {
                    inverse_values[row * rows + column] = augmented[row * rows * 2 + rows + column];
                }
            }
            AST_T* result = visitor_make_matrix(inverse_values, rows, rows);
            free(inverse_values);
            free(augmented);
            free(values);
            return result;
        }

        if (rows > 2)
        {
            free(values);
            fprintf(stderr, "eigenvalues currently supports 1x1 and 2x2 matrices\n");
            exit(1);
        }
        if (rows == 1)
        {
            double result_value = values[0];
            free(values);
            return visitor_make_array(&result_value, 1);
        }
        double trace = values[0] + values[3];
        double determinant = values[0] * values[3] - values[1] * values[2];
        double discriminant = trace * trace - 4 * determinant;
        if (discriminant < 0)
        {
            free(values);
            fprintf(stderr, "eigenvalues must be real\n");
            exit(1);
        }
        double root = sqrt(discriminant);
        double eigenvalue_values[2] = {(trace - root) / 2, (trace + root) / 2};
        free(values);
        return visitor_make_array(eigenvalue_values, 2);
    }

    if (strcmp(node->function_call_name, "mean") == 0 ||
        strcmp(node->function_call_name, "median") == 0 ||
        strcmp(node->function_call_name, "mode") == 0 ||
        strcmp(node->function_call_name, "variance") == 0 ||
        strcmp(node->function_call_name, "stddev") == 0 ||
        strcmp(node->function_call_name, "sum") == 0 ||
        strcmp(node->function_call_name, "product") == 0)
    {
        size_t size;
        double* values = visitor_math_values(visitor, node, &size);
        double result = 0;

        if (strcmp(node->function_call_name, "sum") == 0 ||
            strcmp(node->function_call_name, "mean") == 0 ||
            strcmp(node->function_call_name, "variance") == 0 ||
            strcmp(node->function_call_name, "stddev") == 0)
        {
            for (size_t i = 0; i < size; i++) result += values[i];
            if (strcmp(node->function_call_name, "mean") == 0 ||
                strcmp(node->function_call_name, "variance") == 0 ||
                strcmp(node->function_call_name, "stddev") == 0)
            {
                result /= size;
            }
        }
        else if (strcmp(node->function_call_name, "product") == 0)
        {
            result = 1;
            for (size_t i = 0; i < size; i++) result *= values[i];
        }
        else if (strcmp(node->function_call_name, "median") == 0)
        {
            qsort(values, size, sizeof(double), visitor_compare_numbers);
            result = size % 2 == 1
                ? values[size / 2]
                : (values[size / 2 - 1] + values[size / 2]) / 2;
        }
        else
        {
            qsort(values, size, sizeof(double), visitor_compare_numbers);
            size_t best_count = 1;
            size_t current_count = 1;
            for (size_t i = 1; i < size; i++)
            {
                if (values[i] == values[i - 1]) current_count++;
                else current_count = 1;
                if (current_count > best_count)
                {
                    best_count = current_count;
                    result = values[i];
                }
            }
            if (best_count == 1) result = values[0];
        }

        if (strcmp(node->function_call_name, "variance") == 0 ||
            strcmp(node->function_call_name, "stddev") == 0)
        {
            double mean = result;
            result = 0;
            for (size_t i = 0; i < size; i++)
            {
                double difference = values[i] - mean;
                result += difference * difference;
            }
            result /= size;
            if (strcmp(node->function_call_name, "stddev") == 0) result = sqrt(result);
        }

        free(values);
        return visitor_make_number(result);
    }

    if (strcmp(node->function_call_name, "derivative") == 0 ||
        strcmp(node->function_call_name, "integral") == 0 ||
        strcmp(node->function_call_name, "limit") == 0)
    {
        size_t argument_count = node->function_call_arguments_size;
        if ((strcmp(node->function_call_name, "derivative") == 0 &&
               (argument_count < 1 || argument_count > 3)) ||
            (strcmp(node->function_call_name, "integral") == 0 &&
               (argument_count < 1 || argument_count > 4)) ||
            (strcmp(node->function_call_name, "limit") == 0 &&
             (argument_count < 2 || argument_count > 3)))
        {
            fprintf(stderr, "%s received an invalid number of arguments\n", node->function_call_name);
            exit(1);
        }

        const char* function_name = visitor_math_function_name(visitor, node, 0);

        if (strcmp(node->function_call_name, "derivative") == 0 && argument_count == 1)
        {
            AST_T* function = visitor_find_function(visitor, function_name);
            if (!function || function->function_parameters_size != 1)
            {
                fprintf(stderr, "derivative requires a one-argument user function\n");
                exit(1);
            }
            AST_T* derivative = visitor_symbolic_derivative(
                function->function_body, function->function_parameters[0]
            );
            if (!derivative)
            {
                fprintf(stderr, "Could not symbolically differentiate '%s'\n", function_name);
                exit(1);
            }
            char* expression = visitor_symbolic_format(derivative);
            AST_T* result = visitor_make_string(expression);
            return result;
        }

        if (strcmp(node->function_call_name, "integral") == 0 && argument_count == 1)
        {
            AST_T* function = visitor_find_function(visitor, function_name);
            if (!function || function->function_parameters_size != 1)
            {
                fprintf(stderr, "integral requires a one-argument user function\n");
                exit(1);
            }
            AST_T* integral = visitor_symbolic_integral(
                function->function_body, function->function_parameters[0]
            );
            if (!integral)
            {
                fprintf(stderr, "Could not symbolically integrate '%s'\n", function_name);
                exit(1);
            }
            char* expression = visitor_symbolic_format(integral);
            size_t length = strlen(expression) + 5;
            char* with_constant = malloc(length);
            snprintf(with_constant, length, "%s + C", expression);
            free(expression);
            return visitor_make_string(with_constant);
        }

        double first_value = visitor_math_argument(visitor, node, 1)->number_value;

        if (strcmp(node->function_call_name, "derivative") == 0)
        {
            double step = argument_count == 3
                ? visitor_math_argument(visitor, node, 2)->number_value
                : 1e-6;
            if (step <= 0)
            {
                fprintf(stderr, "derivative step must be positive\n");
                exit(1);
            }
            double forward = visitor_call_math_function(visitor, function_name, first_value + step);
            double backward = visitor_call_math_function(visitor, function_name, first_value - step);
            return visitor_make_number((forward - backward) / (2 * step));
        }

        if (strcmp(node->function_call_name, "integral") == 0)
        {
            double end = visitor_math_argument(visitor, node, 2)->number_value;
            double steps_value = argument_count == 4
                ? visitor_math_argument(visitor, node, 3)->number_value
                : 1000;
            long steps = (long) steps_value;
            if (steps < 1 || steps_value != steps)
            {
                fprintf(stderr, "integral steps must be a positive integer\n");
                exit(1);
            }

            double width = (end - first_value) / steps;
            double total = (visitor_call_math_function(visitor, function_name, first_value) +
                visitor_call_math_function(visitor, function_name, end)) / 2;
            for (long i = 1; i < steps; i++)
            {
                total += visitor_call_math_function(
                    visitor, function_name, first_value + width * i
                );
            }
            return visitor_make_number(total * width);
        }

        double step = argument_count == 3
            ? visitor_math_argument(visitor, node, 2)->number_value
            : 1e-5;
        if (step <= 0)
        {
            fprintf(stderr, "limit step must be positive\n");
            exit(1);
        }
        double above = visitor_call_math_function(visitor, function_name, first_value + step);
        double below = visitor_call_math_function(visitor, function_name, first_value - step);
        return visitor_make_number((above + below) / 2);
    }

    if (strcmp(node->function_call_name, "solve") == 0)
    {
        size_t argument_count = node->function_call_arguments_size;
        if (argument_count < 3 || argument_count > 4)
        {
            fprintf(stderr, "solve expects an equation, lower bound, upper bound, and optional sample count\n");
            exit(1);
        }

        const char* function_name = visitor_math_function_name(visitor, node, 0);
        AST_T* function = visitor_find_function(visitor, function_name);
        if (!function || function->function_parameters_size != 1)
        {
            fprintf(stderr, "solve requires a one-argument user equation\n");
            exit(1);
        }

        double start = visitor_math_argument(visitor, node, 1)->number_value;
        double end = visitor_math_argument(visitor, node, 2)->number_value;
        double sample_value = argument_count == 4
            ? visitor_math_argument(visitor, node, 3)->number_value
            : 1000;
        long samples = (long) sample_value;
        if (samples < 1 || sample_value != samples)
        {
            fprintf(stderr, "solve sample count must be a positive integer\n");
            exit(1);
        }

        double* roots = malloc(sizeof(double) * (samples + 1));
        size_t root_count = 0;
        double width = (end - start) / samples;
        double previous_x = start;
        double previous_y = visitor_call_math_function(visitor, function_name, previous_x);
        for (long i = 1; i <= samples; i++)
        {
            double current_x = start + width * i;
            double current_y = visitor_call_math_function(visitor, function_name, current_x);
            double root = 0;
            int found_root = 0;

            if (fabs(previous_y) < 1e-10)
            {
                root = previous_x;
                found_root = 1;
            }
            else if (fabs(current_y) < 1e-10)
            {
                root = current_x;
                found_root = 1;
            }
            else if ((previous_y < 0 && current_y > 0) ||
                     (previous_y > 0 && current_y < 0))
            {
                double left = previous_x;
                double right = current_x;
                for (int iteration = 0; iteration < 60; iteration++)
                {
                    double middle = (left + right) / 2;
                    double middle_value = visitor_call_math_function(visitor, function_name, middle);
                    if ((previous_y < 0 && middle_value < 0) ||
                        (previous_y > 0 && middle_value > 0))
                    {
                        left = middle;
                    }
                    else
                    {
                        right = middle;
                    }
                }
                root = (left + right) / 2;
                found_root = 1;
            }

            if (found_root && (root_count == 0 || fabs(root - roots[root_count - 1]) > 1e-7))
            {
                roots[root_count++] = root;
            }
            previous_x = current_x;
            previous_y = current_y;
        }

        AST_T* result = visitor_make_array(roots, root_count);
        free(roots);
        return result;
    }

    // Look up user-defined functions
    AST_T* func = NULL;
    for (size_t i = 0; i < visitor->function_definitions_size; i++)
    {
        if (strcmp(visitor->function_definitions[i]->function_name, node->function_call_name) == 0)
        {
            func = visitor->function_definitions[i];
            break;
        }
    }

    if (!func)
    {
        fprintf(stderr, "Undefined function '%s'\n", node->function_call_name);
        exit(1);
    }

    // Create new scope for function execution
    scope_T* prev_scope = visitor->current_scope;
    scope_T* func_scope = init_scope(prev_scope);
    visitor->current_scope = func_scope;

    // Bind parameters
    for (size_t i = 0; i < func->function_parameters_size; i++)
    {
        AST_T* param_def = calloc(1, sizeof(struct AST_STRUCT));
        param_def->type = AST_VARIABLE_DEFINITION;
        param_def->variable_definition_variable_name = strdup(func->function_parameters[i]);
        param_def->variable_definition_value = visitor_visit(visitor, node->function_call_arguments[i]);

        func_scope->variable_definitions_size += 1;
        func_scope->variable_definitions = realloc(
            func_scope->variable_definitions,
            sizeof(AST_T*) * func_scope->variable_definitions_size
        );
        func_scope->variable_definitions[func_scope->variable_definitions_size - 1] = param_def;
    }

    AST_T* result = visitor_visit(visitor, func->function_body);

    if (visitor->in_return)
    {
        result = visitor->current_return_value;
        visitor->in_return = 0;
        visitor->current_return_value = NULL;
    }

    visitor->current_scope = prev_scope;
    return result;
}

AST_T* visitor_visit_string(visitor_T* visitor, AST_T* node)
{
    return node;
}

AST_T* visitor_visit_compound(visitor_T* visitor, AST_T* node)
{
    scope_T* prev_scope = visitor->current_scope;
    scope_T* compound_scope = init_scope(prev_scope);
    visitor->current_scope = compound_scope;

    AST_T* last_val = NULL;
    for (size_t i = 0; i < node->compound_size; i++)
    {
        last_val = visitor_visit(visitor, node->compound_value[i]);
        if (visitor->in_return) break;
    }

    visitor->current_scope = prev_scope;
    return last_val;
}

AST_T* visitor_visit_number(visitor_T* visitor, AST_T* node)
{
    return node;
}

AST_T* visitor_visit_binop(visitor_T* visitor, AST_T* node)
{
    AST_T* left = visitor_visit(visitor, node->binop_left);
    AST_T* right = visitor_visit(visitor, node->binop_right);

    AST_T* value = calloc(1, sizeof(struct AST_STRUCT));
    value->type = AST_NUMBER;

    if (left->type == AST_NUMBER && right->type == AST_NUMBER)
    {
        double l_val = left->number_value;
        double r_val = right->number_value;

        switch (node->binop_op)
        {
            case TOKEN_PLUS: value->number_value = l_val + r_val; break;
            case TOKEN_MINUS: value->number_value = l_val - r_val; break;
            case TOKEN_STAR: value->number_value = l_val * r_val; break;
            case TOKEN_SLASH:
                if (r_val == 0) { fprintf(stderr, "Division by zero\n"); exit(1); }
                value->number_value = l_val / r_val; 
                break;
            case TOKEN_CARET: value->number_value = pow(l_val, r_val); break;
            case TOKEN_EQ: value->number_value = (l_val == r_val) ? 1 : 0; break;
            case TOKEN_GT: value->number_value = (l_val > r_val) ? 1 : 0; break;
            case TOKEN_LT: value->number_value = (l_val < r_val) ? 1 : 0; break;
            case TOKEN_AND: value->number_value = (l_val != 0 && r_val != 0) ? 1 : 0; break;
            case TOKEN_OR: value->number_value = (l_val != 0 || r_val != 0) ? 1 : 0; break;
            default: break;
        }
    }
    else if (node->binop_op == TOKEN_PLUS &&
             (left->type == AST_STRING || right->type == AST_STRING) &&
             (left->type == AST_STRING || left->type == AST_NUMBER) &&
             (right->type == AST_STRING || right->type == AST_NUMBER))
    {
        char left_number[64];
        char right_number[64];
        const char* left_string = left->type == AST_STRING
            ? left->string_value
            : (snprintf(left_number, sizeof(left_number), "%g", left->number_value), left_number);
        const char* right_string = right->type == AST_STRING
            ? right->string_value
            : (snprintf(right_number, sizeof(right_number), "%g", right->number_value), right_number);
        size_t len = strlen(left_string) + strlen(right_string) + 1;
        char* buf = malloc(len);
        snprintf(buf, len, "%s%s", left_string, right_string);

        value->type = AST_STRING;
        value->string_value = buf;
    }

    return value;
}

AST_T* visitor_visit_if(visitor_T* visitor, AST_T* node)
{
    AST_T* condition = visitor_visit(visitor, node->if_condition);
    if (condition && condition->number_value != 0)
    {
        return visitor_visit(visitor, node->if_body);
    }
    else if (node->if_else_body)
    {
        return visitor_visit(visitor, node->if_else_body);
    }
    return init_ast(AST_NOOP);
}

AST_T* visitor_visit_while(visitor_T* visitor, AST_T* node)
{
    while (1)
    {
        AST_T* condition = visitor_visit(visitor, node->while_condition);
        if (!condition || condition->number_value == 0) break;
        visitor_visit(visitor, node->while_body);
    }
    return init_ast(AST_NOOP);
}

AST_T* visitor_visit_for(visitor_T* visitor, AST_T* node)
{
    scope_T* prev_scope = visitor->current_scope;
    scope_T* for_scope = init_scope(prev_scope);
    visitor->current_scope = for_scope;

    if (node->for_init) visitor_visit(visitor, node->for_init);

    while (1)
    {
        if (node->for_condition)
        {
            AST_T* cond = visitor_visit(visitor, node->for_condition);
            if (!cond || cond->number_value == 0) break;
        }
        
        visitor_visit(visitor, node->for_body);
        if (node->for_increment) visitor_visit(visitor, node->for_increment);
    }

    visitor->current_scope = prev_scope;
    return init_ast(AST_NOOP);
}

AST_T* visitor_visit_function(visitor_T* visitor, AST_T* node)
{
    visitor->function_definitions_size += 1;
    visitor->function_definitions = realloc(
        visitor->function_definitions,
        sizeof(AST_T*) * visitor->function_definitions_size
    );
    visitor->function_definitions[visitor->function_definitions_size - 1] = node;
    return node;
}

AST_T* visitor_visit_return(visitor_T* visitor, AST_T* node)
{
    visitor->in_return = 1;
    visitor->current_return_value = visitor_visit(visitor, node->function_return_value);
    return visitor->current_return_value;
}

AST_T* visitor_visit_array(visitor_T* visitor, AST_T* node)
{
    AST_T* evaluated_array = calloc(1, sizeof(struct AST_STRUCT));
    evaluated_array->type = AST_ARRAY;
    evaluated_array->array_size = node->array_size;
    evaluated_array->array_value = malloc(sizeof(AST_T*) * node->array_size);

    for (size_t i = 0; i < node->array_size; i++)
    {
        evaluated_array->array_value[i] = visitor_visit(visitor, node->array_value[i]);
    }

    return evaluated_array;
}

AST_T* visitor_visit_access(visitor_T* visitor, AST_T* node)
{
    AST_T* target = visitor_visit(visitor, node->access_target);
    AST_T* index = visitor_visit(visitor, node->access_index);

    if (target->type == AST_ARRAY)
    {
        int idx = (int)index->number_value;
        if (idx < 0 || (size_t)idx >= target->array_size)
        {
            fprintf(stderr, "Array index out of bounds\n");
            exit(1);
        }
        return target->array_value[idx];
    }

    fprintf(stderr, "Index access target is not an array\n");
    exit(1);
}