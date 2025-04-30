#ifndef DATABASE_MANAGER_H
#define DATABASE_MANAGER_H

#include "db/database.h"

#include <filesystem>
#include <memory>

class DatabaseManager {
public:
    DatabaseManager(const std::filesystem::path& database_directory);
    ~DatabaseManager();
    
    void load();
    void save();
    
    bool is_loaded() const;
    
    Database& get();
private:
    static void init_directory(const std::filesystem::path& path);
    static void check_directory(const std::filesystem::path& path);
    
    static void lock_directory(const std::filesystem::path& path);
    static void unlock_directory(const std::filesystem::path& path);
    
    static bool is_table_file(const std::filesystem::path& filename);

    std::filesystem::path path;
    
    std::unique_ptr<Database> database;
    
    inline static const std::string MAGIC_FILE_NAME = ".magic.db";
    inline static const std::string LOCK_FILE_NAME = ".lock.db";
};

#endif