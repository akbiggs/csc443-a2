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

int record_object_comparator(Record* r1, Record* r2){
    int return_val = 0;

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

int record_comparator(const void* a, const void* b){
    Record* r1 = *((Record **)a);
    Record* r2 = *((Record **)b);
    return record_object_comparator(r1, r2);
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
    this->current_record = (Record*)malloc(sizeof(Record));
    this->current_record->data = (char*)malloc(this->schema->record_size);
    this->current_record->schema = schema;

    this->buffer = (char*) malloc(this->buf_size);
    this->buffer_pointer = this->buffer;
;}

RunIterator::~RunIterator() {
    free(this->current_record->data);
    free(this->current_record);
    free(this->buffer);
}

void RunIterator::read_into_buffer() {
    if (this->left_in_buf > 0 || this->records_left == 0) {
        return;
    }

    if ((unsigned int) (this->buf_size / this->schema->record_size) < this->run_length) {
        this->left_in_buf = this->buf_size / this->schema->record_size;
    } else {
        this->left_in_buf = this->run_length;
    }
    fseek(this->fp, this->file_pos, SEEK_SET);
    memset(this->buffer, '\0', this->buf_size);
    int records_read;
    if ((records_read = fread(this->buffer, this->schema->record_size, this->left_in_buf, this->fp)) < this->left_in_buf) {
        // we are out of records
        this->records_left = records_read;
        this->left_in_buf = records_read;
    }
    this->file_pos += (this->schema->record_size * records_read);
    this->buffer_pointer = this->buffer;
    //printf("read %d records\n", records_read);
}

Record* RunIterator::get_current_record() {
    if (this->records_left == this->run_length) {
        return this->next();
    }
    return this->current_record;
}

Record* RunIterator::next() {
    if (!this->has_next()) {
        this->current_record = NULL;
        return NULL;
    }

    strncpy(this->current_record->data, this->buffer_pointer, this->schema->record_size);

    this->buffer_pointer += this->schema->record_size;
    this->left_in_buf--;
    this->records_left--;

    //printf("record data is %s\n", (this->current_record->data));

    return this->current_record;
}

bool RunIterator::has_next() {
    if (this->left_in_buf == 0) {
        this->read_into_buffer();
    }
    bool potentially_has_next_record = (this->left_in_buf == 0) || (this->buffer != '\0');
    return (this->records_left > 0 && potentially_has_next_record);
}

int RunIterator::get_record_size() {
    return this->schema->record_size;
}
/** MERGE RUNS **/
bool iterators_have_records(RunIterator *iterators[], int num_runs) {
    for (int i = 0; i < num_runs; i++) {
        if (iterators[i]->get_current_record() != NULL) {
            return true;
        }
    }
    return false;
}

RunIterator* get_iterator_with_smallest_value(RunIterator *iterators[], int num_runs) {
    RunIterator* min_record_iterator = NULL;
    for (int i = 0; i < num_runs; i++){
        if (iterators[i]->get_current_record() != NULL){
            if (min_record_iterator != NULL){
                if (record_object_comparator(min_record_iterator->get_current_record(), iterators[i]->get_current_record()) > 0) {
                    min_record_iterator = iterators[i];
                }
            } else {
                min_record_iterator = iterators[i];
            }
        }
    }
    return min_record_iterator;
}

void merge_runs(RunIterator *iterators[], int num_runs, FILE *out_fp,
    long start_pos, char *buf, long buf_size) {

    fseek(out_fp, start_pos, SEEK_SET);
    // Assume proper input i.e. all records are the same size.
    int record_size = iterators[0]->get_record_size();
    int max_number_of_records_in_buffer = floor(buf_size/record_size);

    RunIterator* min_record_iterator;
    int records_in_buffer;
    char* buffer_pos;
    while(iterators_have_records(iterators, num_runs)) {
        records_in_buffer = 0;
        buffer_pos = buf;
        for (int i = 0; i < max_number_of_records_in_buffer; i++) {
            min_record_iterator = get_iterator_with_smallest_value(iterators, num_runs);
            if (min_record_iterator == NULL) {
                break;
            }
            strncpy(buffer_pos, min_record_iterator->get_current_record()->data, record_size);
            records_in_buffer++;
            buffer_pos += record_size;
            min_record_iterator->next();
        }

        fwrite(buf, record_size, records_in_buffer, out_fp);
        fflush(out_fp);
    }
}

