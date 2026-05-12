#include "database.h"
#include <ctype.h>
#include <sys/stat.h>
#include <sys/wait.h>

static int type_from_string(const char *s) {
    char up[MAX_NAME]; strncpy(up, s, MAX_NAME - 1);
    for (int i = 0; up[i]; i++) up[i] = toupper((unsigned char)up[i]);
    if (strcmp(up, "INT") == 0) return TYPE_INT;
    if (strcmp(up, "FLOAT") == 0) return TYPE_FLOAT;
    return TYPE_STRING;
}

static const char* type_to_string(ColumnType t) {
    if (t == TYPE_INT) return "INT";
    if (t == TYPE_FLOAT) return "FLOAT";
    return "STRING";
}

void engine_init(Engine *eng, const char *dataDir) {
    eng->numDatabases = 0;
    eng->currentDB = NULL;
    eng->inTransaction = 0;
    strncpy(eng->dataDir, dataDir, MAX_LINE - 1);
    mkdir(dataDir, 0755);
}

int engine_load(Engine *eng) {
    eng->numDatabases = 0;
    eng->currentDB = NULL;

    DIR *dir = opendir(eng->dataDir);
    if (!dir) return -1;

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL && eng->numDatabases < MAX_DATABASES) {
        if (entry->d_name[0] == '.') continue;
        if (strcmp(entry->d_name, "_trans_tmp") == 0) continue;
        int es_dir = (entry->d_type == DT_DIR);
        if (entry->d_type == DT_UNKNOWN) {
            struct stat st;
            char p[MAX_LINE];
            snprintf(p, MAX_LINE, "%s/%s", eng->dataDir, entry->d_name);
            es_dir = (stat(p, &st) == 0 && S_ISDIR(st.st_mode));
        }
        if (!es_dir) continue;

        Database *db = &eng->databases[eng->numDatabases];
        strncpy(db->name, entry->d_name, MAX_NAME - 1);
        snprintf(db->dirpath, MAX_LINE, "%s/%s", eng->dataDir, db->name);
        db->numTables = 0;

        DIR *tdir = opendir(db->dirpath);
        if (!tdir) continue;

        struct dirent *te;
        while ((te = readdir(tdir)) != NULL && db->numTables < MAX_TABLES) {
            if (te->d_name[0] == '.') continue;
            char *dot = strstr(te->d_name, ".dat");
            if (!dot || dot[4] != '\0') continue;

            Table *t = &db->tables[db->numTables];
            *dot = '\0';
            strncpy(t->name, te->d_name, MAX_NAME - 1);
            snprintf(t->filepath, MAX_LINE, "%s/%s.dat", db->dirpath, t->name);

            FILE *fp = fopen(t->filepath, "r");
            if (!fp) continue;

            char line[MAX_LINE];
            if (fgets(line, MAX_LINE, fp)) {
                line[strcspn(line, "\n")] = 0;
                t->numColumns = 0;
                char *tk = strtok(line, ",");
                while (tk && t->numColumns < MAX_COLUMNS) {
                    char *col = strchr(tk, ':');
                    if (col) {
                        *col = '\0';
                        strncpy(t->columns[t->numColumns].name, tk, MAX_NAME - 1);
                        t->columns[t->numColumns].type = type_from_string(col + 1);
                        t->numColumns++;
                    }
                    tk = strtok(NULL, ",");
                }
            }

            t->numRows = 0;
            while (fgets(line, MAX_LINE, fp) && t->numRows < MAX_ROWS) {
                line[strcspn(line, "\n")] = 0;
                if (strlen(line) == 0) continue;
                Row *r = &t->rows[t->numRows];
                int ci = 0;
                char *tk = strtok(line, ",");
                while (tk && ci < t->numColumns) {
                    strncpy(r->values[ci], tk, MAX_VALUE - 1);
                    ci++;
                    tk = strtok(NULL, ",");
                }
                t->numRows++;
            }
            fclose(fp);
            db->numTables++;
        }
        closedir(tdir);
        eng->numDatabases++;
    }
    closedir(dir);
    printf("Cargadas %d base(s) de datos\n", eng->numDatabases);
    return 0;
}

int engine_save_table(Table *t) {
    FILE *fp = fopen(t->filepath, "w");
    if (!fp) return -1;
    for (int i = 0; i < t->numColumns; i++) {
        if (i) fprintf(fp, ",");
        fprintf(fp, "%s:%s", t->columns[i].name, type_to_string(t->columns[i].type));
    }
    fprintf(fp, "\n");
    for (int i = 0; i < t->numRows; i++) {
        for (int j = 0; j < t->numColumns; j++) {
            if (j) fprintf(fp, ",");
            fprintf(fp, "%s", t->rows[i].values[j]);
        }
        fprintf(fp, "\n");
    }
    fclose(fp);
    return 0;
}

int engine_save_all(Engine *eng) {
    for (int i = 0; i < eng->numDatabases; i++)
        for (int j = 0; j < eng->databases[i].numTables; j++)
            engine_save_table(&eng->databases[i].tables[j]);
    return 0;
}

void eng_log(Engine *eng, const char *msg) {
    char path[MAX_LINE];
    snprintf(path, MAX_LINE, "%s/log.txt", eng->dataDir);
    FILE *fp = fopen(path, "a");
    if (!fp) return;
    time_t t = time(NULL);
    struct tm *tm = localtime(&t);
    fprintf(fp, "[%04d-%02d-%02d %02d:%02d:%02d] %s\n",
            tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday,
            tm->tm_hour, tm->tm_min, tm->tm_sec, msg);
    fclose(fp);
}

int eng_crear_base(Engine *eng, const char *name) {
    for (int i = 0; i < eng->numDatabases; i++)
        if (strcmp(eng->databases[i].name, name) == 0) {
            printf("Error: La base de datos '%s' ya existe.\n", name);
            return -1;
        }
    if (eng->numDatabases >= MAX_DATABASES) {
        printf("Error: Maximo de bases de datos alcanzado.\n");
        return -1;
    }
    Database *db = &eng->databases[eng->numDatabases];
    strncpy(db->name, name, MAX_NAME - 1);
    snprintf(db->dirpath, MAX_LINE, "%s/%s", eng->dataDir, name);
    mkdir(db->dirpath, 0755);
    db->numTables = 0;
    eng->numDatabases++;
    printf("Base de datos '%s' creada.\n", name);
    eng_log(eng, "CREAR BASE - Base de datos creada");
    return 0;
}

int eng_usar_base(Engine *eng, const char *name) {
    for (int i = 0; i < eng->numDatabases; i++) {
        if (strcmp(eng->databases[i].name, name) == 0) {
            eng->currentDB = &eng->databases[i];
            printf("Usando base de datos '%s'.\n", name);
            eng_log(eng, "USAR - Base de datos seleccionada");
            return 0;
        }
    }
    printf("Error: La base de datos '%s' no existe.\n", name);
    return -1;
}

int eng_mostrar_bases(Engine *eng) {
    if (eng->numDatabases == 0) {
        printf("No hay bases de datos.\n");
        return 0;
    }
    printf("\n=== BASES DE DATOS ===\n");
    for (int i = 0; i < eng->numDatabases; i++) {
        printf("  %s", eng->databases[i].name);
        if (eng->currentDB && strcmp(eng->databases[i].name, eng->currentDB->name) == 0)
            printf(" (en uso)");
        printf("\n");
    }
    printf("Total: %d\n", eng->numDatabases);
    eng_log(eng, "MOSTRAR BASES");
    return 0;
}

int eng_eliminar_base(Engine *eng, const char *name) {
    int idx = -1;
    for (int i = 0; i < eng->numDatabases; i++) {
        if (strcmp(eng->databases[i].name, name) == 0) { idx = i; break; }
    }
    if (idx == -1) { printf("Error: Base de datos '%s' no existe.\n", name); return -1; }

    if (eng->currentDB == &eng->databases[idx]) eng->currentDB = NULL;

    char cmd[MAX_LINE];
    snprintf(cmd, MAX_LINE, "rm -rf %s", eng->databases[idx].dirpath);
    system(cmd);

    for (int i = idx; i < eng->numDatabases - 1; i++)
        eng->databases[i] = eng->databases[i + 1];
    eng->numDatabases--;

    printf("Base de datos '%s' eliminada.\n", name);
    eng_log(eng, "ELIMINAR BASE - Base de datos eliminada");
    return 0;
}

int eng_crear_tabla(Engine *eng, const char *name, Column *cols, int numCols) {
    if (!eng->currentDB) { printf("Error: No hay base de datos seleccionada. Usa USAR primero.\n"); return -1; }
    for (int i = 0; i < eng->currentDB->numTables; i++)
        if (strcmp(eng->currentDB->tables[i].name, name) == 0) {
            printf("Error: La tabla '%s' ya existe en '%s'.\n", name, eng->currentDB->name);
            return -1;
        }
    if (eng->currentDB->numTables >= MAX_TABLES) {
        printf("Error: Maximo de tablas alcanzado.\n");
        return -1;
    }
    Table *t = &eng->currentDB->tables[eng->currentDB->numTables];
    strncpy(t->name, name, MAX_NAME - 1);
    for (int i = 0; i < numCols; i++) {
        strncpy(t->columns[i].name, cols[i].name, MAX_NAME - 1);
        t->columns[i].type = cols[i].type;
    }
    t->numColumns = numCols;
    t->numRows = 0;
    snprintf(t->filepath, MAX_LINE, "%s/%s.dat", eng->currentDB->dirpath, name);
    eng->currentDB->numTables++;

    if (!eng->inTransaction) engine_save_table(t);

    printf("Tabla '%s' creada en '%s'.\n", name, eng->currentDB->name);
    char logmsg[MAX_LOG_MSG];
    snprintf(logmsg, MAX_LOG_MSG, "CREAR TABLA - Tabla '%s' en '%s'", name, eng->currentDB->name);
    eng_log(eng, logmsg);
    return 0;
}

int eng_mostrar_tablas(Engine *eng) {
    if (!eng->currentDB) { printf("Error: No hay base de datos seleccionada.\n"); return -1; }
    if (eng->currentDB->numTables == 0) {
        printf("No hay tablas en '%s'.\n", eng->currentDB->name);
        return 0;
    }
    printf("\n=== TABLAS EN '%s' ===\n", eng->currentDB->name);
    for (int i = 0; i < eng->currentDB->numTables; i++) {
        printf("  %s (%d columnas, %d registros)\n",
               eng->currentDB->tables[i].name,
               eng->currentDB->tables[i].numColumns,
               eng->currentDB->tables[i].numRows);
    }
    printf("Total: %d tablas\n", eng->currentDB->numTables);
    eng_log(eng, "MOSTRAR TABLAS");
    return 0;
}

int eng_describir_tabla(Engine *eng, const char *name) {
    if (!eng->currentDB) { printf("Error: No hay base de datos seleccionada.\n"); return -1; }
    Table *t = NULL;
    for (int i = 0; i < eng->currentDB->numTables; i++)
        if (strcmp(eng->currentDB->tables[i].name, name) == 0) { t = &eng->currentDB->tables[i]; break; }
    if (!t) { printf("Error: Tabla '%s' no existe en '%s'.\n", name, eng->currentDB->name); return -1; }

    printf("\n=== TABLA '%s' ===\n", name);
    printf("Base de datos: %s\n", eng->currentDB->name);
    printf("Columnas:\n");
    printf("  # | Nombre       | Tipo\n");
    printf("----+--------------+-------\n");
    for (int i = 0; i < t->numColumns; i++) {
        printf("  %d | %-12s | %s\n", i + 1, t->columns[i].name, type_to_string(t->columns[i].type));
    }
    printf("Total: %d columnas, %d registros\n", t->numColumns, t->numRows);
    eng_log(eng, "DESCRIBIR - Estructura de tabla");
    return 0;
}

int eng_eliminar_tabla(Engine *eng, const char *name) {
    if (!eng->currentDB) { printf("Error: No hay base de datos seleccionada.\n"); return -1; }
    int idx = -1;
    for (int i = 0; i < eng->currentDB->numTables; i++)
        if (strcmp(eng->currentDB->tables[i].name, name) == 0) { idx = i; break; }
    if (idx == -1) { printf("Error: Tabla '%s' no existe.\n", name); return -1; }

    remove(eng->currentDB->tables[idx].filepath);
    for (int i = idx; i < eng->currentDB->numTables - 1; i++)
        eng->currentDB->tables[i] = eng->currentDB->tables[i + 1];
    eng->currentDB->numTables--;

    printf("Tabla '%s' eliminada de '%s'.\n", name, eng->currentDB->name);
    char logmsg[MAX_LOG_MSG];
    snprintf(logmsg, MAX_LOG_MSG, "ELIMINAR TABLA - Tabla '%s'", name);
    eng_log(eng, logmsg);
    return 0;
}

int eng_insertar(Engine *eng, const char *tname, char vals[][MAX_VALUE], int nvals) {
    if (!eng->currentDB) { printf("Error: No hay base de datos seleccionada.\n"); return -1; }
    Table *t = NULL;
    for (int i = 0; i < eng->currentDB->numTables; i++)
        if (strcmp(eng->currentDB->tables[i].name, tname) == 0) { t = &eng->currentDB->tables[i]; break; }
    if (!t) { printf("Error: Tabla '%s' no existe.\n", tname); return -1; }
    if (nvals != t->numColumns) {
        printf("Error: La tabla '%s' tiene %d columnas, pero se dieron %d valores.\n",
               tname, t->numColumns, nvals);
        return -1;
    }
    if (t->numRows >= MAX_ROWS) { printf("Error: Maximo de registros alcanzado.\n"); return -1; }

    Row *r = &t->rows[t->numRows];
    for (int i = 0; i < nvals; i++) strncpy(r->values[i], vals[i], MAX_VALUE - 1);
    t->numRows++;

    if (!eng->inTransaction) engine_save_table(t);

    printf("Registro insertado en '%s'.\n", tname);
    char logmsg[MAX_LOG_MSG];
    snprintf(logmsg, MAX_LOG_MSG, "INSERTAR - Registro en '%s'", tname);
    eng_log(eng, logmsg);
    return 0;
}

int eng_seleccionar(Engine *eng, const char *tname, const char *wf, const char *wv) {
    if (!eng->currentDB) { printf("Error: No hay base de datos seleccionada.\n"); return -1; }
    Table *t = NULL;
    for (int i = 0; i < eng->currentDB->numTables; i++)
        if (strcmp(eng->currentDB->tables[i].name, tname) == 0) { t = &eng->currentDB->tables[i]; break; }
    if (!t) { printf("Error: Tabla '%s' no existe.\n", tname); return -1; }

    if (t->numRows == 0) { printf("No hay registros en '%s'.\n", tname); return 0; }

    int wi = -1;
    if (wf && strlen(wf) > 0) {
        for (int i = 0; i < t->numColumns; i++)
            if (strcmp(t->columns[i].name, wf) == 0) { wi = i; break; }
        if (wi == -1) { printf("Error: Columna '%s' no existe.\n", wf); return -1; }
    }

    printf("\n");
    for (int i = 0; i < t->numColumns; i++) {
        printf("+--------------");
    }
    printf("+\n");
    for (int i = 0; i < t->numColumns; i++) {
        printf("| %-12s", t->columns[i].name);
    }
    printf("|\n");
    for (int i = 0; i < t->numColumns; i++) {
        printf("+--------------");
    }
    printf("+\n");

    int count = 0;
    for (int i = 0; i < t->numRows; i++) {
        if (wi >= 0 && strcmp(t->rows[i].values[wi], wv) != 0) continue;
        for (int j = 0; j < t->numColumns; j++) {
            printf("| %-12s", t->rows[i].values[j]);
        }
        printf("|\n");
        count++;
    }
    for (int i = 0; i < t->numColumns; i++) {
        printf("+--------------");
    }
    printf("+\n");
    printf("Total: %d registros\n", count);

    eng_log(eng, "SELECCIONAR - Consulta realizada");
    return 0;
}

int eng_actualizar(Engine *eng, const char *tname, const char *sf, const char *sv, const char *wf, const char *wv) {
    if (!eng->currentDB) { printf("Error: No hay base de datos seleccionada.\n"); return -1; }
    Table *t = NULL;
    for (int i = 0; i < eng->currentDB->numTables; i++)
        if (strcmp(eng->currentDB->tables[i].name, tname) == 0) { t = &eng->currentDB->tables[i]; break; }
    if (!t) { printf("Error: Tabla '%s' no existe.\n", tname); return -1; }

    int si = -1, wi = -1;
    for (int i = 0; i < t->numColumns; i++) {
        if (strcmp(t->columns[i].name, sf) == 0) si = i;
        if (strcmp(t->columns[i].name, wf) == 0) wi = i;
    }
    if (si == -1) { printf("Error: Columna '%s' no existe.\n", sf); return -1; }
    if (!wf || strlen(wf) == 0 || wi == -1) { printf("Error: Falta DONDE o columna no existe.\n"); return -1; }

    int count = 0;
    for (int i = 0; i < t->numRows; i++) {
        if (strcmp(t->rows[i].values[wi], wv) != 0) continue;
        strncpy(t->rows[i].values[si], sv, MAX_VALUE - 1);
        count++;
    }

    if (!eng->inTransaction) engine_save_table(t);

    printf("Actualizados %d registro(s) en '%s'.\n", count, tname);
    char logmsg[MAX_LOG_MSG];
    snprintf(logmsg, MAX_LOG_MSG, "ACTUALIZAR - %d registros en '%s'", count, tname);
    eng_log(eng, logmsg);
    return 0;
}

int eng_eliminar(Engine *eng, const char *tname, const char *wf, const char *wv) {
    if (!eng->currentDB) { printf("Error: No hay base de datos seleccionada.\n"); return -1; }
    Table *t = NULL;
    for (int i = 0; i < eng->currentDB->numTables; i++)
        if (strcmp(eng->currentDB->tables[i].name, tname) == 0) { t = &eng->currentDB->tables[i]; break; }
    if (!t) { printf("Error: Tabla '%s' no existe.\n", tname); return -1; }

    int wi = -1;
    for (int i = 0; i < t->numColumns; i++)
        if (strcmp(t->columns[i].name, wf) == 0) { wi = i; break; }
    if (wi == -1) { printf("Error: Columna '%s' no existe.\n", wf); return -1; }

    int removed = 0;
    for (int i = 0; i < t->numRows; ) {
        if (strcmp(t->rows[i].values[wi], wv) == 0) {
            for (int j = i; j < t->numRows - 1; j++) t->rows[j] = t->rows[j + 1];
            t->numRows--;
            removed++;
        } else {
            i++;
        }
    }

    if (!eng->inTransaction) engine_save_table(t);

    printf("Eliminados %d registro(s) de '%s'.\n", removed, tname);
    char logmsg[MAX_LOG_MSG];
    snprintf(logmsg, MAX_LOG_MSG, "ELIMINAR - %d registros de '%s'", removed, tname);
    eng_log(eng, logmsg);
    return 0;
}

int eng_iniciar(Engine *eng) {
    if (!eng->currentDB) { printf("Error: No hay base de datos seleccionada.\n"); return -1; }
    if (eng->inTransaction) { printf("Ya hay una transaccion activa.\n"); return -1; }

    char tmpDir[MAX_LINE];
    snprintf(tmpDir, MAX_LINE, "%s/_trans_tmp", eng->dataDir);
    mkdir(tmpDir, 0755);

    for (int i = 0; i < eng->currentDB->numTables; i++) {
        Table *t = &eng->currentDB->tables[i];
        char tmpFile[MAX_LINE];
        snprintf(tmpFile, MAX_LINE, "%s/%s.tmp", tmpDir, t->name);

        FILE *src = fopen(t->filepath, "r");
        if (!src) continue;
        FILE *dst = fopen(tmpFile, "w");
        if (!dst) { fclose(src); continue; }

        char c;
        while ((c = fgetc(src)) != EOF) fputc(c, dst);
        fclose(src); fclose(dst);
    }

    eng->inTransaction = 1;
    printf("Transaccion iniciada en '%s'.\n", eng->currentDB->name);
    eng_log(eng, "INICIAR - Transaccion iniciada");
    return 0;
}

int eng_confirmar(Engine *eng) {
    if (!eng->inTransaction) { printf("No hay transaccion activa.\n"); return -1; }

    engine_save_all(eng);

    char tmpDir[MAX_LINE];
    snprintf(tmpDir, MAX_LINE, "%s/_trans_tmp", eng->dataDir);

    DIR *dir = opendir(tmpDir);
    if (dir) {
        struct dirent *e;
        while ((e = readdir(dir)) != NULL) {
            if (e->d_name[0] == '.') continue;
            char fpath[MAX_LINE];
            snprintf(fpath, MAX_LINE, "%s/%s", tmpDir, e->d_name);
            remove(fpath);
        }
        closedir(dir);
    }
    rmdir(tmpDir);

    eng->inTransaction = 0;
    printf("Transaccion confirmada. Cambios permanentes.\n");
    eng_log(eng, "CONFIRMAR - Transaccion confirmada");
    return 0;
}

int eng_cancelar(Engine *eng) {
    if (!eng->inTransaction) { printf("No hay transaccion activa.\n"); return -1; }

    char tmpDir[MAX_LINE];
    snprintf(tmpDir, MAX_LINE, "%s/_trans_tmp", eng->dataDir);

    DIR *dir = opendir(tmpDir);
    if (!dir) { printf("Error: No hay datos de transaccion.\n"); return -1; }

    struct dirent *e;
    while ((e = readdir(dir)) != NULL) {
        if (e->d_name[0] == '.') continue;
        char *dot = strstr(e->d_name, ".tmp");
        if (!dot || dot[4] != '\0') continue;
        *dot = '\0';

        char srcPath[MAX_LINE], dstPath[MAX_LINE];
        snprintf(srcPath, MAX_LINE, "%s/%s.tmp", tmpDir, e->d_name);
        snprintf(dstPath, MAX_LINE, "%s/%s/%s.dat", eng->dataDir, eng->currentDB->name, e->d_name);

        FILE *src = fopen(srcPath, "r");
        if (!src) continue;
        FILE *dst = fopen(dstPath, "w");
        if (!dst) { fclose(src); continue; }

        char c;
        while ((c = fgetc(src)) != EOF) fputc(c, dst);
        fclose(src); fclose(dst);
        remove(srcPath);
    }
    closedir(dir);
    rmdir(tmpDir);

    char savedDB[MAX_NAME];
    strncpy(savedDB, eng->currentDB->name, MAX_NAME - 1);

    engine_load(eng);

    eng->currentDB = NULL;
    for (int i = 0; i < eng->numDatabases; i++)
        if (strcmp(eng->databases[i].name, savedDB) == 0)
            eng->currentDB = &eng->databases[i];

    eng->inTransaction = 0;
    printf("Transaccion cancelada. Datos restaurados.\n");
    eng_log(eng, "CANCELAR - Transaccion cancelada");
    return 0;
}

int eng_respaldar(Engine *eng) {
    printf("Iniciando respaldo...\n");

    pid_t pid = fork();

    if (pid == 0) {
        char cmd[MAX_LINE];
        snprintf(cmd, MAX_LINE, "cp -r %s %s/backup",
                 eng->dataDir, eng->dataDir);
        printf("[Hijo PID=%d] Ejecutando: %s\n", getpid(), cmd);
        system(cmd);
        printf("[Hijo] Respaldo completado.\n");
        _exit(0);
    } else if (pid > 0) {
        int estado;
        wait(&estado);
        if (WIFEXITED(estado) && WEXITSTATUS(estado) == 0) {
            printf("Respaldo exitoso en %s/backup/\n", eng->dataDir);
            eng_log(eng, "RESPALDAR - Respaldo exitoso");
        } else {
            printf("Error en el respaldo.\n");
            eng_log(eng, "RESPALDAR - Error");
        }
    } else {
        printf("Error: fork() fallo.\n");
        return -1;
    }
    return 0;
}
