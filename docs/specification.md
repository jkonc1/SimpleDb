# Specification of the project

The goal of this project is to create a simplified SQL engine.

## IO

The engine will support IO through:
- On Windows through Named Pipes
- On Unix based systems, Unix Domain Sockets will be used

## SQL

The engine will support a subset of SQL commands:
- SELECT
- INSERT
- DELETE
- CREATE TABLE
- DROP TABLE

### Selection clauses

The exact syntax of the supported selection clauses is described by the images in the `clauses/` directory.
The source of the images are the presentations of *NDBI025 - Database Systems*.

## Data types

The engine will support the following data types:
- INT
- FLOAT
- VARCHAR
- CHAR

## Storage

The data will be stored in a text format resembling CSV. Each table will be stored in a separate file.
