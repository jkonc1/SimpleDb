#ifndef TABLE_SERIALIZATION_H
#define TABLE_SERIALIZATION_H

#include "db/table.h"

/**
 * @brief Load a table from an input stream
 */
Table load_table(std::istream& is);

/**
 * @brief Serialize a table to an output stream
 */
void serialize_table(const Table& table, std::ostream& os);

#endif