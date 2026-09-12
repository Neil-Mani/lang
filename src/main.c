#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "include/lexer.h"
#include "include/parser.h"
#include "include/visitor.h"

char* read_file(const char* filepath) 
{
    FILE* file = fopen(filepath, "r");
    if (!file) 
    {
        fprintf(stderr, "Could not open file '%s'\n", filepath);
        exit(1);
    }

    fseek(file, 0, SEEK_END);
    long length = ftell(file);
    fseek(file, 0, SEEK_SET);

    char* buffer = malloc(length + 1);
    if (!buffer) 
    {
        fprintf(stderr, "Memory allocation error\n");
        exit(1);
    }

    long read_bytes = fread(buffer, 1, length, file);
    buffer[read_bytes] = '\0';

    fclose(file);
    return buffer;
}

int has_alang_extension(const char* filepath)
{
    const char* extension = strrchr(filepath, '.');
    const char expected[] = ".alang";

    if (!extension || strlen(extension) != strlen(expected))
    {
        return 0;
    }

    for (size_t i = 0; expected[i] != '\0'; i++)
    {
        if (tolower((unsigned char) extension[i]) != expected[i])
        {
            return 0;
        }
    }

    return 1;
}

int main(int argc, char* argv[]) 
{
    if (argc < 2) 
    {
        printf("Usage: alang <filename.alang>\n");
        return 1;
    }

    if (!has_alang_extension(argv[1]))
    {
        fprintf(stderr, "Error: alang can only run .alang files\n");
        return 1;
    }

    char* src = read_file(argv[1]);

    lexer_T* lexer = init_lexer(src);
    parser_T* parser = init_parser(lexer);
    AST_T* root = parser_parse(parser);

    // Make sure these two lines are actually here!
    visitor_T* visitor = init_visitor();
    visitor_visit(visitor, root);

    free(src);
    return 0;
}