FROM python:3.11-slim

RUN apt-get update && apt-get install -y gcc && rm -rf /var/lib/apt/lists/*

WORKDIR /app

COPY . .

RUN gcc -Wall -Iinclude -std=c11 src/main.c src/database.c src/parser.c -o db_engine && \
    pip install --no-cache-dir -r web/requirements.txt

# Precargar datos de ejemplo (persisten en la imagen aunque Render reinicie)
RUN echo "CREAR BASE ejemplo\nUSAR ejemplo\nCREAR TABLA productos (id INT nombre STRING precio FLOAT)\nINSERTAR EN productos VALORES (1 Manzana 15.50)\nINSERTAR EN productos VALORES (2 Naranja 22.00)\nINSERTAR EN productos VALORES (3 Pera 18.75)\nSALIR" | ./db_engine

EXPOSE 10000

CMD gunicorn --chdir web --bind 0.0.0.0:10000 app:app
