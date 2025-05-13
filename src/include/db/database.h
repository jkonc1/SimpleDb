#ifndef DATABASE_H
#define DATABASE_H

#include "db/table.h"
#include "parse/token_stream.h"

#include <string>
#include <map>
#include <shared_mutex>


class Database {
public:
    std::string process_query(const std::string& query) noexcept;
    
private:
    Table& get_table(const std::string& name);
    void add_table(const std::string& name, Table table);
    void remove_table(const std::string& name);
    
    std::string process_query_pick_type(TokenStream& stream);
    std::string process_query_make_stream(const std::string& query);
    
    Table evaluate_select(TokenStream& stream, const VariableList& variables);
    
    std::vector<Table> evaluate_select_group(TokenStream& stream, const VariableList& variables, Table& table);
    std::vector<std::pair<const Table&, std::string>> read_selected_tables(TokenStream& stream);
    
    std::string process_create_table(TokenStream& stream);
    std::string process_drop_table(TokenStream& stream);
    std::string process_select(TokenStream& stream);
    std::string process_insert(TokenStream& stream);
    std::string process_delete(TokenStream& stream);
    
    std::map<std::string, Table> tables;
    
    std::function<Table(TokenStream&, const VariableList&)> select_callback = 
        [this](TokenStream& stream, const VariableList& variables){
        return this->evaluate_select(stream, variables);
    };
    
    
    mutable std::shared_mutex tables_lock;
    
    friend class DatabaseManager;
};

#endif