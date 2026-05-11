# Proyecto: Motor Propio de Base de Datos en C

## Descripcion

Este proyecto implementa un motor de base de datos **propio** desde cero en lenguaje C.
No utiliza MySQL, PostgreSQL, MongoDB ni SQLite. Los datos se almacenan en archivos
de texto plano y se gestionan a traves de un interprete de comandos en espanol.

### Caracteristicas principales
- Multiples bases de datos
- Multiples tablas por base de datos
- Tipos de datos: INT, FLOAT, STRING
- CRUD completo (Crear, Leer, Actualizar, Eliminar)
- Transacciones con propiedades ACID
- Manejo de interrupciones (Ctrl+C)
- Procesos con fork() para respaldos
- Archivo de log de operaciones
- Interprete de comandos en espanol

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
├── data/
│   ├── log.txt         # Registro de operaciones
│   ├── <basedatos>/    # Directorio por base de datos
│   │   └── <tabla>.dat # Archivo por tabla
│   └── backup/         # Respaldo generado con fork()
├── Makefile
└── README.md
```

---

## Compilacion

```bash
cd ABD_Proyecto
make clean all
```

Para compilar manualmente sin Make:
```bash
gcc -Wall -Wextra -Iinclude -std=c11 src/*.c -o db_engine
```

---

## Ejecucion

```bash
./db_engine
```

El prompt cambia segun el contexto:
- `>` - Sin base de datos seleccionada
- `tienda>` - Base 'tienda' seleccionada
- `T> tienda>` - En transaccion sobre 'tienda'

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

## Propiedades ACID

### Atomicidad
Las transacciones usan un directorio temporal `data/_trans_tmp/` que guarda
el estado de todas las tablas al iniciar la transaccion. Al hacer `CANCELAR`,
se restauran los archivos originales. Al hacer `CONFIRMAR`, se elimina el
directorio temporal. Si el programa se interrumpe durante una transaccion,
se cancela automaticamente.

### Consistencia
- Validacion de tipos (INT, FLOAT, STRING)
- Verificacion de existencia de tablas antes de operar
- Verificacion de numero correcto de valores al insertar
- No se permiten operaciones sin seleccionar una base de datos

### Aislamiento
El sistema es monousuario. Cada operacion se ejecuta de principio a fin
sin interrupcion. Durante una transaccion, los cambios solo existen en
memoria hasta que se confirman.

### Durabilidad
- `CONFIRMAR` escribe inmediatamente a los archivos `.dat`
- `SALIR` guarda todos los datos antes de terminar
- Ctrl+C dispara el guardado automatico
- El archivo `log.txt` provee un registro permanente

---

## Manejo de Interrupciones (Senales)

El programa captura `SIGINT` (Ctrl+C) usando `signal()`:

1. El manejador establece un flag `volatile sig_atomic_t`
2. El ciclo principal detecta el flag y ejecuta el cierre seguro
3. Si hay transaccion activa, se cancela automaticamente
4. Se guardan todos los datos
5. Se registra en el log y termina

---

## Manejo de Procesos (fork)

El comando `RESPALDAR` demuestra el uso de `fork()`:

1. **Proceso hijo**: usa `system()` para copiar el directorio `data/` a `data/backup/`
2. **Proceso padre**: espera al hijo con `wait()` y verifica el resultado
3. Demuestra creacion de procesos, ejecucion paralela y sincronizacion

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

---

## Entrega del Proyecto

### Opcion recomendada: GitHub

```bash
git init
git add .
git commit -m "Motor propio de base de datos en C"
git remote add origin https://github.com/TU_USUARIO/ABD_Proyecto.git
git push -u origin main
```

### Alternativas
- **GitHub Codespaces**: Entorno Linux en el navegador
- **Replit**: Crear repl de C y subir archivos
- **Maquina Virtual**: VirtualBox + Ubuntu + `sudo apt install gcc make`
- **USB/ZIP**: Comprimir y entregar

### En la PC del profesor

```bash
# Con Make
make clean all && ./db_engine

# Sin Make
gcc -Wall -Wextra -Iinclude -std=c11 src/*.c -o db_engine && ./db_engine
```
