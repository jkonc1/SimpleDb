#ifndef DATABASE_H
#define DATABASE_H

#include <string>
#include <memory>
#include "db/table.h"

#include <map>
#include <shared_mutex>


class Database {
public:
    Database();
    std::string process_query(const std::string& query);
    void save(const std::string& path);
    void load(const std::string& path);
private:
    std::map<std::string, std::shared_ptr<Table>> tables;
    
    std::shared_mutex tables_lock;
};

#endif