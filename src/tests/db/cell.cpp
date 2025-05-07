#include "doctest.h"
#include <functional>
#include <iostream>

#include "db/cell.h"

using enum Cell::DataType;
using DataType = Cell::DataType;

template <class T>
void check_equal(Cell c, DataType type, T value){
    Cell target(value, type);
    
    CHECK(Cell::is_identical(c, target));
}

TEST_CASE("Cell operations"){
    Cell integer("2", Int);
    Cell real((float)1.5, Float);
    Cell string(12, String);
    Cell null;
    Cell character('e', Char);
    
    check_equal(integer+integer, Int, 4);
    check_equal(integer+real, Float, (float)3.5);
    check_equal(integer + string, String, "212");
    check_equal(integer + null, Null, "");
    check_equal(integer+character, String, "2e");
    
    check_equal(real + real, Float, 3);
    check_equal(real+string, String, "1.512");
    check_equal(real+null, Null, "");
    check_equal(real+character, String, "1.5e");
    
    check_equal(string + string, String, "1212");
    check_equal(string+null, Null, "");
    check_equal(string+character, String, "12e");
    
    check_equal(null+null, Null, "");
    check_equal(null+character, Null, "");
    
    check_equal(character+character, String, "ee");
    
    check_equal(integer * real, Float, 3);
    check_equal(real / integer, Float, (float)0.75);
    
    CHECK_THROWS((string-integer));
    CHECK_THROWS((string*integer));
    CHECK_THROWS((character / real));
}

TEST_CASE("Cell comparisions"){
    Cell integer("2", Int);
    Cell real((float)1.5, Float);
    Cell string(12, String);
    Cell null;
    Cell character('e', Char);
    
    CHECK((integer >= integer));
    CHECK(!(integer > integer));
    CHECK((integer>real));
    CHECK((character > string));
    CHECK((string < integer));
    
    CHECK((string == Cell(12, Int)));
    CHECK((character != integer));
    CHECK((real <= character));
    
    // NULL always compares false
    for(auto i : {integer, real, string, null, character}){
        for(auto op : std::vector<std::function<bool(Cell,Cell)>>{
            std::less<>(), std::greater<>(), std::equal_to<>(), std::not_equal_to<>(),
            std::greater_equal<>(), std::less_equal<>()}){
            CHECK(!(op(null, i)));
            CHECK(!(op(i, null)));
        }
    }
}