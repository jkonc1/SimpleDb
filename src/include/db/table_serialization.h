#ifndef TABLE_SERIALIZATION_H
#define TABLE_SERIALIZATION_H

#include "db/table.h"

Table load_table(std::istream& is);

void serialize_table(const Table& table, std::ostream& os);

#endif