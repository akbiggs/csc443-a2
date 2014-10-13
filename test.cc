#include "library.h"

#include "test_lib/UnitTest++.h"
#include "test_lib/ReportAssert.h"

using namespace UnitTest;

void test_open_schema(const char* filename, Schema* schema) {
    if (read_schema(filename, schema)) {
        printf("Could not read schema from %s\n", filename);
        free(schema);
        CHECK(false);
    }
}

TEST(ReadSchema) {
    Schema* schema = (Schema*) malloc(sizeof(Schema));
    test_open_schema("schema_example.json", schema);

    CHECK_EQUAL(4, schema->nattrs);

    CHECK_EQUAL("ttudent_number", schema->attrs[0]->name);
    CHECK_EQUAL(9, schema->attrs[0]->length);
    CHECK_EQUAL("string", schema->attrs[0]->type);

    CHECK_EQUAL("account_name", schema->attrs[1]->name);
    CHECK_EQUAL(8, schema->attrs[1]->length);
    CHECK_EQUAL("string", schema->attrs[1]->type);

    CHECK_EQUAL("start_year", schema->attrs[2]->name);
    CHECK_EQUAL(4, schema->attrs[2]->length);
    CHECK_EQUAL("integer", schema->attrs[2]->type);

    CHECK_EQUAL("cgpa", schema->attrs[3]->name);
    CHECK_EQUAL(4, schema->attrs[3]->length);
    CHECK_EQUAL("float", schema->attrs[3]->type);

    free(schema);
}

int main() {
    return UnitTest::RunAllTests();
}
