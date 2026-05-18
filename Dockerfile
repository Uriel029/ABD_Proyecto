FROM python:3.11-slim

RUN apt-get update && apt-get install -y gcc && rm -rf /var/lib/apt/lists/*

WORKDIR /app

COPY . .

RUN gcc -Wall -Iinclude -std=c11 src/main.c src/database.c src/parser.c -o db_engine && \
    pip install --no-cache-dir -r web/requirements.txt

# Precargar datos de ejemplo (persisten en la imagen aunque Render reinicie)
RUN printf 'CREAR BASE ejemplo\nUSAR ejemplo\nCREAR TABLA productos (id INT nombre STRING precio FLOAT cantidad INT)\nINSERTAR EN productos VALORES (1 Laptop 15000.50 5)\nINSERTAR EN productos VALORES (2 Mouse 250.99 10)\nINSERTAR EN productos VALORES (3 Teclado 800 3)\nSALIR\n' | ./db_engine

EXPOSE 10000

CMD gunicorn --chdir web --bind 0.0.0.0:10000 app:app
