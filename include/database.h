#ifndef DATABASE_H
#define DATABASE_H

/* Habilita funciones POSIX (DT_DIR, pid_t, wait) aunque usemos -std=c11 */
#define _DEFAULT_SOURCE 1

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

/* ======================== CONSTANTES ======================== */

#define MAX_DATABASES    10    /* Maximo de bases de datos en memoria */
#define MAX_TABLES       20    /* Maximo de tablas por base */
#define MAX_COLUMNS      20    /* Maximo de columnas por tabla */
#define MAX_ROWS       1000    /* Maximo de registros por tabla */
#define MAX_NAME         50    /* Largo maximo de nombres (BD, tabla, columna) */
#define MAX_LINE       1024    /* Largo maximo de una linea de entrada */
#define MAX_VALUE       256    /* Largo maximo de un valor en una celda */
#define MAX_TRANSACTIONS 100   /* (reservado) */
#define MAX_LOG_MSG     500    /* Largo maximo de mensaje en log */

/* ======================== TIPOS ======================== */

typedef enum {
    TYPE_INT,      /* Numeros enteros */
    TYPE_FLOAT,    /* Numeros decimales */
    TYPE_STRING    /* Texto */
} ColumnType;

/* ======================== ESTRUCTURAS ======================== */

/* Una columna: nombre + tipo de dato */
typedef struct {
    char name[MAX_NAME];
    ColumnType type;
} Column;

/* Una fila: arreglo de valores (uno por columna) */
typedef struct {
    char values[MAX_COLUMNS][MAX_VALUE];
} Row;

/* Una tabla: nombre, columnas, filas, y ruta al archivo .dat */
typedef struct {
    char name[MAX_NAME];
    Column columns[MAX_COLUMNS];
    int numColumns;
    Row rows[MAX_ROWS];
    int numRows;
    char filepath[MAX_LINE];   /* Ruta completa: data/<bd>/<tabla>.dat */
} Table;

/* Una base de datos: nombre, tablas, y ruta a su carpeta */
typedef struct {
    char name[MAX_NAME];
    Table tables[MAX_TABLES];
    int numTables;
    char dirpath[MAX_LINE];    /* Ruta: data/<nombre>/ */
} Database;

/* El motor principal: contiene todas las BD, la BD actual, y estado */
typedef struct {
    Database databases[MAX_DATABASES];
    int numDatabases;
    Database *currentDB;        /* Puntero a la BD seleccionada con USAR */
    int inTransaction;          /* 1 = estamos dentro de una transaccion */
    char dataDir[MAX_LINE];     /* Directorio raiz de datos (normalmente "data") */
} Engine;

/* ======================== PROTOTIPOS ======================== */

/* Inicializacion */
void engine_init(Engine *eng, const char *dataDir);
int engine_load(Engine *eng);
int engine_save_table(Table *table);
int engine_save_all(Engine *eng);

/* Bases de datos */
int eng_crear_base(Engine *eng, const char *name);
int eng_usar_base(Engine *eng, const char *name);
int eng_mostrar_bases(Engine *eng);
int eng_eliminar_base(Engine *eng, const char *name);

/* Tablas */
int eng_crear_tabla(Engine *eng, const char *name, Column *cols, int numCols);
int eng_mostrar_tablas(Engine *eng);
int eng_describir_tabla(Engine *eng, const char *name);
int eng_eliminar_tabla(Engine *eng, const char *name);

/* CRUD */
int eng_insertar(Engine *eng, const char *tableName, char values[][MAX_VALUE], int numValues);
int eng_seleccionar(Engine *eng, const char *tableName, const char *wf, const char *wv);
int eng_actualizar(Engine *eng, const char *tableName, const char *sf, const char *sv, const char *wf, const char *wv);
int eng_eliminar(Engine *eng, const char *tableName, const char *wf, const char *wv);

/* Transacciones */
int eng_iniciar(Engine *eng);
int eng_confirmar(Engine *eng);
int eng_cancelar(Engine *eng);

/* Respaldo y log */
int eng_respaldar(Engine *eng);
void eng_log(Engine *eng, const char *msg);

#endif
