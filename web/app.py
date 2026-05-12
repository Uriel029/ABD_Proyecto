import os
import subprocess
from flask import Flask, render_template, request, jsonify, session

app = Flask(__name__)
app.secret_key = os.urandom(24)

DIRECTORIO = os.path.dirname(os.path.abspath(__file__))
EJECUTABLE = os.path.join(os.path.dirname(DIRECTORIO), "db_engine")

COMANDOS_SIN_DB = [
    "CREAR BASE", "MOSTRAR BASES", "USAR ", "SALIR",
    "AYUDA", "RESPALDAR", "ELIMINAR BASE", "LIMPIAR"
]


@app.route("/")
def index():
    return render_template("index.html")


@app.route("/api/ejecutar", methods=["POST"])
def ejecutar():
    data = request.get_json()
    comando = data.get("comando", "").strip()
    if not comando:
        return jsonify({"salida": "", "error": "Comando vacio"})

    db_actual = session.get("db_actual", None)

    # Rastrear cambios en la BD actual (sesion)
    if comando.upper().startswith("USAR "):
        db_actual = comando[5:].strip()
        session["db_actual"] = db_actual
    elif comando.upper() in ("SALIR BASE", "SALIR BD"):
        db_actual = None
        session.pop("db_actual", None)
    elif comando.upper().startswith("ELIMINAR BASE "):
        nombre = comando[14:].strip()
        if nombre == db_actual:
            db_actual = None
            session.pop("db_actual", None)

    necesita_db = not any(comando.upper().startswith(p) for p in COMANDOS_SIN_DB)
    cmd_completo = comando
    if db_actual and necesita_db:
        cmd_completo = f"USAR {db_actual}\n{comando}"

    try:
        proc = subprocess.Popen(
            [EJECUTABLE],
            stdin=subprocess.PIPE, stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            cwd=os.path.dirname(EJECUTABLE),
            text=True
        )
        stdout, stderr = proc.communicate(input=f"{cmd_completo}\nSALIR\n", timeout=10)

        salida = procesar_salida(stdout, comando, db_actual)
        error = stderr.strip() if stderr and stderr.strip() else ""

        return jsonify({"salida": salida, "error": error})
    except subprocess.TimeoutExpired:
        return jsonify({"salida": "", "error": "Timeout - el comando tardo demasiado"})
    except FileNotFoundError:
        return jsonify({"salida": "", "error": "db_engine no encontrado"})
    except Exception as e:
        return jsonify({"salida": "", "error": str(e)})


def procesar_salida(stdout, comando_original, db_actual):
    lineas = stdout.split("\n")
    resultado = []
    for linea in lineas:
        ls = linea.strip()
        if not ls:
            continue

        if "MI PROPIO MOTOR" in ls or "Escribe AYUDA" in ls or "Escribe SALIR" in ls:
            continue
        if "Datos guardados" in ls or "Hasta luego" in ls:
            continue
        if "Cargadas" in ls:
            continue
        if ls.endswith("> ") or ls in (">", "T>"):
            continue

        # Clasificar linea
        if "Comando invalido" in ls or "Error:" in ls:
            resultado.append(f'<span class="error">{ls}</span>')
        elif "Usando base" in ls or "creada" in ls or "creado" in ls:
            resultado.append(f'<span class="exito">{ls}</span>')
        elif "Registro insertado" in ls or "Actualizados" in ls or "Eliminados" in ls:
            resultado.append(f'<span class="exito">{ls}</span>')
        elif "Transaccion" in ls:
            cls = "aviso" if "cancelada" in ls else "exito"
            resultado.append(f'<span class="{cls}">{ls}</span>')
        elif "Respaldo exitoso" in ls:
            resultado.append(f'<span class="exito">{ls}</span>')
        elif "No hay" in ls or "no hay" in ls:
            resultado.append(f'<span class="aviso">{ls}</span>')
        elif "Ya existe" in ls or "no existe" in ls:
            resultado.append(f'<span class="error">{ls}</span>')
        elif "Saliendo" in ls:
            resultado.append(f'<span class="aviso">{ls}</span>')
        elif "+---" in ls or "+===" in ls:
            resultado.append(f'<span class="borde">{ls}</span>')
        elif "Total:" in ls:
            resultado.append(f'<span class="encabezado">{ls}</span>')
        elif "|" in ls:
            if any(p in ls for p in ["ID", "NOMBRE", "PRECIO", "CANT",
                    "Tipo", "Nombre", "Base", "Tabla"]):
                resultado.append(f'<span class="encabezado">{ls}</span>')
            else:
                resultado.append(f'<span>{ls}</span>')
        else:
            resultado.append(f'<span>{ls}</span>')

    prompt = f"{db_actual or ''}=# " if db_actual else "=# "
    salida_html = f'<span class="prompt">{prompt}{comando_original}</span>\n'
    for r in resultado:
        salida_html += r + "\n"

    return salida_html


if __name__ == "__main__":
    app.run(debug=True, port=5000)
