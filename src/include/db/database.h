#ifndef DATABASE_H
#define DATABASE_H

#include "db/table.h"
#include "parse/token_stream.h"

#include <string>
#include <map>
#include <shared_mutex>


class Database {
public:
    /*
     * @brief Construct an empty database
     */
    Database() = default;
    
    /*
     * @brief Construct a database from a list of tables
     * @params Pairs of (table, table name)
     */
    Database(std::vector<std::pair<Table, std::string>> table_list);
    
    /**
     * @brief Process an SQL query and return the response
     * @param query The SQL query to process
     * @return The response
     */
    std::string process_query(const std::string& query) noexcept;
    
    class Accessor{
    private:
        Accessor() = default;
        friend class DatabaseManager;
        friend class Database;
    };
    
    const std::map<std::string, Table>& get_tables([[maybe_unused]] Accessor accessor = Accessor()) const {return tables;}
    
private:
    /**
     * @brief Get a table by name
     * @throws InvalidQuery if the table doesn't exist
     */
    Table& get_table(const std::string& name);
    
    /**
     * @brief Add a new table to the database
     * @throws InvalidQuery if a table with that name already exists
     */
    void add_table(const std::string& name, Table table);
    
    /**
     * @brief Remove a table from the database
     * @throws InvalidQuery if the table doesn't exist
     */
    void remove_table(const std::string& name);
    
    /**
     * @brief Pass the query to the correct handler
     * @param stream The query
     * @return Response to the query
     */
    std::string process_query_pick_type(TokenStream& stream);
    
    /**
     * @brief Convert the query string into a TokenStream and process it
     */
    std::string process_query_make_stream(const std::string& query);
    
    /**
     * @brief Evaluate a SELECT statement
     * @return Table resulting from the statement
     */
    Table evaluate_select(TokenStream& stream, const VariableList& variables);
    
    /**
     * @brief Process the GROUP BY and HAVING clauses
     * @param table The table to be grouped
     * @return A table for each of the resulting groups
     */
    std::vector<Table> evaluate_select_group(TokenStream& stream, const VariableList& variables, Table& table);
    
    /**
     * @brief Read tables referenced in a SELECT statement
     * @return Pairs of the table and the alias
     */
    std::vector<std::pair<const Table&, std::string>> read_selected_tables(TokenStream& stream);
    
    /**
     * @brief Process a CREATE TABLE statement
     * @param stream The query
     * @return Response to the query
     */
    std::string process_create_table(TokenStream& stream);
    
    /**
     * @brief Process a DROP TABLE statement
     * @param stream The query
     * @return Response to the query
     */
    std::string process_drop_table(TokenStream& stream);
    
    /**
     * @brief Process a SELECT statement
     * @param stream The query
     * @return Response to the query
     */
    std::string process_select(TokenStream& stream);
    
    /**
     * @brief Process an INSERT statement
     * @param stream The query
     * @return Response to the query
     */
    std::string process_insert(TokenStream& stream);
    
    /**
     * @brief Process a DELETE statement
     * @param stream The query
     * @return Response to the query
     */
    std::string process_delete(TokenStream& stream);
    
    std::map<std::string, Table> tables;
    
    /**
     * @brief Callback for SELECT subquery evaluation
     */
    std::function<Table(TokenStream&, const VariableList&)> select_callback = 
        [this](TokenStream& stream, const VariableList& variables){
        return this->evaluate_select(stream, variables);
    };
    
    /**
     * @brief Shared mutex for table access
     * @details Protects the structure of the tables map but not the tables themselves
     */
    mutable std::shared_mutex tables_lock;
};

#endif