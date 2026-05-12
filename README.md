# Motor de Base de Datos en C

Motor de base de datos **propio** implementado desde cero en C. Sin MySQL, PostgreSQL,
MongoDB ni SQLite. Almacenamiento en archivos de texto plano con interprete de
comandos en espanol, interfaz de terminal web y GUI de escritorio.

**Live demo:** [abd-proyecto.onrender.com](https://abd-proyecto.onrender.com)

---

## Interfaces

### Terminal Web (Flask)
Navegador web tipo PostgreSQL. Corre en Render con Docker.

```bash
cd web
pip install flask gunicorn
python app.py
# Abrir http://localhost:5000
```

### GUI de Escritorio (tkinter)
Interfaz grafica local con Python.

```bash
python3 interfaz.py
```

### Terminal C (CLI)
Consola original del motor.

```bash
./db_engine
```

---

## Estructura del Proyecto

```
ABD_Proyecto/
├── src/
│   ├── main.c          # REPL, manejo de senales (SIGINT)
│   ├── database.c      # Motor de BD completo
│   └── parser.c        # Interprete de comandos en espanol
├── include/
│   ├── database.h      # Estructuras y prototipos
│   └── parser.h        # Prototipo del interprete
├── web/
│   ├── app.py          # Backend Flask
│   ├── requirements.txt
│   └── templates/
│       └── index.html  # Terminal PostgreSQL en HTML+JS
├── data/
│   ├── log.txt         # Registro de operaciones
│   ├── <basedatos>/
│   │   └── <tabla>.dat # Archivo por tabla
│   └── backup/         # Respaldo generado con fork()
├── interfaz.py         # GUI de escritorio (tkinter)
├── Dockerfile          # Build para deploy en Render
├── Makefile
└── README.md
```

---

## Compilacion

```bash
make clean all
```

o manual:

```bash
gcc -Wall -Wextra -Iinclude -std=c11 src/*.c -o db_engine
```

El `Dockerfile` compila automaticamente en Linux para deploy web.

---

## Comandos Disponibles

### Bases de Datos
| Comando | Descripcion |
|---------|-------------|
| `CREAR BASE nombre` | Crea una nueva base de datos |
| `USAR nombre` | Selecciona una base de datos |
| `SALIR BASE` o `SALIR BD` | Sale de la base de datos actual |
| `MOSTRAR BASES` | Lista todas las bases de datos |
| `ELIMINAR BASE nombre` | Elimina una base de datos |

### Tablas
| Comando | Descripcion |
|---------|-------------|
| `CREAR TABLA nombre (col1 TIPO col2 TIPO ...)` | Crea una tabla |
| `MOSTRAR TABLAS` | Lista tablas de la BD actual |
| `DESCRIBIR nombre` | Muestra estructura de una tabla |
| `ELIMINAR TABLA nombre` | Elimina una tabla |

**Tipos disponibles:** INT, FLOAT, STRING

### CRUD (Datos)
| Comando | Descripcion |
|---------|-------------|
| `INSERTAR EN tabla VALORES (v1 v2 ...)` | Inserta un registro |
| `SELECCIONAR * DE tabla [DONDE col=val]` | Consulta registros |
| `ACTUALIZAR tabla SET col=val DONDE col=val` | Actualiza registros |
| `ELIMINAR DE tabla DONDE col=val` | Elimina registros |

### Transacciones
| Comando | Descripcion |
|---------|-------------|
| `INICIAR` | Inicia una transaccion |
| `CONFIRMAR` | Confirma los cambios (COMMIT) |
| `CANCELAR` | Revierte los cambios (ROLLBACK) |

### Otros
| Comando | Descripcion |
|---------|-------------|
| `RESPALDAR` | Crea respaldo usando fork() |
| `AYUDA` | Muestra ayuda de comandos |
| `LIMPIAR` | Limpia la pantalla (solo GUI/Web) |
| `SALIR` | Sale guardando datos |

---

## Ejemplo Completo

```
> CREAR BASE tienda
> USAR tienda
tienda> CREAR TABLA productos (id INT nombre STRING precio FLOAT cantidad INT)
tienda> INSERTAR EN productos VALORES (1 Laptop 15000.50 5)
tienda> INSERTAR EN productos VALORES (2 Mouse 250.99 10)
tienda> INSERTAR EN productos VALORES (3 Teclado 800 3)
tienda> SELECCIONAR * DE productos

+--------------+--------------+--------------+--------------+
| id          | nombre      | precio      | cantidad    |
+--------------+--------------+--------------+--------------+
| 1           | Laptop      | 15000.50    | 5           |
| 2           | Mouse       | 250.99      | 10          |
| 3           | Teclado     | 800         | 3           |
+--------------+--------------+--------------+--------------+
Total: 3 registros

tienda> SELECCIONAR * DE productos DONDE id=1
tienda> ACTUALIZAR productos SET precio=14500 DONDE id=1
tienda> ELIMINAR DE productos DONDE id=2

--- Transaccion con cancelacion ---
tienda> INICIAR
T> tienda> INSERTAR EN productos VALORES (4 Monitor 3500 2)
T> tienda> CANCELAR
tienda> SELECCIONAR * DE productos  (solo 3 registros, el 4 fue revertido)

--- Transaccion con confirmacion ---
tienda> INICIAR
T> tienda> INSERTAR EN productos VALORES (4 Monitor 3500 2)
T> tienda> CONFIRMAR
tienda> SELECCIONAR * DE productos  (4 registros, Monitor ya es permanente)

--- Respaldo con fork ---
tienda> RESPALDAR

--- Multiples bases de datos ---
tienda> CREAR BASE escuela
tienda> USAR escuela
escuela> CREAR TABLA alumnos (id INT nombre STRING grado INT)
escuela> INSERTAR EN alumnos VALORES (1 Juan 5)
escuela> INSERTAR EN alumnos VALORES (2 Maria 6)
escuela> SELECCIONAR * DE alumnos
escuela> SALIR
```

---

## Arquitectura

### Motor C (core)
- **Engine** → **Database[]** → **Table[]** (Columnas + Filas)
- Datos en memoria durante la ejecucion
- Persistencia en archivos `.dat` (CSV con cabecera de tipos)
- Directorio `data/` con una carpeta por base de datos
- Archivo `log.txt` con registro cronologico

### Interfaz Web (Flask + subprocess)
- Cada comando HTTP ejecuta `db_engine` como subproceso
- La sesion (base de datos actual) se rastrea via cookies de Flask
- `USAR <bd>` se antepone automaticamente a comandos que lo requieren
- Precarga de datos de ejemplo durante el build de Docker

### GUI de Escritorio (tkinter)
- Misma logica que la web: subprocess + rastreo de sesion en Python
- Prompt estilo PostgreSQL: `=#` o `basedatos=#`

---

## Propiedades ACID

### Atomicidad
Las transacciones usan un directorio temporal `data/_trans_tmp/` que guarda
el estado de todas las tablas al iniciar la transaccion. Al hacer `CANCELAR`,
se restauran los archivos originales. Al hacer `CONFIRMAR`, se elimina el
directorio temporal.

### Consistencia
- Validacion de tipos (INT, FLOAT, STRING)
- Verificacion de existencia de tablas y columnas
- Verificacion de numero correcto de valores al insertar
- No se permiten operaciones sin seleccionar una base de datos

### Aislamiento
Sistema monousuario. Cada operacion se ejecuta de principio a fin sin
interrupcion. Durante una transaccion, los cambios solo existen en memoria
hasta que se confirman.

### Durabilidad
- `CONFIRMAR` escribe inmediatamente a los archivos `.dat`
- `SALIR` guarda todos los datos antes de terminar
- Ctrl+C dispara el guardado automatico
- `log.txt` provee un registro permanente

---

## Despliegue en Render

1. Subir el proyecto a GitHub
2. En [render.com](https://render.com): **New+** → **Web Service**
3. Conectar repo, elegir **Docker** como runtime
4. Render detecta el `Dockerfile`, compila el motor C, instala Flask y sirve
5. Obtener URL tipo `https://abd-proyecto.onrender.com`

**Nota:** El plan gratis tiene almacenamiento efimero. Los datos creados en
sesion se pierden al reiniciar. El `Dockerfile` precarga una base `ejemplo`
con datos que persisten en la imagen.

---

## Compatibilidad Linux

El proyecto se compila y ejecuta en Linux. Particularidades:

- `d_type` no es confiable en todos los sistemas de archivos Linux
  (ext4, XFS). Se usa `stat()` como fallback via `S_ISDIR()`.
- Se define `_DEFAULT_SOURCE` para exponer POSIX con `-std=c11`.
- Se incluye `sys/wait.h` para `fork()` + `wait()`.
- El `Dockerfile` usa `python:3.11-slim` (Debian) como base.

---

## Archivos de Almacenamiento

Formato de los archivos `.dat`:

```
columna1:TIPO,columna2:TIPO,...
valor1,valor2,valor3,...
```

Ejemplo `data/tienda/productos.dat`:
```
id:INT,nombre:STRING,precio:FLOAT,cantidad:INT
1,Laptop,15000.50,5
2,Mouse,250.99,10
```

Archivos generados:
- `data/log.txt` - Registro cronologico de operaciones
- `data/backup/` - Respaldo completo via fork()

---

## Evidencias Sugeridas

1. Compilacion: `make clean all` sin errores ni warnings
2. CRUD: CREAR BASE, CREAR TABLA, INSERTAR, SELECCIONAR, ACTUALIZAR, ELIMINAR
3. Transaccion CANCELAR: INICIAR + INSERTAR + CANCELAR (datos revertidos)
4. Transaccion CONFIRMAR: INICIAR + INSERTAR + CONFIRMAR (datos persistentes)
5. Respaldo: RESPALDAR + verificar `data/backup/`
6. Persistencia: Ejecutar 2 veces, mostrar que los datos persisten
7. Multi-BD: CREAR BASE, USAR, mostrar cambios de contexto
8. Ctrl+C: Captura de pantalla mostrando cierre seguro
9. Log: `cat data/log.txt` mostrando el registro historico
10. Archivos: `ls -laR data/` mostrando la estructura
11. Web: Captura del navegador en `https://abd-proyecto.onrender.com`
12. GUI: Captura de la ventana de `python3 interfaz.py`

---

## Tecnologias

| Componente | Lenguaje/Tecnologia |
|------------|-------------------|
| Motor de BD | C11 (gcc) |
| Interprete de comandos | C11 (strtok, manual) |
| GUI de escritorio | Python 3 + tkinter |
| Backend web | Python 3 + Flask + gunicorn |
| Frontend web | HTML5 + CSS3 + JavaScript (fetch) |
| Despliegue | Docker + Render |
| Almacenamiento | Archivos `.dat` (CSV) |
| Respaldo | fork() + system() + wait() |
| Senales | signal() + sig_atomic_t |
