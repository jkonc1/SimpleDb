# Dumbatase developer documentation

Let's start by drafting a high level picture of the project.
The project consists of several components:
- **IO** is responsible for providing the user-facing communication interface. It provides
  - **Socket** representing an endpoint the user can connect to for sending queries.
  - **Connection** representing a connection between an user and the server. It's created after the user connects to the **Socket** and is used to send and receive messages.
- **DatabaseManager** takes care of loading the database from the filesystem and storing it there.
- **JobQueue** take care of managing currently active queries and ensuring they are answered.
- **Engine** is the core of the project. It is responsible for parsing and executing queries.

**Engine** further consists of several parts:
- **TokenStream** is an abstraction for reading the input query in a tokenized way.
- **Cell** is a type capable of holding a value of any of the supported types and executing arithmetic operations on it. It's used to store the values in the tables.
- **Table** represents a table in the database. It supports basic operations like filtering by conditions, cross products of tables...
- **Database** holds the tables and executes the queries on them using the operations provided by the tables.

The main flow of the application is along these lines:

Startup:
1. **JobQueue** is spawned.
2. SIGINT handler is configured.
3. **DatabaseManager** is spawned and used to load the database from the filesystem.
4. The **Socket** is spawned and starts listening for incoming connections.

Execution:
1. **Socket** receives a connection from a user and creates a **Connection** object.
2. This **Connection** is added to the **JobQueue**.
3. After receiving a new query, **JobQueue** spawns a new thread to handle it.
4. The thread passes the query to **Database** for execution.
5. **Database** parses and executes the query.
  - This is done by converting the query into a **TokenStream** for parsing and executing it by calling the operations implemented on **Table**.
  - Invalid queries are handled by throwing `InvalidQuery`, which is caught by the root query execution method of **Database** and converted to the corresponding error message.
  - The response is then returned.
6. The **Connection** sends the response back to the user.
7. The **Connection** is closed and the thread finishes.

Shutdown:
1. Is triggered by the SIGINT handler.
2. The **Socket** is closed.
3. **JobQueue** waits for all the queries to finish executing.
4. **DatabaseManager** is used to save the database to the filesystem.
5. The application exits.

## IO

The IO module provides two interfaces:
- **Socket** represents an object that listens for incoming connections and accepts them.
  - `void listen(std::function<void(std::unique_ptr<IPCConnection>&&)> callback)`
    - This method blocks and begins listening for incoming connections.
    - When a connection is accepted, it calls the callback with the connection object.
  - `void stop()`
    - When called, the `listen()` running on the corresponding object exits and listening for connections stops. 
- **Connection** represents a connection between the server and a user. It is used to send and receive messages.
  - `void send(const std::string& message)`
    - This method sends a message to the user.
  - `std::string receive()`
    - This method blocks and waits for a message from the user. When a message is received, it reads until the end of the line and returns it.

For obvious reasons, the implementation details are platform dependent.

### Windows

On Windows, the interfaces are provided through *Windows Named Pipes*.
They are implemented partially using *ASIO Windows object handles* and the parts not supported by ASIO are implemented by directly using the *Windows API*.

In particular, **Connection** is completely covered by *ASIO* while spawning the **Socket** is done using the *Windows API*.

### Unix

On Unix, the interfaces are provided through *Unix Domain Sockets*.

They are fully supported by *ASIO* and thus the implementation is straightforward.

## DatabaseManager

**DatabaseManager** owns a **Database** and maintains the connection between it and its filesystem representation.

### Pseudo CSV

A CSV-like format is used to serialize the tables. It has the following format:
- Each line contains several values separated by commas.
- The final value is also followed by a comma.
- There is no quoting or such, instead some characters are escaped using a backslash:
  - `\,` is used to escape a comma.
  - `\x` is used to represent a NULL value (instead of an empty string).
  - `\\` is used to escape a backslash.

### Storage format

The database is stored as a directory. The directory contains a file for each table in the directory. The name of the file is the name of the table. The format of the table file is as follows:
- The first line lists the names of the columns of the table.
- The second line lists the types of the columns of the table.
- Each subsequent line lists the values of a row in the table.

These are stored as pseudo CSV.

Example:
```
name,age,gender,
STRING,INT,CHAR,
John,25,M,
Max,19,\x,
```

In addition to the table files, there are two special files:
- `.magic.db` is used to mark a directory as a database.
  - A directory not containing this file will not be loaded.
- `.lock.db` is used to prevent multiple **Dumbatase** processes from accessing the same database at the same time.
  - This file is created when the database is loaded and deleted when it is unloaded.
  - If the file exists on load, the loading fails.

To prevent data loss while saving the database, the new tables are first saved to a temporary directory, which then replaces the original one.

## JobQueue

**JobQueue** is responsible for managing the currently active queries and ensuring they are answered.

It is implemented as a thread aggregator, where each thread is responsible for executing a query.

The interface is as follows:
  - `void add_job(std::move_only_function<void() noexcept> task)`
    - Adds a new job to the queue.
    - A thread is spawned to execute the job.
  - `void finish()`
    - Waits for all the jobs to finish by joining all the threads.

## Engine

### TokenStream

**TokenStream** is an extension over STL's istreams that allows the input to be read token by token.
The following token types are supported:
- *Identifier* - `[a-zA-Z_][a-zA-Z0-9_]*`
- *String* - `\"[^\"]*\"` or `\'[^\']*\'`
- *Number* - `[0-9][0-9.]*`
- *SpecialChar* - all characters outside `[a-zA-Z0-9_\"\'\s]`.
  - Also includes double character symbols `==`, `<>`, `<=`, `>=`
- *Empty* - a dummy token used to mark the end of the stream.

White characters are ignored but assumed to separate tokens.

A **Token** consists of a type and a value. The type is one of the types listed above and the value is the string representation of the token.

The most important methods of **TokenStream** are:
- `Token get_token()`
  - Returns the next token in the stream.
- `const Token& peek_token()`
  - Returns the next token in the stream without consuming it.

It also provides several utility methods extending these, such as ignoring a predefined token or reading a token of given type.

The process is implemented by keeping a `std::optional` containing the next token. When next token is requested, it's parsed and stored in the optional. 
Since the type of the token in uniquely determined by its first character, the parsing is done by reading the first character and calling the corresponding parsing method.

### Cell

The **Cell** type represents a single value in a table. It's a wrapper around a `std::variant` containing all the supported data types.
It also implements overloads for basic binary operators and methods for converting between **Cell**s of different types.

The conversions are done by a bunch of `std::visit`s and mostly on case-by-case basis.
The binary operations are performed by first converting the operands to a common type and then applying the operation defined on that type.

### Table

A **Table** represents a table in the database. 
It consists of a **TableHeader** and a list of table rows.

The **TableHeader** contains the names and types of the columns in the table. Besides other things, it is used to lookup the index of a column by its name.

It's external interface allows the following operations:
1. Filtering the rows of the table by a condition.
2. Grouping the table by a set of columns.
  - This creates a new table for each combination of values in the grouped columns.
3. Creating the cross product of several tables.
4. Generating a new table by projecting the rows of the original table through a set of expressions.
5. Inserting a new row into the table.

### Database

A **Database** holds the tables and executes the queries on them using the operations provided by the tables.

`CREATE TABLE` is implemented by creating a new table and inserting it into the set of tables.
`DROP TABLE` is implemented by looking up the table by its name and removing it from the set of tables.
`INSERT INTO` is implemented by looking up the table by its name and inserting the new row into it.
`DELETE` looks up the table and filters by the negation of the condition.
`SELECT` works by:
1. Looking up the corresponding tables and creating their cross product.
2. Filtering the rows of the cross product by the `WHERE` condition.
3. Grouping the rows by the `GROUP BY` columns.
4. Filtering out the group tables by the `HAVING` condition.
5. Projecting the rows of the group tables by the `SELECT` expressions.

Steps 2-4 are conditional on the presence of the corresponding clauses in the query.

### ExpressionEvaluation

**ExpressionEvaluation** is a utility class closely linked to **Table** used to evaluate a single expression.

It works parsing the expression into its tree and evaluating it on each row of the table.
Aggregates are handled by being evaluated during the building of the tree and the results are inserted into the tree as constants.

It is used for the projection operation and inside conditions.

### ConditionEvaluation

**ConditionEvaluation** is another utility class closely linked to **Table**. Its purpose is evaluating a single condition on a **Table**.

It works by parsing the condition and evaluating it on each row in parallel by sweeping through the condition with a `std::valarray<bool>`.
`SELECT` subqueries are handled by evaluating them for each row separately by the callback provided by the caller. The callback call the `SELECT` evaluation interface of **Database**.

## Thread safety

Since separate threads are utilized for each query, the components used for query execution must be thread safe.

After the listening is started, the **IO** module is thread-safe thanks to the guarantees provided by *ASIO*.
**JobQueue** guards itself with a mutex and thus is thread safe.
**DatabaseManager** ensures thread-safety by exclusively locking the database before performing any operation on it. Inter-process safety is ensured by using lock files.
**Table** guards itself with a `std::shared_mutex` and thus is thread safe.
**Database** has a `std::shared_mutex` guarding the list of tables (but not the tables themselves) making the table access thread safe.
