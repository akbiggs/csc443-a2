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
    test_open_schema("test_files/schema_example.json", schema);

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
    test_open_schema("test_files/schema_example.json", schema);

    CHECK_EQUAL(schema->record_size, 29LU);

    free(schema);
}

TEST(MakeRuns) {
    FILE* in = fopen("test_files/test_data.csv", "r");
    FILE* out = fopen("test_files/out.csv", "w");
    
    int run_length = 3;

    Schema* schema = (Schema*)malloc(sizeof(Schema));
    test_open_schema("test_files/schema_example.json", schema);

    schema->n_sort_attrs = 1;
    schema->sort_attrs = (int*)malloc(sizeof(int));
    schema->sort_attrs[0] = 3;

    mk_runs(in, out, run_length, schema);

    fclose(out);
    out = fopen("test_files/out.csv", "r");
    
    size_t line_length = schema->record_size + 1;
    char* line = (char*) malloc(line_length);

    int num_lines = 0;
    float cgpas[run_length];

    while (fgets(line, line_length, out)) {

        // check once we've made a run that the values are properly sorted
        if (num_lines % run_length == 0 && num_lines != 0) {
            for (int i = 0; i < run_length - 1; i++) {
                CHECK(cgpas[i] <= cgpas[i + 1]);
            }
        }

        float cgpa = atof(line + 24);
        cgpas[num_lines % run_length] = cgpa;

        num_lines++;
    }

    CHECK_EQUAL(num_lines, 1000);

    fclose(out);
    fclose(in);
}

int main() {
    return UnitTest::RunAllTests();
}
