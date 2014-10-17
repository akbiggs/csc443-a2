#include "library.h"

size_t sizeof_attr(Attribute* attr) {
    return attr->length;
}

int read_schema(const char* schema_file, Schema* schema) {
    Json::Value root;
    Json::Reader reader;

    std::ifstream file_contents(schema_file);
    if (!reader.parse(file_contents, root, false)) {
        return -1;
    }

    schema->nattrs = root.size();
    schema->record_size = 0;
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
        schema->record_size += sizeof_attr(attribute);
    }

    return 0;
}

int offset_to_attribute(Schema *schema, int attr) {
    int offset = 0;
    for (int i = 0; i < attr; i++) {
        offset += 1 + sizeof_attr(schema->attrs[i]);
    }
    return offset;
}

int record_comparator(const void* a, const void* b){
    int return_val = 0;
    int attr;
    int offset;
    Record* r1 = *((Record **)a);
    Record* r2 = *((Record **)b);
    for (int i = 0; i < r1->schema->n_sort_attrs; i++) {
        attr = r1->schema->sort_attrs[i];
        offset = offset_to_attribute(r1->schema, attr);
        return_val = strncmp(r1->data + offset, r2->data + offset, sizeof_attr(r1->schema->attrs[attr]));
        if (return_val != 0) {
            break;
        }
    }
    return return_val;
}

void mk_runs(FILE *in_fp, FILE *out_fp, long run_length, Schema *schema) {
    fseek(in_fp, 0, SEEK_SET);
    fseek(out_fp, 0, SEEK_SET);

    size_t buffer_size = schema->record_size * run_length;
    char* buffer_memory = (char*)malloc(buffer_size);
    long run_record_count = 0;
    Record* records[run_length];

    do {
        run_record_count = 0;
        // Zero out so we can check the number of tecords.
        memset(buffer_memory, '\0', buffer_size);

        // Read in as much as possible till.
        if (fread(buffer_memory, schema->record_size + schema->nattrs, run_length, in_fp) == 0) {
            break;
        }
        //printf("buffer is: %.58s\n\n",buffer_memory);

        // I'm reading them in, they better be null terminated...
        for (int j = 0; j < run_length; j++) {
            Record* record = (Record*)malloc(sizeof(Record));
            record->data = buffer_memory+ (schema->record_size + schema->nattrs) * j;
            if (record->data[0] == '\0') {
                break;
            }
            record->schema = schema;
            records[j] = record;
            run_record_count++;
            //printf("%s\n", record->data);
            //printf("%s\n", records[j]->data);
            //printf("record size is %ld\n", schema->record_size);
        }

        // In-memory sort
        qsort(records, run_record_count, sizeof(Record*), record_comparator);

        // Write back sorted data.
        fwrite(buffer_memory, schema->record_size, run_record_count, out_fp);
        fflush(out_fp);
        for (int j = 0; j < run_record_count; j++) {
            free(records[j]);
        }
    } while (run_record_count == run_length);

    free(buffer_memory);
}

void merge_runs(RunIterator *iterators[], int num_runs, FILE *out_fp,
    long start_pos, char *buf, long buf_size) {
    // Your implementation
}

