#include "library.h"

int main(int argc, char** argv) {
    Schema* schema = (Schema*) malloc(sizeof(Schema));
    read_schema("schema_example.json", schema);

    std::cout << "Number of attributes: " << schema->nattrs << std::endl;
    std::cout << "Attributes:" << std::endl;
    for (int i = 0; i < schema->nattrs; i++) {
        std::cout << i << std::endl;
        Attribute* attr = schema->attrs[i];
        printf("\t%s\n", attr->name);
        printf("\t%s\n", attr->type);
        printf("\t%d\n", attr->length);
    }

    FILE* in_file = fopen("test_data", "r");
    FILE* out_file = fopen("test_out", "w");

    mk_runs(in_file, out_file, 2, schema);
    return 0;
}