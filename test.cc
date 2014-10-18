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

TEST(AttributeOffset) {
    Schema* schema = (Schema*) malloc(sizeof(Schema));
    test_open_schema("test_files/schema_example.json", schema);

    CHECK_EQUAL(offset_to_attribute(schema, 0), 0);
    CHECK_EQUAL(offset_to_attribute(schema, 1), 10);
    CHECK_EQUAL(offset_to_attribute(schema, 2), 19);
    CHECK_EQUAL(offset_to_attribute(schema, 3), 24);

    free(schema);
}

TEST(MakeRuns) {
    FILE* in = fopen("test_files/test_data.csv", "r");
    FILE* out = fopen("test_files/out.csv", "w");

    int run_length = 7;

    Schema* schema = (Schema*)malloc(sizeof(Schema));
    test_open_schema("test_files/schema_example.json", schema);

    schema->n_sort_attrs = 2;
    schema->sort_attrs = (int*)malloc(sizeof(int)*2);
    schema->sort_attrs[0] = 3;
    schema->sort_attrs[1] = 2;

    mk_runs(in, out, run_length, schema);

    fclose(out);
    out = fopen("test_files/out.csv", "r");

    size_t line_length = schema->record_size + 1;
    char* line = (char*) malloc(line_length);

    int num_lines = 0;
    float cgpas[run_length];
    int start_years[run_length];

    while (fgets(line, line_length, out)) {

        // check once we've made a run that the values are properly sorted
        if (num_lines % run_length == 0 && num_lines != 0) {
            for (int i = 0; i < run_length - 1; i++) {
                CHECK(cgpas[i] <= cgpas[i + 1]);

                if (cgpas[i] == cgpas[i + 1]) {
                    CHECK(start_years[i] <= start_years[i + 1]);
                }
            }
        }

        float cgpa = atof(line + offset_to_attribute(schema, 3));
        int start_year = atoi(line + offset_to_attribute(schema, 2));

        cgpas[num_lines % run_length] = cgpa;
        start_years[num_lines % run_length] = start_year;

        num_lines++;
    }

    CHECK_EQUAL(num_lines, 1000);

    fclose(out);
    fclose(in);
}

TEST(MakeRunIterator) {
    FILE* fp = fopen("test_files/out.csv", "r");

    Schema* schema = (Schema*) malloc(sizeof(Schema));
    test_open_schema("test_files/schema_example.json", schema);

    int run_length = 7;
    int start = schema->record_size * run_length * 8;
    RunIterator* ri = new RunIterator(fp, start, run_length,
            schema->record_size * 3, schema);

    FILE* fp2 = fopen("test_files/out.csv", "r");
    fseek(fp2, start, SEEK_SET);

    char data[schema->record_size + 1];
    for (int i = 0; i < run_length; i++) {
        CHECK(ri->has_next());
        CHECK(ri->has_next()); // make sure has_next() can be run multiple times
        Record* r = ri->next();

        fread(data, schema->record_size, 1, fp2);

        //printf("data is %s\n", data);
        //printf("rdata is %s\n", r->data);
        CHECK(strncmp(r->data, data, schema->record_size) == 0);
    }

    CHECK(!ri->has_next());

    free(ri);
    fclose(fp);
}

int main() {
    return UnitTest::RunAllTests();
}
