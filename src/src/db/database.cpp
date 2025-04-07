#include "db/database.h"
#include "db/table_serialization.h"
#include "db/exceptions.h"
#include "parse/type.h"

#include <fstream>
#include <mutex>
#include <random>

Database::Database(const std::filesystem::path& path) : path(path) {
    if(!std::filesystem::exists(path)){
        init_directory(path);
    }
    
    check_directory(path);
    
    lock_directory(path);
    
    for(auto& entry : std::filesystem::directory_iterator(path)){
        std::filesystem::path filename = entry.path();
        
        std::string table_name = filename.filename().string();
        
        std::ifstream file;
        try{
            file.open(filename);
        }
        catch(std::exception& e){
            throw std::runtime_error("Failed to open file " + filename.string() + " - " + std::string(e.what()));
        }
        
        try{
            add_table(table_name, load_table(file));
        }catch(ParsingError& e){
            throw std::runtime_error("Failed to parse table " + filename.string() + " - " + std::string(e.what()));
        }
        catch(std::exception& e){
            throw std::runtime_error("Failed to load table " + filename.string() + " - " + std::string(e.what()));
        }
    }
}

void Database::check_directory(const std::filesystem::path& path){
    if(!std::filesystem::is_directory(path)){
        throw std::runtime_error("Path " + path.string() + " does not exist or is not a directory");
    }
    
    if(!std::filesystem::exists(path / MAGIC_FILE_NAME)){
        throw std::runtime_error("Path " + path.string() + " is not a database");
    }
}

void Database::lock_directory(const std::filesystem::path& path){
    if(std::filesystem::exists(path / LOCK_FILE_NAME)){
        throw std::runtime_error("Database is already locked");
    }
    
    try{
        std::ofstream(path / LOCK_FILE_NAME);
    }catch (std::exception& e){
        throw std::runtime_error("Failed to lock database - " + std::string(e.what()));
    }
}

void Database::unlock_directory(const std::filesystem::path& path){
    try{
        std::filesystem::remove(path / LOCK_FILE_NAME);
    }catch (std::exception& e){
        throw std::runtime_error("No lock file when unlocking - database might be corrupted!");
    }
}

void Database::init_directory(const std::filesystem::path& path){
    std::filesystem::create_directory(path);
    try{
        std::ofstream(path / MAGIC_FILE_NAME);
    }catch (std::exception& e){
        throw std::runtime_error("Failed to initialize database - " + std::string(e.what()));
    }
}

std::filesystem::path make_temp_dir() {
    // TODO this is not ideal since we are potentially sharing the database to all users
    std::string random_filename = std::to_string(std::random_device()());
    
    return std::filesystem::temp_directory_path() / random_filename;
}

void Database::save() const {
    // We can't have anything changing while saving
    auto lock = std::unique_lock(tables_lock);
    
    // First dump to a temp directory and then move it
    
    std::filesystem::path temp_dir = make_temp_dir();
    
    for(const auto& [table_name, table] : tables){
        std::ofstream file(temp_dir/table_name);
        
        serialize_table(table, file);
    }
    
    // let's just check it once again before deleting
    check_directory(path);
    
    std::filesystem::remove_all(path);
    std::filesystem::rename(temp_dir, path);
}

void Database::add_table(const std::string& table_name, Table table){
    auto lock = std::unique_lock(tables_lock);
    
    if(tables.contains(table_name)){
        throw std::runtime_error("Table " + table_name + " already exists");
    }
    
    tables.emplace(table_name, std::move(table));
}

Table& Database::get_table(const std::string& table_name){
    try{
        return tables.at(table_name);
    }
    catch(std::out_of_range&){
        throw InvalidQuery("Table " + table_name + " does not exist");
    }
}

void Database::remove_table(const std::string& table_name){
    auto lock = std::unique_lock(tables_lock);
    
    if(!tables.contains(table_name)){
        throw std::runtime_error("Table " + table_name + " does not exist");
    }
    
    tables.erase(table_name);
}

std::string make_response(bool success, const std::string& message){
    return std::string(success ? "OK" : "ERROR") + " " + message;
}

std::string Database::process_query(const std::string& query){
    bool success = true;
    std::string message;
    try {
        message = _process_query(query);
    }
    catch (const std::exception& e) {
        success = false;
        message = e.what();
    }
    
    return make_response(success, message);
}

std::string Database::_process_query(const std::string& query){
    TokenStream stream(query);
    
    std::string command = stream.get_token(TokenType::Identifier);
    
    if(command == "CREATE"){
        stream.ignore_token("TABLE");
        return process_create_table(stream);
    }
    
    else if(command == "DROP"){
        stream.ignore_token("TABLE");
        return process_drop_table(stream);
    }
    else if(command == "SELECT"){
        return process_select(stream);
    }
    else if(command == "INSERT"){
        stream.ignore_token("INTO");
        return process_insert(stream);
    }
    else if(command == "DELETE"){
        return process_delete(stream);
    }
    else{
        throw InvalidQuery("Unknown query type");
    }
}

std::string Database::process_drop_table(TokenStream& stream){
    std::string table_name;
    
    if(stream.empty()){
        throw InvalidQuery("Invalid query format");
    }
    
    remove_table(table_name);
    
    return make_response(true, "Table " + table_name + " dropped");
}

std::string Database::process_create_table(TokenStream& stream){
    std::string table_name = stream.get_token(TokenType::Identifier);
    
    std::vector<std::string> column_names;
    std::vector<Cell::DataType> column_types;
    
    stream.ignore_token("(");
    
    while(stream.peek_token().value != ")"){
        std::string name = stream.get_token(TokenType::Identifier);
        Cell::DataType type = string_to_type(stream.get_token(TokenType::Identifier));
        
        column_names.push_back(std::move(name));
        column_types.push_back(std::move(type));
    }
    
    stream.ignore_token(")");
    stream.ignore_token(";");
    
    Table table(std::move(column_names), std::move(column_types));
    
    add_table(table_name, std::move(table));
    
    return make_response(true, "Table " + table_name + " created");
}

/*
std::string Database::process_insert(TokenStream& stream){
    std::string table_name = stream.get_token(TokenType::Identifier);
    
    stream.ignore_token("VALUES");
    
    std::vector<Cell> values;
    
    stream.ignore_token("(");
    
    while(stream.peek_token().value != ")"){
        values.push_back(parse_token(stream));
    }
    
    stream.ignore_token(")");
    stream.ignore_token(";");
    
    Table& table = get_table(table_name);
    
    table.add_row(std::move(values));
    
    return make_response(true, "Row inserted into table " + table_name);
}
*/

std::string Database::process_select(TokenStream&){ throw std::runtime_error("Not implemented");};
std::string Database::process_insert(TokenStream&){ throw std::runtime_error("Not implemented");};
std::string Database::process_delete(TokenStream&){ throw std::runtime_error("Not implemented");};