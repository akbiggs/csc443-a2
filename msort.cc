#include <cstdlib>
#include <cstdio>
#include <iostream>
#include <fstream>
#include <sys/timeb.h>

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
        cout << "Could not read schema from " << argv[1] << endl;
        free(schema);
        return 1;
    }
    
    // Print out the schema
    string attr_name;
    int attr_len;
    for (int i = 0; i < schema->nattrs; ++i) {
        attr_name = schema->attrs[i]->name;
        attr_len = schema->attrs[i]->length;
        cout << "{name : " << attr_name << ", length : " << attr_len << "}" << endl;
    }

    if(init_sort_attrs(schema, argv + 6, argc - 6)){
        cout << "Failed to find sort attribute in schema" << endl;
    }

    if (schema->record_size > atoi(argv[4])) {
        cout << "Not enough memeory to read in a record." << endl;
        free(schema);
        return 3;
    }

    long memory_capacity = atoi(argv[4]);

    //Record Start Time
    struct timeb t;
    ftime(&t);
    long start_ms = t.time * 1000 + t.millitm;
    
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

    
    //Calculate program runtime.
    ftime(&t);
    long end_ms = t.time * 1000 + t.millitm;
    
    //Print metrics.
    std::cout << "TIME: " << (end_ms - start_ms) << std::endl;
    std::cout << "TOTAL NUMBER OF RECORDS : " << 0 << std::endl;
    
    // end
    free(schema);
    fclose(runs);
    fclose(sorted_runs);
    return 0;
}
