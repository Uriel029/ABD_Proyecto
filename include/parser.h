#ifndef PARSER_H
#define PARSER_H

#include "database.h"

typedef struct {
    char command[20];
    char dbName[MAX_NAME];
    char tableName[MAX_NAME];
    Column columns[MAX_COLUMNS];
    int numColumns;
    char values[MAX_COLUMNS][MAX_VALUE];
    int numValues;
    char setField[MAX_NAME];
    char setValue[MAX_VALUE];
    char whereField[MAX_NAME];
    char whereValue[MAX_VALUE];
    int hasWhere;
} ParsedCmd;

int parse_command(const char *input, ParsedCmd *cmd);

#endif
