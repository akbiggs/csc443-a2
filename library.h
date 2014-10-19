#include <iostream>
#include <fstream>
#include <math.h>
#include <string.h>

#include "json/json.h"
/**
* An attribute schema. You should probably modify
* this to add your own fields.
*/
typedef struct {
    char* name;
    char* type;
    int length;
    // don't need to process the distribution since
    // it's only required for the python data generation
    // script
} Attribute;

/**
* A record schema contains an array of attribute
* schema `attrs`, as well as an array of sort-by
* attributes (represented as the indices of the
* `attrs` array).
*/
typedef struct {
    Attribute **attrs;
    int nattrs;
    int* sort_attrs;
    int n_sort_attrs;

    size_t record_size;
} Schema;

int read_schema(const char* schema_file, Schema *schema);
int offset_to_attribute(Schema *schema, int attr);

/**
* A record can defined as a struct with a pointer
* to the schema and some data.
*/
typedef struct {
    Schema *schema;
    char *data;
} Record;

/**
* Creates sorted runs of length `run_length` in
* the `out_fp`.
*/
void mk_runs(FILE *in_fp, FILE *out_fp, long run_length, Schema *schema);

/**
* The iterator helps you scan through a run.
* you can add additional members as your wish
*/
class RunIterator {
public:
    /**
    * Creates an interator using the `buf_size` to
    * scan through a run that starts at `start_pos`
    * with length `run_length`.
    */
    RunIterator(FILE *fp, long start_pos, long run_length, long buf_size,
        Schema *schema);

    /**
    * free memory
    */
    ~RunIterator();

    /**
    * reads the next record
    */
    Record *next();

    /**
    * return false if iterator reaches the end
    * of the run
    */
    bool has_next();

    int get_record_size();
    Record *get_current_record();

private:
    FILE* fp;
    char* buffer;
    char* buffer_pointer;
    long buf_size;
    long left_in_buf;

    long file_pos;
    long end_file_pos;
    long run_length;
    long records_left;

    void read_into_buffer();
    Schema *schema;
    Record *current_record;
};

/**
* Merge runs given by the `iterators`.
* The number of `iterators` should be equal to the `num_runs`.
* Write the merged runs to `out_fp` starting at position `start_pos`.
* Cannot use more than `buf_size` of heap memory allocated to `buf`.
*/
void merge_runs(RunIterator *iterators[], int num_runs, FILE *out_fp,
    long start_pos, char *buf, long buf_size);

