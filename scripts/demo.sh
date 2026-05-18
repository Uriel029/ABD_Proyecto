#!/bin/bash
# Script de demostracion para el Motor de Base de Datos en C
# Ejecuta todos los comandos principales de forma automatizada.
#
# Uso: bash scripts/demo.sh   (desde la raiz del proyecto)
#   o: bash demo.sh           (dentro de scripts/)

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"

cd "$PROJECT_DIR" || { echo "Error: No se pudo acceder a $PROJECT_DIR"; exit 1; }

ENGINE=./db_engine

if [ ! -f "$ENGINE" ]; then
    echo "Compilando el proyecto..."
    make clean all || { echo "Error de compilacion"; exit 1; }
fi

echo "============================================="
echo "  DEMO: Motor de Base de Datos en C"
echo "============================================="
echo ""

echo "=== 1. CREAR BASES, TABLAS E INSERTAR REGISTROS ==="
printf 'CREAR BASE tienda\nUSAR tienda\nCREAR TABLA productos (id INT nombre STRING precio FLOAT cantidad INT)\nDESCRIBIR productos\nINSERTAR EN productos VALORES (1 Laptop 15000.50 5)\nINSERTAR EN productos VALORES (2 Mouse 250.99 10)\nINSERTAR EN productos VALORES (3 Teclado 800 3)\nSALIR\n' | $ENGINE

echo ""
echo "=== 2. CONSULTAR (SELECT) ==="
printf 'USAR tienda\nSELECCIONAR * DE productos\nSELECCIONAR * DE productos DONDE id=1\nSALIR\n' | $ENGINE

echo ""
echo "=== 3. ACTUALIZAR ==="
printf 'USAR tienda\nACTUALIZAR productos SET precio=14500 DONDE id=1\nSELECCIONAR * DE productos\nSALIR\n' | $ENGINE

echo ""
echo "=== 4. ELIMINAR ==="
printf 'USAR tienda\nELIMINAR DE productos DONDE id=2\nSELECCIONAR * DE productos\nSALIR\n' | $ENGINE

echo ""
echo "=== 5. TRANSACCION CON CANCELACION (ROLLBACK) ==="
printf 'USAR tienda\nSELECCIONAR * DE productos\nINICIAR\nINSERTAR EN productos VALORES (4 Monitor 3500 2)\nSELECCIONAR * DE productos\nCANCELAR\nSELECCIONAR * DE productos\nSALIR\n' | $ENGINE

echo ""
echo "=== 6. TRANSACCION CON CONFIRMACION (COMMIT) ==="
printf 'USAR tienda\nINICIAR\nINSERTAR EN productos VALORES (4 Monitor 3500 2)\nCONFIRMAR\nSELECCIONAR * DE productos\nSALIR\n' | $ENGINE

echo ""
echo "=== 7. RESPALDO CON fork() ==="
printf 'USAR tienda\nRESPALDAR\nMOSTRAR BASES\nSALIR\n' | $ENGINE

echo ""
echo "=== 8. MULTIPLES BASES DE DATOS ==="
printf 'CREAR BASE escuela\nUSAR escuela\nCREAR TABLA alumnos (id INT nombre STRING grado INT)\nINSERTAR EN alumnos VALORES (1 Juan 5)\nINSERTAR EN alumnos VALORES (2 Maria 6)\nMOSTRAR TABLAS\nMOSTRAR BASES\nSALIR\n' | $ENGINE

echo ""
echo "============================================="
echo "  DEMO COMPLETADA EXITOSAMENTE"
echo "============================================="
echo ""
echo "Ver archivos generados:"
ls -laR data/
