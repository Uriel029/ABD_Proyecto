#ifndef PARSER_H
#define PARSER_H

#include "database.h"

/*
 * ParsedCmd: Resultado de parsear un comando del usuario.
 *
 * command:     Nombre del comando (CREAR_BASE, USAR, INSERTAR, ...)
 * dbName:      Nombre de base de datos (CREAR BASE, USAR, ELIMINAR BASE)
 * tableName:   Nombre de tabla (CREAR TABLA, INSERTAR EN, SELECCIONAR, ...)
 * columns:     Arreglo de columnas (CREAR TABLA)
 * numColumns:  Numero de columnas en el arreglo
 * values:      Arreglo de valores (INSERTAR EN ... VALORES)
 * numValues:   Numero de valores en el arreglo
 * setField:    Columna a actualizar (ACTUALIZAR ... SET)
 * setValue:    Valor nuevo para la columna
 * whereField:  Columna del WHERE
 * whereValue:  Valor del WHERE
 * hasWhere:    1 si el comando incluye DONDE, 0 si no
 */
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

/*
 * parse_command: Interpreta una linea de comando en espanol.
 *
 * Entrada: string escrito por el usuario (ej: "CREAR BASE tienda")
 * Salida: 0 = exito, -1 = comando invalido
 *          La estructura cmd se llena con los campos correspondientes
 *
 * Los comandos se comparan en mayusculas (case-insensitive).
 */
int parse_command(const char *input, ParsedCmd *cmd);

#endif
