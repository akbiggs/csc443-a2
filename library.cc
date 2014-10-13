#include "library.h"

int read_schema(std::string schema_file, Schema *schema) {
    Json::Value root;
    Json::Reader reader;

    std::ifstream file_contents(schema_file);
    if (!reader.parse(file_contents, root, false)) {
        return -1;
    }

    schema->nattrs = root.size();
    schema->attrs = new Attribute *[schema->nattrs];
    for (int i = 0; i < root.size(); i++) {
        Attribute *attribute = (Attribute *) malloc(sizeof(Attribute));
        Json::Value json_attribute = root.get(i, "");

        attribute->name = new char[255];
        strncpy(attribute->name, json_attribute.get("name", "").asCString(), 255);

        if (json_attribute.isMember("type")) {
            attribute->type = new char[255];
            strncpy(attribute->type, json_attribute.get("type", "").asCString(), 255);
        } else {
            attribute->type = "string";
        }

        attribute->length = json_attribute.get("length", -1).asInt();

        schema->attrs[i] = attribute;
    }

    return 0;
}

int record_comparator(const void* a, const void* b){
    // Be warned. Be null terminated.
    // This will not work since we only want to sort on a particular field. Unsure what to put here. palceholder.
    // Something like strcomp(a->at()) or idk;
    return strcmp((const char *)a, (const char *)b);
}

void mk_runs(FILE *in_fp, FILE *out_fp, long run_length, Schema *schema) {
    fseek(in_fp, 0, SEEK_SET);
    fseek(out_fp, 0, SEEK_SET);

    // Unsure what to use so lets use some defaults for now
    int page_size = 8001;
    int num_buffer_pages = (run_length/page_size) + 1; // Minimum for merge sort
    char* buffer_memory = (char*)malloc(page_size * num_buffer_pages);

    char* current_page;
    int run_page_count = 0;
    int total_records_in_run = 0;
    int page_max_record_count = 8;
    int last_page_record_count = 0;
    // Unsure how to use Schema but it's the sum of those. Hard coding for now with shitty value.
    int record_size = 1000;

    do {
        memset(buffer_memory, 0, page_size * num_buffer_pages);

        run_page_count = 0;
        total_records_in_run = 0;

        // Read in as much as possible till we hit a page that isn't full.
        for (int j = 0; j < num_buffer_pages - 1; j++){
            current_page = &buffer_memory[j * page_size];
            fread(current_page, page_size, 1, in_fp);
            // Convert page in to some kind of Record
            run_page_count++;
            // If this happens we don't want to fill up any more buffer pages but just use what we have.
            last_page_record_count = total_records_in_page(current_page);
            if (last_page_record_count < page_max_record_count) {
                break;
            }
        }

        // I'm reading them in, they better be null terminated...

        // In-memory sort
        total_records_in_run = ((run_page_count - 1) * page_max_record_count) + last_page_record_count;
        qsort(buffer_memory, total_records_in_run, record_size, record_comparator);

        // Write all but the last page back in full page sizes.
        for (int j = 0; j < run_page_count - 1; j++){
            current_page = &buffer_memory[j * page_size];
            fwrite(rurrent_page, page_size, 1, out_fp);
            fflush(out_fp);
        }

        // Write the last potentially not full page.
        current_page = &buffer_memory[(run_page_count - 1) * page_size];
        fwrite(current_page, last_page_record_count * record_size, 1, out_fp);
        fflush(out_fp);
    } while (last_page_record_count < page_max_record_count);
    free(buffer_memory);
}

void merge_runs(RunIterator *iterators[], int num_runs, FILE *out_fp,
    long start_pos, char *buf, long buf_size) {
    // Your implementation
}

