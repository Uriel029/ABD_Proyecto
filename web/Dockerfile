FROM python:3.11-slim

WORKDIR /app

# Instalar gcc para compilar el motor C
RUN apt-get update && apt-get install -y gcc && rm -rf /var/lib/apt/lists/*

# Copiar motor C
COPY . ../
RUN make -C .. clean all

# Copiar web app
COPY web/requirements.txt .
RUN pip install --no-cache-dir -r requirements.txt

COPY web/ .

# Compilar nuevamente en el contexto correcto
RUN cd .. && gcc -Wall -Iinclude -std=c11 src/main.c src/database.c src/parser.c -o db_engine

EXPOSE 10000

CMD gunicorn --bind 0.0.0.0:10000 app:app
