#include <cstdlib>
#include <cstdio>
#include <iostream>
#include <fstream>

#include "library.h"
#include "json/json.h"

using namespace std;

int main(int argc, const char* argv[]) {
    if (argc < 7) {
        cout << "ERROR: invalid input parameters!" << endl;
        cout << "Please enter <schema_file> <input_file> <output_file> <mem_capacity> <k> <sorting_attributes>" << endl;
        exit(1);
    }

    Schema* schema = (Schema*) malloc(sizeof(Schema));

    if (read_schema(argv[1], schema)) {
        printf("Could not read schema from %s\n", argv[1]);
        free(schema);
        return 1;
    }
    string schema_file(argv[1]);

    // Print out the schema
    string attr_name;
    int attr_len;
    for (int i = 0; i < schema->nattrs; ++i) {
        attr_name = schema->attrs[i]->name;
        attr_len = schema->attrs[i]->length;
        cout << "{name : " << attr_name << ", length : " << attr_len << "}" << endl;
    }

    schema->n_sort_attrs = argc - 6;
    int sort_attrs[schema->n_sort_attrs];
    int attr_index = -1;
    for (int i = 0; i < schema->n_sort_attrs; i++) {
        attr_index = -1;
        for (int j = 0; j < schema->nattrs; j++) {
            //printf("before strcmp of %s and %s which is %d\n", argv[6 + i], schema->attrs[j]->name, strcmp(argv[6 + i], schema->attrs[j]->name));
            if (strcmp(argv[6 + i], schema->attrs[j]->name) == 0) {
                attr_index = j;
                break;
            }
        }
        if (attr_index == -1) {
            printf("Attr %s was not found in the schema\n", argv[6+i]);
            return 2;
        }
        sort_attrs[i] = attr_index;;
    }

    schema->sort_attrs = sort_attrs;

    if (schema->record_size > atoi(argv[4])) {
        printf("Not enough memeory to read in a record.\n");
        free(schema);
        return 3;
    }

    long memory_capacity = atoi(argv[4]);

    // Do the sort
    FILE *in_fp = fopen(argv[2], "r");
    FILE *tmp_out = fopen("tmp_file", "w");

    long run_length = floor(memory_capacity/schema->record_size);
    mk_runs(in_fp, tmp_out, run_length, schema);
    fclose(tmp_out);

    FILE *runs = fopen("tmp_file", "r");
    FILE *sorted_runs = fopen(argv[3], "w");
    int k = atoi(argv[5]);
    long buf_size = memory_capacity/(k + 1);

    RunIterator *iterators[k];

    for (int i = 0; i < k; i++) {
        iterators[i] = new RunIterator(runs, run_length * schema->record_size * i, run_length, buf_size, schema);
    }
    char buf[memory_capacity];
    merge_runs(iterators, k, sorted_runs, 0, buf, buf_size);

    // end
    free(schema);
    fclose(runs);
    fclose(sorted_runs);
    return 0;
}
