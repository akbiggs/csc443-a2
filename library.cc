#include "library.h"

size_t sizeof_attr(Attribute* attr) {
    int length = attr->length;
    if (!strcmp(attr->type, "string")) {
        // length bytes + 1 for the termination char
        return length + 1;
    } else if (!strcmp(attr->type, "integer")) {
        return sizeof(int);
    } else if (!strcmp(attr->type, "float")) {
        return sizeof(float);
    } 

    fprintf(stderr, "Could not get size of attribute type %s\n", attr->type);
    return 0;
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

void mk_runs(FILE *in_fp, FILE *out_fp, long run_length, Schema *schema) {
    // Your implementation
}

void merge_runs(RunIterator *iterators[], int num_runs, FILE *out_fp,
    long start_pos, char *buf, long buf_size) {
    // Your implementation
}

