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

static Schema* comparatorSchema;

int offset_to_attribute(Schema *schema, int attr) {
    int offset = 0;
    for (int i = 0; i < schema->nattrs; i++) {
        offset += 1 + sizeof_attr(schema->attrs[i]);
    }
    return offset;
}

int record_comparator(const void* a, const void* b){
    int return_val = 0;
    int attr;
    int offset;
    for (int i = 0; i < comparatorSchema->n_sort_attrs; i++) {
        attr = comparatorSchema->sort_attrs[i];
        offset = offset_to_attribute(comparatorSchema, attr);
        return_val = strncmp((const char *)a +  offset, (const char *)b + offset, sizeof_attr(comparatorSchema->attrs[attr]));
        if (return_val != 0) {
            break;
        }
    }
    return return_val;
}

long total_records_in_page(char* buffer_memory, Schema *schema, long run_length) {
    char* record;
    long record_count = 0;
    for (long i = 0; i < run_length; i++){
        record = &buffer_memory[i * schema->record_size];
        if (record[0] == '\0'){
            return record_count;
        }
        record_count++;
    }
    return record_count;
}

void mk_runs(FILE *in_fp, FILE *out_fp, long run_length, Schema *schema) {
    fseek(in_fp, 0, SEEK_SET);
    fseek(out_fp, 0, SEEK_SET);

    size_t buffer_size = schema->record_size * run_length;
    char* buffer_memory = (char*)malloc(buffer_size);
    long run_record_count = 0;
    Record records[run_length];

    comparatorSchema = schema;

    do {
        // Zero out so we can check the number of tecords.
        memset(buffer_memory, '\0', buffer_size);

        // Read in as much as possible till.
        fread(buffer_memory, schema->record_size, run_length, in_fp);

        // If this ends up being less that page size....we don't want to fill
        // up any more buffer pages but just use what we have.
        run_record_count = total_records_in_page(buffer_memory, schema, run_length);

        // I'm reading them in, they better be null terminated...
        for (int j = 0; j < run_record_count; j++) {
            // Create record using data + schema then we don't need static scheme anymore....
        }

        // In-memory sort
        qsort(buffer_memory, run_record_count, schema->record_size, record_comparator);

        // Write back sorted data.
        fwrite(buffer_memory, schema->record_size, run_record_count, out_fp);
        fflush(out_fp);
    } while (run_record_count < run_length);

    free(buffer_memory);
}

void merge_runs(RunIterator *iterators[], int num_runs, FILE *out_fp,
    long start_pos, char *buf, long buf_size) {
    // Your implementation
}

