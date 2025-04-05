#ifndef DATABASE_H
#define DATABASE_H

#include "db/table.h"
#include "parse/token_stream.h"

#include <string>
#include <map>
#include <shared_mutex>
#include <filesystem>


class Database {
public:
    Database(const std::filesystem::path& path);
    std::string process_query(const std::string& query);
    
    void save() const;
private:
    Table& get_table(const std::string& name);
    void add_table(const std::string& name, Table table);
    void remove_table(const std::string& name);
    
    static void init_directory(const std::filesystem::path& path);
    static void check_directory(const std::filesystem::path& path);
    
    static void lock_directory(const std::filesystem::path& path);
    static void unlock_directory(const std::filesystem::path& path);
    
private:

    std::string _process_query(const std::string& query);
    
    std::string process_create_table(TokenStream& stream);
    std::string process_drop_table(TokenStream& stream);
    std::string process_select(TokenStream& stream);
    std::string process_insert(TokenStream& stream);
    std::string process_delete(TokenStream& stream);
    
    const std::filesystem::path path;

    std::map<std::string, Table> tables;
    
    mutable std::shared_mutex tables_lock;
    
    inline static const std::string MAGIC_FILE_NAME = ".magic.db";
    inline static const std::string LOCK_FILE_NAME = ".lock.db";
};

#endif