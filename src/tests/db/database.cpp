#include "doctest.h"

#include "db/database.h"

bool is_response_ok(const std::string& response) {
    return response.starts_with("OK ");
}

TEST_CASE("Create and drop tables"){
    Database db;
    
    std::string resp;
    
    resp = db.process_query("CREATE TABLE users (id INT, name STRING, email STRING);");
    
    INFO(resp);
    
    CHECK(is_response_ok(resp));
    
    resp = db.process_query("DROP TABLE users;");
    
    INFO(resp);
    CHECK(is_response_ok(resp));

    resp = db.process_query("DROP TABLE users;");
    
    INFO(resp);
    CHECK(!is_response_ok(resp));
    
    resp = db.process_query("cReaTE table tabel123 (id iNt);");
    
    INFO(resp);
    CHECK(is_response_ok(resp));
}
