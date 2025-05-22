#ifndef ROW_CONTAINER_H
#define ROW_CONTAINER_H

#include "db/table.h"

/**
 * @brief Combine two hash values into one
 */
inline size_t hash_combine(size_t a, size_t b){
    return (998244353 * a + b) ^ (a >> 3) ^ (b<<2);
}

/**
 * @brief Hash for TableRow
 */
struct TableRowHash {
    size_t operator()(const TableRow& row) const {
        size_t result = 1;
        for(auto&& cell : row){
            result = hash_combine(result, std::hash<Cell>()(cell));
        }
        return result;
    }
};

/**
 * @brief Identity comparison for TableRow
 */
struct TableRowIdentical{
    bool operator()(const TableRow& a, const TableRow& b) const {
        if(a.size() != b.size()) {
            return false;
        }
        
        for(size_t i = 0; i < a.size(); i++){
            if(!Cell::is_identical(a[i], b[i])){
                return false;
            }
        }
        return true;
    }
};

#endif