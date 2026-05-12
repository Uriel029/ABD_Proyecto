#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include "database.h"
#include "parser.h"

/*
 * main.c: Punto de entrada del motor de base de datos.
 *
 * Funcionamiento:
 * 1. Configura el manejador de SIGINT (Ctrl+C)
 * 2. Inicializa y carga datos desde disco
 * 3. Muestra el banner de bienvenida
 * 4. Entra en un bucle REPL (Read-Eval-Print-Loop)
 *    - Muestra el prompt segun el contexto
 *    - Lee una linea del usuario
 *    - Parsea el comando con parse_command()
 *    - Ejecuta la funcion del motor correspondiente
 * 5. En SALIR o EOF (pipe), guarda datos y termina
 */

/* ==================================================================
 * VARIABLES GLOBALES
 * ================================================================== */

/* Flag para Ctrl+C: volatile porque se modifica desde el manejador
 * de senales, sig_atomic_t porque es seguro para usar en senales */
static volatile sig_atomic_t interrumpido = 0;

/* El motor es global para que el manejador de SIGINT y
 * cleanup_and_exit() puedan acceder a el */
static Engine engine;

/* ==================================================================
 * MANEJADOR DE SENALES
 * ================================================================== */

/*
 * sigint_handler: Se ejecuta cuando el usuario presiona Ctrl+C (SIGINT).
 *
 * Solo establece el flag interrumpido = 1.
 * El ciclo principal detecta el flag y ejecuta el cierre seguro.
 * Esto es importante porque muchas funciones no son "signal-safe".
 */
static void sigint_handler(int sig) {
    (void)sig;
    interrumpido = 1;
}

/* ==================================================================
 * AYUDA
 * ================================================================== */

static void mostrar_ayuda(void) {
    printf("\n============================================\n");
    printf("   MOTOR DE BASE DE DATOS - COMANDOS\n");
    printf("============================================\n");
    printf("  BASES DE DATOS:\n");
    printf("    CREAR BASE <nombre>\n");
    printf("    USAR <nombre>\n");
    printf("    SALIR BASE  (o SALIR BD)\n");
    printf("    MOSTRAR BASES\n");
    printf("    ELIMINAR BASE <nombre>\n\n");
    printf("  TABLAS:\n");
    printf("    CREAR TABLA <nombre> (col1 TIPO col2 TIPO ...)\n");
    printf("    MOSTRAR TABLAS\n");
    printf("    DESCRIBIR <nombre>\n");
    printf("    ELIMINAR TABLA <nombre>\n\n");
    printf("    Tipos: INT, FLOAT, STRING\n\n");
    printf("  DATOS (CRUD):\n");
    printf("    INSERTAR EN <tabla> VALORES (v1 v2 ...)\n");
    printf("    SELECCIONAR * DE <tabla> [DONDE col=val]\n");
    printf("    ACTUALIZAR <tabla> SET col=val DONDE col=val\n");
    printf("    ELIMINAR DE <tabla> DONDE col=val\n\n");
    printf("  TRANSACCIONES:\n");
    printf("    INICIAR / CONFIRMAR / CANCELAR\n\n");
    printf("  OTROS:\n");
    printf("    RESPALDAR / AYUDA / SALIR\n");
    printf("============================================\n\n");
}

/* ==================================================================
 * CIERRE SEGURO
 * ================================================================== */

/*
 * cleanup_and_exit: Guarda todo y termina el programa.
 *
 * Se llama desde:
 * - Comando SALIR
 * - Ctrl+C (SIGINT)
 * - EOF en pipe (redireccion de entrada)
 *
 * Pasos:
 * 1. Si hay transaccion activa, la cancela
 * 2. Guarda todas las tablas a disco
 * 3. Registra en el log
 * 4. Mensaje de despedida
 */
static void cleanup_and_exit(void) {
    if (engine.inTransaction) {
        printf("Cancelando transaccion activa...\n");
        eng_cancelar(&engine);
    }
    engine_save_all(&engine);
    eng_log(&engine, "SALIR - Programa terminado");
    printf("Datos guardados. Hasta luego!\n");
}

/* ==================================================================
 * LECTURA DE ENTRADA
 * ================================================================== */

/*
 * leer_linea: Lee una linea completa desde stdin.
 *
 * - Lee caracter por caracter con fgetc()
 * - Termina en '\n' (salto de linea)
 * - Retorna -1 en EOF (sin datos)
 * - Retorna la cantidad de caracteres leidos (exito)
 *
 * Nota: El prompt se escribe con printf() antes de llamar a esta
 * funcion, y se hace fflush(stdout) para asegurar que se vea.
 */
static int leer_linea(char *buf, int size) {
    int i = 0;
    int c;
    while (i < size - 1) {
        c = fgetc(stdin);
        if (c == EOF) return -1;
        if (c == '\n') break;
        buf[i++] = (char)c;
    }
    buf[i] = '\0';
    return i;
}

/* ==================================================================
 * PROGRAMA PRINCIPAL
 * ================================================================== */

int main(void) {
    /* Configurar manejador de Ctrl+C */
    signal(SIGINT, sigint_handler);

    /* Inicializar motor y cargar datos existentes */
    engine_init(&engine, "data");
    engine_load(&engine);

    /* Banner de bienvenida */
    printf("============================================\n");
    printf("       MI PROPIO MOTOR DE BASE DE DATOS\n");
    printf("============================================\n");
    printf("  Escribe AYUDA para ver los comandos\n");
    printf("  Escribe SALIR para terminar\n");
    printf("============================================\n\n");

    char input[MAX_LINE];

    /* REPL: Read-Eval-Print Loop */
    while (1) {
        /* Verificar si el usuario presiono Ctrl+C */
        if (interrumpido) {
            printf("\nInterrupcion (Ctrl+C) detectada.\n");
            if (engine.inTransaction) {
                printf("Cancelando transaccion...\n");
                eng_cancelar(&engine);
            }
            cleanup_and_exit();
            break;
        }

        /* Mostrar prompt:
         * - "> " cuando no hay BD seleccionada
         * - "tienda> " cuando hay BD seleccionada
         * - "T> tienda> " cuando hay transaccion activa */
        if (engine.currentDB) {
            printf("%s", engine.inTransaction ? "T> " : "");
            printf("%s> ", engine.currentDB->name);
        } else {
            printf("> ");
        }
        fflush(stdout);

        /* Leer comando del usuario */
        if (leer_linea(input, MAX_LINE) < 0) {
            if (interrumpido) continue;
            if (isatty(STDIN_FILENO)) {
                /* Ctrl+D en terminal interactiva: ignorar y seguir */
                printf("\n");
                continue;
            }
            /* EOF en pipe (entrada redirigida): salir */
            cleanup_and_exit();
            break;
        }
        if (strlen(input) == 0) continue;

        /* Parsear el comando */
        ParsedCmd cmd;
        if (parse_command(input, &cmd) != 0) {
            printf("Comando invalido. Escribe AYUDA.\n");
            continue;
        }

        /* Ejecutar segun el comando parseado */
        if (strcmp(cmd.command, "SALIR") == 0) {
            cleanup_and_exit();
            break;
        }

        else if (strcmp(cmd.command, "AYUDA") == 0) {
            mostrar_ayuda();
        }

        else if (strcmp(cmd.command, "CREAR_BASE") == 0) {
            eng_crear_base(&engine, cmd.dbName);
        }

        else if (strcmp(cmd.command, "USAR") == 0) {
            eng_usar_base(&engine, cmd.dbName);
        }

        else if (strcmp(cmd.command, "SALIR_BASE") == 0) {
            if (engine.currentDB) {
                printf("Saliendo de la base de datos '%s'.\n", engine.currentDB->name);
                engine.currentDB = NULL;
            } else {
                printf("No hay base de datos seleccionada.\n");
            }
        }

        else if (strcmp(cmd.command, "MOSTRAR_BASES") == 0) {
            eng_mostrar_bases(&engine);
        }

        else if (strcmp(cmd.command, "MOSTRAR_TABLAS") == 0) {
            eng_mostrar_tablas(&engine);
        }

        else if (strcmp(cmd.command, "CREAR_TABLA") == 0) {
            eng_crear_tabla(&engine, cmd.tableName, cmd.columns, cmd.numColumns);
        }

        else if (strcmp(cmd.command, "DESCRIBIR") == 0) {
            eng_describir_tabla(&engine, cmd.tableName);
        }

        else if (strcmp(cmd.command, "ELIMINAR_TABLA") == 0) {
            eng_eliminar_tabla(&engine, cmd.tableName);
        }

        else if (strcmp(cmd.command, "ELIMINAR_BASE") == 0) {
            eng_eliminar_base(&engine, cmd.dbName);
        }

        else if (strcmp(cmd.command, "INSERTAR") == 0) {
            eng_insertar(&engine, cmd.tableName, cmd.values, cmd.numValues);
        }

        else if (strcmp(cmd.command, "SELECCIONAR") == 0) {
            eng_seleccionar(&engine, cmd.tableName,
                          cmd.hasWhere ? cmd.whereField : "",
                          cmd.hasWhere ? cmd.whereValue : "");
        }

        else if (strcmp(cmd.command, "ACTUALIZAR") == 0) {
            eng_actualizar(&engine, cmd.tableName, cmd.setField, cmd.setValue,
                         cmd.whereField, cmd.whereValue);
        }

        else if (strcmp(cmd.command, "ELIMINAR") == 0) {
            eng_eliminar(&engine, cmd.tableName, cmd.whereField, cmd.whereValue);
        }

        else if (strcmp(cmd.command, "INICIAR") == 0) {
            eng_iniciar(&engine);
        }

        else if (strcmp(cmd.command, "CONFIRMAR") == 0) {
            eng_confirmar(&engine);
        }

        else if (strcmp(cmd.command, "CANCELAR") == 0) {
            eng_cancelar(&engine);
        }

        else if (strcmp(cmd.command, "RESPALDAR") == 0) {
            eng_respaldar(&engine);
        }
    }

    return 0;
}
