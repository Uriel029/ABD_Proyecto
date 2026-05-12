#include "parser.h"
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

/*
 * mayusculas: Convierte un string a mayusculas (in-place).
 * Se usa para comparar comandos sin importar como los escribio el usuario.
 */
static void mayusculas(char *s) {
    for (int i = 0; s[i]; i++) s[i] = toupper((unsigned char)s[i]);
}

/*
 * extraer_tokens: Divide una linea en tokens (palabras).
 * - Respeta comillas dobles para valores con espacios.
 * - Ej: INSERTAR EN t VALORES ("Hola Mundo" 123)
 *       → tokens: ["INSERTAR", "EN", "t", "VALORES", "Hola Mundo", "123"]
 */
static int extraer_tokens(const char *in, char tok[][MAX_NAME], int max) {
    int n = 0;
    const char *p = in;
    while (*p && n < max) {
        while (*p == ' ') p++;
        if (!*p) break;
        if (*p == '"') {
            p++;
            int ci = 0;
            while (*p && *p != '"' && ci < MAX_NAME - 1) tok[n][ci++] = *p++;
            tok[n][ci] = '\0';
            if (*p == '"') p++;
        } else {
            int ci = 0;
            while (*p && *p != ' ' && ci < MAX_NAME - 1) tok[n][ci++] = *p++;
            tok[n][ci] = '\0';
        }
        n++;
    }
    return n;
}

/*
 * entre_parentesis: Extrae el contenido entre parentesis de un string.
 * Ej: "INSERTAR EN t VALORES (1 Juan 25)" → "1 Juan 25"
 */
static char* entre_parentesis(const char *in, char *buf, int size) {
    const char *a = strchr(in, '(');
    const char *c = strchr(in, ')');
    if (!a || !c || a >= c) { buf[0] = '\0'; return buf; }
    a++;
    int len = c - a;
    if (len >= size) len = size - 1;
    strncpy(buf, a, len);
    buf[len] = '\0';
    return buf;
}

/*
 * parse_command: Interpreta un comando escrito por el usuario.
 *
 * Reconoce estos patrones (todo case-insensitive):
 *
 *   SALIR / SALIR BASE / SALIR BD
 *   AYUDA / INICIAR / CONFIRMAR / CANCELAR / RESPALDAR
 *   CREAR BASE <nombre>
 *   CREAR TABLA <nombre> (<col> <TIPO> [<col> <TIPO>...])
 *   USAR <nombre>
 *   MOSTRAR BASES / MOSTRAR TABLAS
 *   DESCRIBIR <tabla>
 *   INSERTAR EN <tabla> VALORES (<v1> <v2> ...)
 *   SELECCIONAR * DE <tabla> [DONDE <col>=<val>]
 *   ACTUALIZAR <tabla> SET <col>=<val> DONDE <col>=<val>
 *   ELIMINAR DE <tabla> DONDE <col>=<val>
 *   ELIMINAR TABLA <nombre>
 *   ELIMINAR BASE <nombre>
 */
int parse_command(const char *input, ParsedCmd *cmd) {
    memset(cmd, 0, sizeof(ParsedCmd));

    char tokens[15][MAX_NAME];
    int nt = extraer_tokens(input, tokens, 15);
    if (nt == 0) return -1;

    mayusculas(tokens[0]);

    /* ===== COMANDOS DE UNA SOLA PALABRA ===== */
    if (strcmp(tokens[0], "SALIR") == 0) {
        if (nt >= 2) {
            mayusculas(tokens[1]);
            if (strcmp(tokens[1], "BASE") == 0 || strcmp(tokens[1], "BD") == 0) {
                strcpy(cmd->command, "SALIR_BASE");
                return 0;
            }
        }
        strcpy(cmd->command, "SALIR"); return 0;
    }
    if (strcmp(tokens[0], "AYUDA") == 0) {
        strcpy(cmd->command, "AYUDA"); return 0;
    }
    if (strcmp(tokens[0], "INICIAR") == 0) {
        strcpy(cmd->command, "INICIAR"); return 0;
    }
    if (strcmp(tokens[0], "CONFIRMAR") == 0) {
        strcpy(cmd->command, "CONFIRMAR"); return 0;
    }
    if (strcmp(tokens[0], "CANCELAR") == 0) {
        strcpy(cmd->command, "CANCELAR"); return 0;
    }
    if (strcmp(tokens[0], "RESPALDAR") == 0) {
        strcpy(cmd->command, "RESPALDAR"); return 0;
    }

    /* ===== CREAR BASE / CREAR TABLA ===== */
    if (strcmp(tokens[0], "CREAR") == 0 && nt >= 3) {
        mayusculas(tokens[1]);
        if (strcmp(tokens[1], "BASE") == 0) {
            strcpy(cmd->command, "CREAR_BASE");
            strncpy(cmd->dbName, tokens[2], MAX_NAME - 1);
            return 0;
        }
        if (strcmp(tokens[1], "TABLA") == 0 && nt >= 3) {
            strcpy(cmd->command, "CREAR_TABLA");
            strncpy(cmd->tableName, tokens[2], MAX_NAME - 1);

            /* Extraer columnas de los parentesis */
            char paren[MAX_LINE];
            entre_parentesis(input, paren, MAX_LINE);
            if (strlen(paren) == 0) return -1;

            char *p = paren;
            cmd->numColumns = 0;
            while (*p && cmd->numColumns < MAX_COLUMNS) {
                while (*p == ' ') p++;
                if (!*p) break;
                /* Nombre de columna */
                char cname[MAX_NAME]; int ci = 0;
                while (*p && *p != ' ' && *p != ',') cname[ci++] = *p++;
                cname[ci] = '\0';
                if (ci == 0) break;

                /* Tipo de columna */
                while (*p == ' ') p++;
                char ctype[MAX_NAME]; ci = 0;
                while (*p && *p != ' ' && *p != ',') ctype[ci++] = *p++;
                ctype[ci] = '\0';

                if (strlen(ctype) == 0) return -1;

                strncpy(cmd->columns[cmd->numColumns].name, cname, MAX_NAME - 1);
                char upctype[MAX_NAME];
                strncpy(upctype, ctype, MAX_NAME - 1);
                mayusculas(upctype);
                if (strcmp(upctype, "INT") == 0) cmd->columns[cmd->numColumns].type = TYPE_INT;
                else if (strcmp(upctype, "FLOAT") == 0) cmd->columns[cmd->numColumns].type = TYPE_FLOAT;
                else cmd->columns[cmd->numColumns].type = TYPE_STRING;
                cmd->numColumns++;

                while (*p == ' ' || *p == ',') p++;
            }
            return cmd->numColumns > 0 ? 0 : -1;
        }
        return -1;
    }

    /* ===== USAR ===== */
    if (strcmp(tokens[0], "USAR") == 0 && nt >= 2) {
        strcpy(cmd->command, "USAR");
        strncpy(cmd->dbName, tokens[1], MAX_NAME - 1);
        return 0;
    }

    /* ===== MOSTRAR BASES / MOSTRAR TABLAS ===== */
    if (strcmp(tokens[0], "MOSTRAR") == 0 && nt >= 2) {
        mayusculas(tokens[1]);
        if (strcmp(tokens[1], "BASES") == 0) {
            strcpy(cmd->command, "MOSTRAR_BASES"); return 0;
        }
        if (strcmp(tokens[1], "TABLAS") == 0) {
            strcpy(cmd->command, "MOSTRAR_TABLAS"); return 0;
        }
        return -1;
    }

    /* ===== DESCRIBIR ===== */
    if (strcmp(tokens[0], "DESCRIBIR") == 0 && nt >= 2) {
        strcpy(cmd->command, "DESCRIBIR");
        strncpy(cmd->tableName, tokens[1], MAX_NAME - 1);
        return 0;
    }

    /* ===== INSERTAR EN ... VALORES (...) ===== */
    if (strcmp(tokens[0], "INSERTAR") == 0 && nt >= 4) {
        mayusculas(tokens[1]);
        if (strcmp(tokens[1], "EN") != 0) return -1;
        strcpy(cmd->command, "INSERTAR");
        strncpy(cmd->tableName, tokens[2], MAX_NAME - 1);

        mayusculas(tokens[3]);
        if (strcmp(tokens[3], "VALORES") != 0) return -1;

        /* Extraer valores de los parentesis */
        char paren[MAX_LINE];
        entre_parentesis(input, paren, MAX_LINE);

        char *p = paren;
        cmd->numValues = 0;
        while (*p && cmd->numValues < MAX_COLUMNS) {
            while (*p == ' ') p++;
            if (!*p) break;
            if (*p == '"') {
                p++; int ci = 0;
                while (*p && *p != '"' && ci < MAX_VALUE - 1) cmd->values[cmd->numValues][ci++] = *p++;
                cmd->values[cmd->numValues][ci] = '\0';
                if (*p == '"') p++;
            } else {
                int ci = 0;
                while (*p && *p != ' ' && *p != ',' && ci < MAX_VALUE - 1)
                    cmd->values[cmd->numValues][ci++] = *p++;
                cmd->values[cmd->numValues][ci] = '\0';
            }
            cmd->numValues++;
            while (*p == ' ' || *p == ',') p++;
        }
        return cmd->numValues > 0 ? 0 : -1;
    }

    /* ===== SELECCIONAR * DE ... [DONDE col=val] ===== */
    if (strcmp(tokens[0], "SELECCIONAR") == 0 && nt >= 4) {
        if (strcmp(tokens[1], "*") != 0) return -1;
        mayusculas(tokens[2]);
        if (strcmp(tokens[2], "DE") != 0) return -1;
        strcpy(cmd->command, "SELECCIONAR");
        strncpy(cmd->tableName, tokens[3], MAX_NAME - 1);

        for (int i = 4; i < nt; i++) {
            mayusculas(tokens[i]);
            if (strcmp(tokens[i], "DONDE") == 0 && i + 1 < nt) {
                cmd->hasWhere = 1;
                char *eq = strchr(tokens[i + 1], '=');
                if (eq) {
                    *eq = '\0';
                    strncpy(cmd->whereField, tokens[i + 1], MAX_NAME - 1);
                    strncpy(cmd->whereValue, eq + 1, MAX_VALUE - 1);
                }
                break;
            }
        }
        return 0;
    }

    /* ===== ACTUALIZAR ... SET ... DONDE ... ===== */
    if (strcmp(tokens[0], "ACTUALIZAR") == 0 && nt >= 6) {
        strcpy(cmd->command, "ACTUALIZAR");
        strncpy(cmd->tableName, tokens[1], MAX_NAME - 1);

        mayusculas(tokens[2]);
        if (strcmp(tokens[2], "SET") != 0) return -1;

        char *eq = strchr(tokens[3], '=');
        if (!eq) return -1;
        *eq = '\0';
        strncpy(cmd->setField, tokens[3], MAX_NAME - 1);
        strncpy(cmd->setValue, eq + 1, MAX_VALUE - 1);

        for (int i = 4; i < nt; i++) {
            mayusculas(tokens[i]);
            if (strcmp(tokens[i], "DONDE") == 0 && i + 1 < nt) {
                cmd->hasWhere = 1;
                char *eq2 = strchr(tokens[i + 1], '=');
                if (eq2) {
                    *eq2 = '\0';
                    strncpy(cmd->whereField, tokens[i + 1], MAX_NAME - 1);
                    strncpy(cmd->whereValue, eq2 + 1, MAX_VALUE - 1);
                }
                break;
            }
        }
        return 0;
    }

    /* ===== ELIMINAR DE / ELIMINAR TABLA / ELIMINAR BASE ===== */
    if (strcmp(tokens[0], "ELIMINAR") == 0 && nt >= 3) {
        mayusculas(tokens[1]);

        if (strcmp(tokens[1], "DE") == 0) {
            strcpy(cmd->command, "ELIMINAR");
            strncpy(cmd->tableName, tokens[2], MAX_NAME - 1);
            for (int i = 3; i < nt; i++) {
                mayusculas(tokens[i]);
                if (strcmp(tokens[i], "DONDE") == 0 && i + 1 < nt) {
                    cmd->hasWhere = 1;
                    char *eq = strchr(tokens[i + 1], '=');
                    if (eq) {
                        *eq = '\0';
                        strncpy(cmd->whereField, tokens[i + 1], MAX_NAME - 1);
                        strncpy(cmd->whereValue, eq + 1, MAX_VALUE - 1);
                    }
                    break;
                }
            }
            return 0;
        }

        if (strcmp(tokens[1], "TABLA") == 0 && nt >= 3) {
            strcpy(cmd->command, "ELIMINAR_TABLA");
            strncpy(cmd->tableName, tokens[2], MAX_NAME - 1);
            return 0;
        }

        if (strcmp(tokens[1], "BASE") == 0 && nt >= 3) {
            strcpy(cmd->command, "ELIMINAR_BASE");
            strncpy(cmd->dbName, tokens[2], MAX_NAME - 1);
            return 0;
        }
        return -1;
    }

    return -1;
}
