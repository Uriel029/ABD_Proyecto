#ifndef DATABASE_H
#define DATABASE_H

#define _DEFAULT_SOURCE 1

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#define MAX_DATABASES 10
#define MAX_TABLES 20
#define MAX_COLUMNS 20
#define MAX_ROWS 1000
#define MAX_NAME 50
#define MAX_LINE 1024
#define MAX_VALUE 256
#define MAX_TRANSACTIONS 100
#define MAX_LOG_MSG 500

typedef enum { TYPE_INT, TYPE_FLOAT, TYPE_STRING } ColumnType;

typedef struct {
    char name[MAX_NAME];
    ColumnType type;
} Column;

typedef struct {
    char values[MAX_COLUMNS][MAX_VALUE];
} Row;

typedef struct {
    char name[MAX_NAME];
    Column columns[MAX_COLUMNS];
    int numColumns;
    Row rows[MAX_ROWS];
    int numRows;
    char filepath[MAX_LINE];
} Table;

typedef struct {
    char name[MAX_NAME];
    Table tables[MAX_TABLES];
    int numTables;
    char dirpath[MAX_LINE];
} Database;

typedef struct {
    Database databases[MAX_DATABASES];
    int numDatabases;
    Database *currentDB;
    int inTransaction;
    char dataDir[MAX_LINE];
} Engine;

void engine_init(Engine *eng, const char *dataDir);
int engine_load(Engine *eng);
int engine_save_table(Table *table);
int engine_save_all(Engine *eng);

int eng_crear_base(Engine *eng, const char *name);
int eng_usar_base(Engine *eng, const char *name);
int eng_mostrar_bases(Engine *eng);
int eng_eliminar_base(Engine *eng, const char *name);

int eng_crear_tabla(Engine *eng, const char *name, Column *cols, int numCols);
int eng_mostrar_tablas(Engine *eng);
int eng_describir_tabla(Engine *eng, const char *name);
int eng_eliminar_tabla(Engine *eng, const char *name);

int eng_insertar(Engine *eng, const char *tableName, char values[][MAX_VALUE], int numValues);
int eng_seleccionar(Engine *eng, const char *tableName, const char *wf, const char *wv);
int eng_actualizar(Engine *eng, const char *tableName, const char *sf, const char *sv, const char *wf, const char *wv);
int eng_eliminar(Engine *eng, const char *tableName, const char *wf, const char *wv);

int eng_iniciar(Engine *eng);
int eng_confirmar(Engine *eng);
int eng_cancelar(Engine *eng);

int eng_respaldar(Engine *eng);
void eng_log(Engine *eng, const char *msg);

#endif
