# Introduction
Dumbatase is a simple SQL engine supporting a subset of SQL. 

# Documentation
## System requirements
This project supports Windows and Unix-based operating systems. 

Building the software requires the MSVC compiler on Windows and either the GCC or clang on Unix-based systems.
The compiler must support the C++23 standard.

## Build instructions
On Windows, the project can be built by being imported into Visual Studio and built there.

For Unix-based systems, build script [`tools/build.sh`](../../src/tools/build.sh) is provided.
After the script is run, the built binary will be located at [`build/Dumbatase`](../../src/build/Dumbatase).

## Starting the project
The project is run by executing the binary with two arguments:
`Dumbatase <database location> <io location>`

`<database location>` shall either be a path to a database created by a previous run of the program
or a free location, in which case a new database will be created there.

`<io location>` shall be the path to the communication interface further described in **Interface**.


## Interface
After the program is started, a communication *interface* is opened at `<io location>`. This interface
is OS-dependent.
- On Windows, communication is provided through *Named Pipes*. The program spawns Named Pipes at 
the given location, the user program shall connect to it to process queries.
    - Windows named pipes must be located inside `\\.\pipe\`
- On Unix-based systems, the communication makes use of *Unix Domain Sockets*. A socket is spawned
at the given location and processes queries for the duration of the program.
    - Sockets may be located anywhere in the filesystem

## Interaction
To execute queries, the user shall connect to the *interface* (example connection scripts can be found
in [`tools/connect`](../../src/tools/connect)).
Afterwards, they shall send a single query on a single line through the *interface*.
They will be sent back a response that starts with either `OK`, `ERR` or `EXC` followed by a space.
- If it's `OK`, the query was processed successfully.
    - The rest of the response is either as described by the used query type or undefined otherwise.
- In case of `ERR`, the query was malformed or otherwise invalid and could not be processed.
    - The rest of the response may describe the error. The exact content is undefined and shall not be relied on.
- In case of `EXC`, the engine has encountered an internal error during the execution of the query.
    - The rest of the response may describe the error. The exact content is undefined and shall not be relied on.

After the response, the connection is closed. To make another query, a new connection shall be made.

## Exiting
The engine is stopped by being sent a `SIGINT` signal.
Upon receiving the signal, the engine stops receiving new connections and finishes executing the currently
active ones.
Once all the remaining queries are processed, the *interface* is closed and the database is saved. 
Afterwards, the program exits.

In cases of necessity, the engine may be forcefully stopped by sending a second `SIGINT` signal.
In this case, there are no guarantees about the processing of active queries or saving the database.

## Feature support

### Types
The engine supports the following data types:
- INT - `42`
- STRING - `'foo'` or `"bar"`
- FLOAT - 3.14
- CHAR - 'x'
- NULL

Implicit conversions are performed during binary operations on values.
The conversions are defined by the following tree:
INT -> FLOAT -> STRING -> NULL
               /
            CHAR
Both operands are promoted to the lowest common ancestor of their types.
            
### Transactions
Not supported, every connection to the *interface* serves a single query.

### Query types

#### CREATE TABLE
The `CREATE TABLE` query is used to create a new table.

Syntax : `CREATE TABLE <table name> ( <column name> <column type>, ... );`.

`<table name>` and `<column name>` shall be *identifiers*. 
The database shall not already contain a table named `<table name>`.
`<column type>` shall be a non-`NULL` data type.

An *identifier* is of format `[a-zA-Z][a-zA-Z0-9_]*` and is not any SQL keyword.

If the query is successful, a new table named `<table name>` is inserted in the database.
Its columns are described by the provided column names and types.

#### DROP TABLE
The `DROP TABLE` query is used to remove a table from the database.

Syntax : `DROP TABLE <table name>;`

`<table name>` shall be the name of an existing table.

If the query is successful, table `<table name>` is removed from the database.

#### INSERT INTO
The `INSERT INTO` query is used to insert a new row into a table.

Syntax : `INSERT INTO <table name> ( <inserted column>, ... )? VALUES ( <column value>, ... );`

`<table name>` shall be the name of a table in the database.

If `<inserted column>` are provided, their count shall match the count of `<column value>`.
`<inserted column>` shall contain distinct names of columns of `<table name>`. 
The columns named in `<inserted column>` shall be filled by the corresponding `<column value>`, others shall be filled by `NULL`s.

If `<inserted column>` are not provided, the row shall not contain `NULL`s and `<column value>` shall contain the values of the row in the same order the columns were ordered on creation of the table.

The types of values in `<column value>` shall match the types of their corresponding columns.

#### SELECT
The `SELECT` query is used extract values containing certain criteria from the database.

Syntax : `SELECT (* | <expression>,...) FROM <table name> <table alias>, ... [WHERE <search condition>] [GROUP BY <grouping column>, ... [ HAVING <search condition> ]];` 

`<table name>` shall be the name of a table in the database.
`<table alias>` is an optional alias for the table, it is used to disambiguate columns with the same name in different tables.
If the alias is not provided, the table name is used as the alias.

The syntax of `<search condition>` and `<expression>` is described in [`select_syntax.md`](select_syntax.md).

The output of the query following the `OK` is as follows:
- The first line is of format `(<column name>,)*` and describes the column names of the resulting table.
- The second line if of format `(<column type>,)*` and describes the types of columns of the resulting tables.
    - `<column type>` is a non-`NULL` data type
- The remaining lines are of format `(<value>,)*` and describe one row of the table each
    - `<value>` are string representations of the values in the row (with no surrounding quotes or such)
    - commas in values are escaped as `\,`
    - `NULL` cells are represented by the escape sequence `\x`
    - backslashes are escaped by doubling

An example of the output is
```
OK character,name,ascii,
CHAR,STRING,INT,
\,,comma,44,
\\,backslash,\x,
```

#### DELETE
The `DELETE` query is used to erase rows satisfying a condition.

Syntax : `DELETE FROM <table name> [WHERE <search condition>];`

`<table name>` is the name of a table. The syntax of `<search condition>` is described in [`select_syntax.md`](select_syntax.md).

If the query is successful, rows satisfying `<search condition>` will be removed from the table.
If the `WHERE` clause is not present, all rows of the table will be erased instead.
