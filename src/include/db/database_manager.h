#ifndef DATABASE_MANAGER_H
#define DATABASE_MANAGER_H

#include "db/database.h"

#include <filesystem>
#include <memory>

/**
 * @brief Binds a Database to a filesystem location
          Enables loading and saving the database
 */
class DatabaseManager {
public:
    /**
     * @param database_directory Path to the directory of the database
     */
    DatabaseManager(const std::filesystem::path& database_directory);
    
    ~DatabaseManager();
    
    /**
     * @brief Load the database from filesystem
     * @throws `std::runtime_error` if loading fails
     */
    void load();
    
    /**
     * @brief Save database to the filesystem
     * @throws `std::runtime_error` if loading fails
     */
    void save();
    
    /**
     * @brief Check if database is loaded
     */
    bool is_loaded() const;
    
    /**
     * @brief Get a reference to the contained database
     * @throws `std::runtime_error` if database is not loaded
     */
    Database& get();
private:
    /**
     * @brief Initialize a database directory
     */
    static void init_directory(const std::filesystem::path& path);
    
    /**
     * @brief Check if a directory is a database
     */
    static void check_directory(const std::filesystem::path& path);
    
    /**
     * @brief Lock a database directory
     * @details Creates a lock file
     * @throws `std::runtime_error` if the directory is already locked or cannot be locked
     */
    static void lock_directory(const std::filesystem::path& path);
    
    /**
     * @brief Unlock a database directory
     * @details Removes the lock file to allow other processes to access the database
     * @throws `std::runtime_error` if the directory is not locked or cannot be unlocked
     */
    static void unlock_directory(const std::filesystem::path& path);
    
    /**
     * @brief Check if a file is a database table file (as compared to a lock or magic)
     */
    static bool is_table_file(const std::filesystem::path& filename);

    std::filesystem::path path;
    
    std::unique_ptr<Database> database;
    
    inline static const std::string MAGIC_FILE_NAME = ".magic.db";
    
    inline static const std::string LOCK_FILE_NAME = ".lock.db";
};

#endif