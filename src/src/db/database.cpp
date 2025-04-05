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
