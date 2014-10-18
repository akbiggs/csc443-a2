#include "library.h"

int read_schema(const char* schema_file, Schema* schema) {
    Json::Value root;
    Json::Reader reader;

    std::ifstream file_contents(schema_file);
    if (!reader.parse(file_contents, root, false)) {
        return -1;
    }

    schema->nattrs = root.size();
    schema->record_size = schema->nattrs;
    schema->attrs = new Attribute *[schema->nattrs];
    for (unsigned int i = 0; i < root.size(); i++) {
        Attribute *attribute = (Attribute *) malloc(sizeof(Attribute));
        Json::Value json_attribute = root.get(i, "");

        attribute->name = new char[255];
        strncpy(attribute->name, json_attribute.get("name", "").asCString(), 255);

        attribute->type = new char[10];
        if (json_attribute.isMember("type")) {
            strncpy(attribute->type, json_attribute.get("type", "").asCString(), 10);
        } else {
            strncpy(attribute->type, "string", 7);
        }

        attribute->length = json_attribute.get("length", 0).asInt();

        schema->attrs[i] = attribute;
        schema->record_size += attribute->length;
    }

    return 0;
}

int offset_to_attribute(Schema *schema, int attr) {
    int offset = 0;
    for (int i = 0; i < attr; i++) {
        // add 1 to accomodate for the commas separators in the record's data
        offset += 1 + schema->attrs[i]->length;
    }
    return offset;
}

int record_comparator(const void* a, const void* b){
    int return_val = 0;

    Record* r1 = *((Record **)a);
    Record* r2 = *((Record **)b);

    for (int i = 0; i < r1->schema->n_sort_attrs; i++) {
        int attr = r1->schema->sort_attrs[i];
        int offset = offset_to_attribute(r1->schema, attr);
        int attr_length = r1->schema->attrs[attr]->length;

        return_val = strncmp(r1->data + offset, r2->data + offset, attr_length);
        if (return_val != 0) {
            break;
        }
    }

    return return_val;
}

void mk_runs(FILE *in_fp, FILE *out_fp, long run_length, Schema *schema) {
    rewind(in_fp);
    rewind(out_fp);

    size_t buffer_size = schema->record_size * run_length;
    char* buffer_memory = (char*)malloc(buffer_size);
    long run_record_count = 0;
    Record* records[run_length];

    do {
        run_record_count = 0;

        // Zero out so we can check the number of tecords.
        memset(buffer_memory, '\0', buffer_size);

        // Read in as much as possible till.
        if (fread(buffer_memory, schema->record_size, run_length, in_fp) == 0) {
            break;
        }

        // I'm reading them in, they better be null terminated...
        for (int j = 0; j < run_length; j++) {
            Record* record = (Record*)malloc(sizeof(Record));
            record->data = buffer_memory + schema->record_size * j;
            if (record->data[0] == '\0') {
                break;
            }

            record->schema = schema;
            records[j] = record;

            run_record_count++;
        }

        // In-memory sort
        qsort(records, run_record_count, sizeof(Record*), record_comparator);

        // write back sorted data
        for (int j = 0; j < run_record_count; j++) {
            fwrite(records[j]->data, schema->record_size, 1, out_fp);
            free(records[j]);
        }
        fflush(out_fp);

    } while (run_record_count == run_length);

    free(buffer_memory);
}

/** RUN ITERATOR **/

RunIterator::RunIterator(FILE* fp, long start_pos, long run_length, long buf_size,
        Schema* schema) {
    this->fp = fp;
    fseek(this->fp, start_pos, SEEK_SET);

    this->buf_size = buf_size;
    this->left_in_buf = 0;

    this->file_pos = start_pos;
    this->run_length = run_length;
    this->records_left = this->run_length;
    this->end_file_pos = start_pos + (run_length * schema->record_size);

    this->schema = schema;
}

RunIterator::~RunIterator() {
    fclose(this->fp);
    free(schema);
}

void RunIterator::read_into_buffer() {
    if (this->left_in_buf > 0) {
        return;
    }

    if ((unsigned int) (this->buf_size / this->schema->record_size) < this->run_length) {
        this->left_in_buf = this->buf_size / this->schema->record_size;
    } else {
        this->left_in_buf = this->run_length;
    }

    this->buffer = (char*) malloc(this->left_in_buf * this->schema->record_size);
    memset(this->buffer, '\0', this->buf_size);
    int records_read;
    if ((records_read = fread(this->buffer, this->schema->record_size, this->left_in_buf, this->fp)) < this->left_in_buf) {
        // we are out of records
        this->records_left = records_read;
    }
}

Record* RunIterator::next() {
    if (!this->has_next()) {
        return NULL;
    }

    char* record_data = (char*)malloc(this->schema->record_size);
    strncpy(record_data, this->buffer, this->schema->record_size);

    this->buffer += this->schema->record_size;
    this->left_in_buf--;
    this->records_left--;

    Record* record = (Record*)malloc(sizeof(Record));
    record->data = record_data;
    record->schema = schema;

    //printf("record data is %s\n", record_data);

    return record;
}

bool RunIterator::has_next() {
    if (this->left_in_buf == 0) {
        this->read_into_buffer();
    }
    bool potentially_has_next_record = (this->left_in_buf == 0) || (this->buffer != '\0');
    return (this->records_left > 0 && potentially_has_next_record);
}

/** MERGE RUNS **/
void merge_runs(RunIterator *iterators[], int num_runs, FILE *out_fp,
    long start_pos, char *buf, long buf_size) {
    // Your implementation
}



RunIterator::RunIterator(FILE *fp, long start_pos, long run_length, long buf_size,
        Schema *schema){

}

Record* RunIterator::next(){
    return 0;
}

bool RunIterator::has_next(){
    return false;
}

RunIterator::~RunIterator(){

}
