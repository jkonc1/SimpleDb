# Introduction
Dumbatase is a simple SQL engine supporting a subset of SQL. 

# Documentation
## System requirements
This project supports Windows and Unix-based operating systems. 

## Build instructions
On Windows, the project can be built by being imported into Visual Studio and built there.

For Unix-based systems, build script [`tools/build.sh`](../tools/build.sh) is provided.
After the script is run, the built binary will be located at [`build/Dumbatase`](../build/Dumbatase).

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
in [`tools/connect`](../tools/connect)).
Afterwards, they shall send a single query on a single line through the *interface*.
They will be sent back a response that starts with either `OK`, `ERR` or `EXC`.
- If it's `OK`, the query was processed successfully.
    - The rest of the response is .
- In case of `ERR`, the query was malformed and could not be processed.
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

### Transactions
Not supported, every connection to the *interface* serves a single query.

### Query types

#### CREATE TABLE
The `CREATE TABLE` query is used by create a new table.

Syntax : `CREATE TABLE <table name> ( (<column name>, <column type>)* )`.

`<table name>` and `<column name>` shall be *identifiers*
