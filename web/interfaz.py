#!/usr/bin/env python3
import tkinter as tk
import subprocess
import os

class TerminalDB:
    def __init__(self, root):
        self.root = root
        self.root.title("db_engine v1.0")
        self.root.geometry("900x600")
        self.root.minsize(600, 400)

        self.directorio = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
        self.ejecutable = os.path.join(self.directorio, "db_engine")
        self.db_actual = None

        self.root.configure(bg="#1a1a2e")
        self.root.protocol("WM_DELETE_WINDOW", self.cerrar)
        self.crear_interfaz()

    def crear_interfaz(self):
        self.root.columnconfigure(0, weight=1)
        self.root.rowconfigure(0, weight=1)

        marco = tk.Frame(self.root, bg="#1a1a2e", padx=12, pady=12)
        marco.grid(row=0, column=0, sticky="nsew")
        marco.columnconfigure(0, weight=1)
        marco.rowconfigure(0, weight=1)

        self.texto = tk.Text(marco,
                             font=("Consolas", 12),
                             bg="#1a1a2e",
                             fg="#cdd6f4",
                             insertbackground="#00d4ff",
                             relief="flat", bd=0,
                             padx=10, pady=10,
                             wrap="word",
                             highlightthickness=0)
        self.texto.grid(row=0, column=0, sticky="nsew")

        scroll = tk.Scrollbar(marco, command=self.texto.yview,
                             bg="#1a1a2e", troughcolor="#1a1a2e")
        scroll.grid(row=0, column=1, sticky="ns")
        self.texto.configure(yscrollcommand=scroll.set)

        self.texto.tag_config("prompt", foreground="#00d4ff")
        self.texto.tag_config("resultado", foreground="#cdd6f4")
        self.texto.tag_config("error", foreground="#ff4757")
        self.texto.tag_config("exito", foreground="#00ff88")
        self.texto.tag_config("aviso", foreground="#ffa502")
        self.texto.tag_config("encabezado", foreground="#00d4ff")
        self.texto.tag_config("borde", foreground="#313244")

        linea_frame = tk.Frame(marco, bg="#1a1a2e")
        linea_frame.grid(row=1, column=0, columnspan=2, sticky="ew", pady=(8, 0))
        linea_frame.columnconfigure(1, weight=1)

        self.label_prompt = tk.Label(linea_frame, text="=> ",
                                     font=("Consolas", 12),
                                     bg="#1a1a2e", fg="#00d4ff")
        self.label_prompt.grid(row=0, column=0, sticky="w")

        self.entrada = tk.Entry(linea_frame, font=("Consolas", 12),
                                bg="#1a1a2e", fg="#ffffff",
                                insertbackground="#00d4ff",
                                relief="flat", bd=0,
                                highlightthickness=0)
        self.entrada.grid(row=0, column=1, sticky="ew")
        self.entrada.bind("<Return>", self.ejecutar)
        self.entrada.focus()

        self.prompt_inicial()
        self.verificar_ejecutable()

    def verificar_ejecutable(self):
        if not os.path.exists(self.ejecutable):
            self.texto.insert("end", "Error: db_engine no encontrado. Compila con make clean all\n", "error")

    def prompt_inicial(self):
        self.texto.insert("end", "db_engine v1.0 - Motor de base de datos en C\n", "encabezado")
        self.texto.insert("end", "Escribe AYUDA para ver los comandos\n", "aviso")
        self.texto.insert("end", "Escribe SALIR para terminar\n\n", "aviso")
        self.texto.see("end")

    def ejecutar(self, event=None):
        comando = self.entrada.get().strip()
        if not comando:
            return

        self.entrada.delete(0, "end")

        if comando.upper() == "AYUDA":
            self.mostrar_ayuda()
            return

        if comando.upper() == "LIMPIAR":
            self.texto.delete("1.0", "end")
            return

        if comando.upper().startswith("USAR "):
            self.db_actual = comando[5:].strip()
        elif comando.upper() in ("SALIR BASE", "SALIR BD"):
            self.db_actual = None
        elif comando.upper().startswith("ELIMINAR BASE "):
            nombre = comando[14:].strip()
            if nombre == self.db_actual:
                self.db_actual = None

        necesita_db = not any(comando.upper().startswith(p) for p in [
            "CREAR BASE", "MOSTRAR BASES", "USAR ", "SALIR",
            "AYUDA", "RESPALDAR", "ELIMINAR BASE", "LIMPIAR"])

        cmd_completo = comando
        if self.db_actual and necesita_db:
            cmd_completo = f"USAR {self.db_actual}\n{comando}"

        prompt = f"{self.db_actual or ''}=# " if self.db_actual else "=# "
        self.texto.insert("end", prompt + comando + "\n", "prompt")
        self.texto.see("end")
        self.root.update()

        try:
            proc = subprocess.Popen(
                [self.ejecutable],
                stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                stderr=subprocess.PIPE, cwd=self.directorio, text=True
            )
            stdout, stderr = proc.communicate(input=f"{cmd_completo}\nSALIR\n", timeout=10)

            if stderr and stderr.strip():
                self.texto.insert("end", stderr.strip() + "\n", "error")

            self.procesar_salida(stdout)

        except subprocess.TimeoutExpired:
            self.texto.insert("end", "Error: Timeout\n", "error")
        except FileNotFoundError:
            self.texto.insert("end", "Error: db_engine no encontrado. Compila con make clean all\n", "error")
        except Exception as e:
            self.texto.insert("end", f"Error: {str(e)}\n", "error")

        self.texto.see("end")

    def procesar_salida(self, salida):
        for linea in salida.split("\n"):
            ls = linea.strip()
            if not ls:
                continue
            if "Comando invalido" in ls or "Error:" in ls:
                self.texto.insert("end", ls + "\n", "error")
            elif "Cargadas" in ls:
                pass
            elif "Usando base" in ls or "creada" in ls or "creado" in ls:
                self.texto.insert("end", ls + "\n", "exito")
            elif "Registro insertado" in ls:
                self.texto.insert("end", ls + "\n", "exito")
            elif "Actualizados" in ls or "Eliminados" in ls:
                self.texto.insert("end", ls + "\n", "exito")
            elif "Transaccion" in ls:
                if "cancelada" in ls:
                    self.texto.insert("end", ls + "\n", "aviso")
                else:
                    self.texto.insert("end", ls + "\n", "exito")
            elif "Respaldo exitoso" in ls:
                self.texto.insert("end", ls + "\n", "exito")
            elif "No hay base" in ls or "No hay registros" in ls or "No hay tablas" in ls:
                self.texto.insert("end", ls + "\n", "aviso")
            elif "Ya existe" in ls or "no existe" in ls:
                self.texto.insert("end", ls + "\n", "error")
            elif "Datos guardados" in ls or "Hasta luego" in ls or "Saliendo" in ls:
                pass
            elif "MI PROPIO MOTOR" in ls or "Escribe" in ls or "===" in ls or "---" in ls:
                pass
            elif "+---" in ls or "+===" in ls:
                self.texto.insert("end", ls + "\n", "borde")
            elif "Total:" in ls:
                self.texto.insert("end", ls + "\n", "encabezado")
            elif "|" in ls:
                if any(p in ls for p in ["ID", "NOMBRE", "PRECIO", "CANT",
                        "Tipo", "Nombre", "Base", "Tabla"]):
                    self.texto.insert("end", ls + "\n", "encabezado")
                else:
                    self.texto.insert("end", ls + "\n", "resultado")
            else:
                self.texto.insert("end", ls + "\n", "resultado")

    def mostrar_ayuda(self):
        ayuda = (
            "CREAR BASE <nombre>        - Crear una base de datos\n"
            "USAR <nombre>              - Seleccionar base de datos\n"
            "SALIR BASE / SALIR BD      - Desseleccionar base\n"
            "MOSTRAR BASES              - Listar bases de datos\n"
            "ELIMINAR BASE <nombre>     - Eliminar base de datos\n"
            "\n"
            "CREAR TABLA <nombre> (...)  - Crear tabla con columnas\n"
            "MOSTRAR TABLAS             - Listar tablas\n"
            "DESCRIBIR <nombre>         - Ver estructura de tabla\n"
            "ELIMINAR TABLA <nombre>    - Eliminar tabla\n"
            "\n"
            "INSERTAR EN <t> VALORES ()  - Insertar registro\n"
            "SELECCIONAR * DE <t> [...]  - Consultar registros\n"
            "ACTUALIZAR <t> SET ...      - Actualizar registros\n"
            "ELIMINAR DE <t> DONDE ...   - Eliminar registros\n"
            "\n"
            "INICIAR / CONFIRMAR / CANCELAR - Transacciones\n"
            "RESPALDAR                   - Respaldar datos\n"
            "AYUDA / LIMPIAR / SALIR     - Otros\n"
        )
        self.texto.insert("end", "\n" + ayuda, "resultado")
        self.texto.see("end")

    def cerrar(self):
        self.root.destroy()

if __name__ == "__main__":
    root = tk.Tk()
    app = TerminalDB(root)
    root.mainloop()
