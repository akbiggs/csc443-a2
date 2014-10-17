#include <stdio.h>
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

    CHECK_EQUAL("student_number", schema->attrs[0]->name);
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

TEST(SizeOfRecord) {
    Schema* schema = (Schema*) malloc(sizeof(Schema));
    test_open_schema("schema_example.json", schema);

    CHECK_EQUAL(schema->record_size, 29LU);

    free(schema);
}

TEST(MakeRuns) {
    FILE* in = fopen("mydata.csv", "r");
    FILE* out = fopen("test_out.csv", "w");
    
    int run_length = 2;

    Schema* schema = (Schema*)malloc(sizeof(Schema));
    test_open_schema("schema_example.json", schema);

    schema->n_sort_attrs = 1;
    schema->sort_attrs = (int*)malloc(sizeof(int));
    schema->sort_attrs[0] = 3;

    mk_runs(in, out, run_length, schema);

    fclose(out);
    out = fopen("test_out.csv", "r");
    
    size_t line_length = schema->record_size + 1;
    char* line = (char*) malloc(line_length);

    int num_lines = 0;
    int first_cgpa = 0;
    int second_cgpa = 0;
    while (fgets(line, line_length, out)) {
        int cgpa = atoi(line + 26);

        if (num_lines % 2 == 0) {
            if (num_lines != 0) {
                CHECK(first_cgpa <= second_cgpa);
            }

            first_cgpa = cgpa;
        } else {
            second_cgpa = cgpa;
        }

        num_lines++;
    }

    CHECK_EQUAL(num_lines, 100);

    fclose(out);
    fclose(in);
}

int main() {
    return UnitTest::RunAllTests();
}
