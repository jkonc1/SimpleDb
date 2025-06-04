#include "db/database_manager.h"
#include "db/table_serialization.h"
#include "db/exceptions.h"
#include "helper/logger.h"

#include <fstream>
#include <random>
#include <mutex>

DatabaseManager::DatabaseManager(const std::filesystem::path& database_directory) : path(database_directory) {}

DatabaseManager::~DatabaseManager(){
    if(is_loaded()){
        try{
            save();
        }
        catch(...){
            logger::log("Warning: failed to save database");
        }
    }
}

bool DatabaseManager::is_loaded() const {
    return database != nullptr;
}

bool DatabaseManager::is_table_file(const std::filesystem::path& filename) {
    return filename.extension() != ".db";
}

void DatabaseManager::load(){
    if(is_loaded()){
        return;
    }
    
    if(!std::filesystem::exists(path)){
        init_directory(path);
    }
    
    check_directory(path);
    lock_directory(path);
    
    std::vector<std::pair<Table, std::string>> tables;
    
    for(const auto& entry : std::filesystem::directory_iterator(path)){
        const std::filesystem::path& filename = entry.path();
        
        if(!is_table_file(filename)){
            // Skip magics, locks etc.
            continue;
        }
        
        std::string table_name = filename.filename().string();
        
        std::ifstream file;
        try{
            file.open(filename);
        }
        catch(std::exception& e){
            throw std::runtime_error("Failed to open table " + filename.string() + " - " + std::string(e.what()));
        }
        
        try{
            tables.emplace_back(load_table(file), table_name);
        }catch(ParsingError& e){
            throw std::runtime_error("Failed to parse table " + filename.string() + " - " + std::string(e.what()));
        }
        catch(std::exception& e){
            throw std::runtime_error("Failed to load table " + filename.string() + " - " + std::string(e.what()));
        }
    }
    
    database = std::make_unique<Database>(std::move(tables));
}

void DatabaseManager::check_directory(const std::filesystem::path& path){
    if(!std::filesystem::is_directory(path)){
        throw std::runtime_error("Path " + path.string() + " does not exist or is not a directory");
    }
    
    if(!std::filesystem::exists(path / MAGIC_FILE_NAME)){
        throw std::runtime_error("Path " + path.string() + " is not a database");
    }
}

void DatabaseManager::lock_directory(const std::filesystem::path& path){
    if(std::filesystem::exists(path / LOCK_FILE_NAME)){
        throw std::runtime_error("Database is already locked");
    }
    
    try{
        std::ofstream(path / LOCK_FILE_NAME);
    }catch (std::exception& e){
        throw std::runtime_error("Failed to lock database - " + std::string(e.what()));
    }
}

void DatabaseManager::unlock_directory(const std::filesystem::path& path){
    try{
        std::filesystem::remove(path / LOCK_FILE_NAME);
    }catch (std::exception& e){
        throw std::runtime_error("No lock file when unlocking - database might be corrupted!");
    }
}

void DatabaseManager::init_directory(const std::filesystem::path& path){
    std::filesystem::create_directory(path);
    try{
        std::ofstream(path / MAGIC_FILE_NAME);
    }catch (std::exception& e){
        throw std::runtime_error("Failed to initialize database - " + std::string(e.what()));
    }
}

std::filesystem::path make_temp_dir() {
    std::string random_filename = std::to_string(std::random_device()());
    
    return std::filesystem::temp_directory_path() / random_filename;
}

static void move_across_devices(const std::filesystem::path& from, const std::filesystem::path& to) {
    std::filesystem::copy(from, to, std::filesystem::copy_options::recursive | std::filesystem::copy_options::overwrite_existing);
    std::filesystem::remove_all(from);
}

void DatabaseManager::save() {
    if(!is_loaded()){
        throw std::runtime_error("Saving database failed - Database is not loaded");
    }
    
    // First dump to a temp directory and then move it
    
    std::filesystem::path temp_dir = make_temp_dir();
    
    init_directory(temp_dir);
    
    for(const auto& [table_name, table] : database->get_tables()){
        std::ofstream file(temp_dir/table_name);
        
        serialize_table(table, file);
    }
    
    // let's just check it once again before deleting
    check_directory(path);
    
    std::filesystem::remove_all(path);
    move_across_devices(temp_dir, path);
}

Database& DatabaseManager::get(){
    if(!is_loaded()){
        throw std::runtime_error("Database is not loaded");
    }
    return *database;
}
