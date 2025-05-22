#include "doctest.h"

#include "db/table_serialization.h"
#include "db/table.h"
#include <sstream>
#include <vector>

using enum Cell::DataType;

const std::string serialized = 
"Name,Age,Weight,Gender,\n"
"STRING,INT,FLOAT,CHAR,\n"
"John,\\x,0.5,M,\n"
"Jane,30,\\x,F,\n"
"\\x,28,4,\\x,\n";

TEST_CASE("Table") {
    std::vector<std::pair<Cell::DataType, std::string>> columns = {
        {String, "Name"},
        {Int, "Age"},
        {Float, "Weight"},
        {Char, "Gender"}
    };
    
    Table table(columns);
    
    std::map<std::string, std::string> row1 = {
        {"Name", "John"},
        {"Weight", ".5"},
        {"Gender", "M"}
    };
    table.add_row(row1);
    
    std::map<std::string, std::string> row2 = {
        {"Name", "Jane"},
        {"Age", "30"},
        {"Gender", "F"}
    };
    table.add_row(row2);
    
    std::map<std::string, std::string> row3 = {
        {"Age", "28"},
        {"Weight", "4"},
    };
    table.add_row(row3);
    
    std::ostringstream out;
    
    serialize_table(table, out);
    
    
    SUBCASE("Serialize table"){
        
        CHECK(out.str() == serialized);
    }
}

TEST_CASE("Deserialize and serialize"){
    std::istringstream in(serialized);
    
    Table table =  load_table(in);
    
    std::ostringstream out;
    
    serialize_table(table, out);
    
    CHECK(out.str() == serialized);
}
