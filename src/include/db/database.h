#ifndef DATABASE_H
#define DATABASE_H

#include "db/table.h"

#include <string>
#include <memory>
#include <map>
#include <shared_mutex>
#include <filesystem>
#include <vector>


class Database {
public:
    Database(const std::filesystem::path& path);
    std::string process_query(const std::string& query);
    
    void save() const;
private:
    void add_table(const std::string& name, std::shared_ptr<Table> table);
    void remove_table(const std::string& name);
    
    static void init_directory(const std::filesystem::path& path);
    static void check_directory(const std::filesystem::path& path);
    
    static void lock_directory(const std::filesystem::path& path);
    static void unlock_directory(const std::filesystem::path& path);
    
private:
    
    const std::filesystem::path path;

    std::map<std::string, std::shared_ptr<Table>> tables;
    
    mutable std::shared_mutex tables_lock;
    
    inline static const std::string MAGIC_FILE_NAME = ".magic.db";
    inline static const std::string LOCK_FILE_NAME = ".lock.db";
};

#endif