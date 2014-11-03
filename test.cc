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
    int i;
    for (i = 0; i < run_length; i++) {
        CHECK(ri->has_next());
        CHECK(ri->has_next()); // make sure has_next() can be run multiple times
        Record* r = ri->next();

        if (fread(data, schema->record_size, 1, fp2) == 0) {
            break;
        }

        //printf("data is %s\n", data);
        //printf("rdata is %s\n", r->data);
        CHECK(strncmp(r->data, data, schema->record_size) == 0);
    }
    CHECK(i == 7);

    CHECK(!ri->has_next());

    free(ri);
    fclose(fp);
}


TEST(MakeRunIteratorRunLengthTooLong) {
    FILE* fp = fopen("test_files/5_records.csv", "r");

    Schema* schema = (Schema*) malloc(sizeof(Schema));
    test_open_schema("test_files/schema_example.json", schema);

    int run_length = 7;
    int start = 0;
    RunIterator* ri = new RunIterator(fp, start, run_length,
            schema->record_size * 3, schema);

    FILE* fp2 = fopen("test_files/5_records.csv", "r");

    char data[schema->record_size + 1];
    int i;
    for (i = 0; i < run_length; i++) {
        if (!ri->has_next()) {
            break;
        }
        CHECK(ri->has_next());
        CHECK(ri->has_next()); // make sure has_next() can be run multiple times
        Record* r = ri->next();

        if (fread(data, schema->record_size, 1, fp2) == 0) {
            break;
        }

        //printf("data is %s\n", data);
        //printf("rdata is %s\n", r->data);
        CHECK(strncmp(r->data, data, schema->record_size) == 0);
    }
    //printf("i is %d\n", i);
    CHECK(i == 5);

    CHECK(!ri->has_next());

    free(ri);
    fclose(fp);
}

TEST(MakeRun) {
    FILE* fp = fopen("test_files/5_records.csv", "r");
    FILE* out_fp = fopen("test_files/5_records_runs.csv", "w");

    Schema* schema = (Schema*) malloc(sizeof(Schema));
    test_open_schema("test_files/schema_example.json", schema);
    schema->n_sort_attrs = 2;
    schema->sort_attrs = (int*)malloc(sizeof(int)*2);
    schema->sort_attrs[0] = 3;
    schema->sort_attrs[1] = 2;
    int run_length = 2;


    mk_runs(fp, out_fp, 2, schema);
    fclose(out_fp);
    fclose(fp);

    fp = fopen("test_files/5_records_runs.csv", "r");
    out_fp = fopen("test_files/5_records_sorted.csv", "w");
    int start1 = 0;
    int start2 = schema->record_size * run_length;
    int start3 = schema->record_size * run_length * 2;
    RunIterator* ri1 = new RunIterator(fp, start1, run_length,
            schema->record_size * 3, schema);
    RunIterator* ri2 = new RunIterator(fp, start2, run_length,
            schema->record_size * 3, schema);
    RunIterator* ri3 = new RunIterator(fp, start3, run_length,
            schema->record_size * 3, schema);

    RunIterator* iterators[3] = {ri1, ri2, ri3};

    long start_pos = 0;
    char buf[300];
    long buf_size = 3 * schema->record_size;
    merge_runs(iterators, 3, out_fp, start_pos, buf, buf_size);
    fclose(fp);
    fclose(out_fp);

    FILE* expected_fp = fopen("test_files/5_records_sorted_expected.csv", "r");
    FILE* result_fp = fopen("test_files/5_records_sorted.csv", "r");
    char expected_record[schema->record_size + 1];
    char result_record[schema->record_size + 1];

    expected_record[schema->record_size + 1] = '\0';
    result_record[schema->record_size + 1] = '\0';

    int i;
    for (i = 0; i < 5; i++) {
        if(fread(expected_record, schema->record_size, 1, expected_fp) == 0) {
            break;
        };
        if (fread(result_record, schema->record_size, 1, result_fp) == 0) {
            break;
        }
        //printf("data is %s\n", result_record);
        //printf("expected data is %s\n", expected_record);
        CHECK(strcmp(expected_record, result_record) == 0);
    }
    CHECK(i == 5);
    fclose(result_fp);
    fclose(expected_fp);
    free(schema);
}

TEST(NumRecords) {
    FILE* fp = fopen("test_files/5_records.csv", "r");
    FILE* fp2 = fopen("test_files/test_data.csv", "r");
    Schema* schema = (Schema*) malloc(sizeof(Schema));
    test_open_schema("test_files/schema_example.json", schema);
    
    int fp_loc = ftell(fp);
    int fp2_loc = ftell(fp2);
    
    CHECK(number_of_records(fp, schema->record_size) == 5);
    CHECK(number_of_records(fp2, schema->record_size) == 1000);

    CHECK(ftell(fp) == fp_loc);
    CHECK(ftell(fp2) == fp2_loc);
    
    free(schema);
    fclose(fp);
    fclose(fp2);
}

int main() {
    return UnitTest::RunAllTests();
}
