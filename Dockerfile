FROM python:3.11-slim

RUN apt-get update && apt-get install -y gcc && rm -rf /var/lib/apt/lists/*

WORKDIR /app

COPY . .

RUN gcc -Wall -Iinclude -std=c11 src/main.c src/database.c src/parser.c -o db_engine && \
    pip install --no-cache-dir -r web/requirements.txt

EXPOSE 10000

CMD gunicorn --chdir web --bind 0.0.0.0:10000 app:app
