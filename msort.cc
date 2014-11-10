#include <cstdlib>
#include <cstdio>
#include <iostream>
#include <fstream>
#include <math.h>
#include <sys/timeb.h>

#include "library.h"
#include "json/json.h"

using namespace std;

int main(int argc, const char* argv[]) {
    if (argc < 7) {
        cout << "Usage: " << argv[0] << " <schema_file> <input_file> <output_file> <mem_capacity> <k> <sorting_attributes>" << endl;
        exit(1);
    }
    
    const char* schema_file = argv[1];
    Schema* schema = (Schema*) malloc(sizeof(Schema));

    if (read_schema(schema_file, schema)) {
        cout << "Could not read schema from " << schema_file << endl;
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
        return 2;
    }

    unsigned long memory_capacity = atoi(argv[4]);
    if (schema->record_size > memory_capacity) {
        cout << "Not enough memory to read in a record." << endl;
        free(schema);
        return 3;
    }

    //Record Start Time
    struct timeb t;
    ftime(&t);
    long start_ms = t.time * 1000 + t.millitm;
    
    const char* input_file = argv[2];
    const char* output_file = argv[3];
    
    // make the initial runs
    FILE *in_fp = fopen(input_file, "r");
    FILE *tmp_out = fopen("tmp_file", "w");

    long run_length = floor(memory_capacity/schema->record_size);
    mk_runs(in_fp, tmp_out, run_length, schema);
    fclose(tmp_out);

    // merge runs together
    int k = atoi(argv[5]);
    int num_records = number_of_records(in_fp, schema->record_size);
    char buf[memory_capacity];
    long buf_size = memory_capacity/(k + 1);
   
    bool flip = false;
    while (run_length < num_records) {

        int num_runs = ceil((float)num_records / run_length);

        // ugly logic to avoid reading and writing to and from the same file
        const char* read_filename = flip ? "tmp_file2" : "tmp_file";
        const char* write_filename;
        if (num_runs < k || num_runs * run_length == num_records) {
            write_filename = output_file;
        } else {
            write_filename = flip ? "tmp_file" : "tmp_file2";
        }

        RunIterator *iterators[num_runs];
        FILE *runs = fopen(read_filename, "r");
        for (int i = 0; i < num_runs; i++) {
            iterators[i] = new RunIterator(runs, run_length * schema->record_size * i, run_length, buf_size, schema);
        }

        // merge and write back to file
        FILE *sorted_runs = fopen(write_filename, "w");
        merge_runs(iterators, num_runs, sorted_runs, 0, buf, buf_size);
        fclose(sorted_runs);
        fclose(runs);
        
        flip = !flip;
        run_length *= k;
    }
    
    //Calculate program runtime.
    ftime(&t);
    long end_ms = t.time * 1000 + t.millitm;
    
    //Print metrics.
    std::cout << "TOTAL NUMBER OF RECORDS : " << num_records << std::endl;
    std::cout << "TIME: " << (end_ms - start_ms) << std::endl;
    
    // end
    remove("tmp_file");
    remove("tmp_file2");
    
    free(schema);
    fclose(in_fp);
    return 0;
}
