#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <fstream>

#include "leveldb/db.h"
#include "json/json.h"
#include "library.h"

int main(int argc, const char* argv[]) {

    if (argc < 5) {
        std::cout << "ERROR: invalid input parameters!" << std::endl;
	std::cout << "Please enter <schema_file> <input_file> <out_index> <sorting attributes>" << std::endl;
	return 1;
    }
    
    //Read in schema.
    Schema schema;
    if(read_schema(argv[1], &schema)){
        std::cout << "Could not read schema from " << argv[1] << std::endl;
        return 2;
    }
    
    //Create list of attributes to sort.
    init_sort_attrs(&schema, argv + 4, argc - 4);
    
    //Open input file
    FILE* input_file = fopen(argv[2], "rb");
    if(!input_file){
        std::cout << "Could not read input file " << argv[2] << std::endl;
        return 3;
    }
    
    //Open out index file
    std::ofstream out_index;
    out_index.open(argv[3]);
    if(!out_index.is_open()){
        std::cout << "Could not create out index file " << argv[3] << std::endl;
        return 4;
    }
    
    //Create B+ tree
    leveldb::DB *db;
    leveldb::Options options;
    options.create_if_missing = true;
    leveldb::Status status = leveldb::DB::Open(options, "./leveldb_dir", &db);
    
    if(!status.ok()){
        std::cout << "Failed to create B+tree" << std::endl;
        return 5;
    }
    
    //Read records and insert into B+ tree.
    char buffer[schema.record_size+3];
    char sort_buffer[schema.record_size+1];
    buffer[schema.record_size] = 0;
    
    fgets(buffer, schema.record_size+2, input_file);
    std::cout << "Schema Record Size: " << schema.record_size << std::endl;
    int counter = 0;
    
    leveldb::WriteOptions write_opts;
    
    while(!feof(input_file)){
        std::cout << counter << ": " << buffer << std::endl;
        
        sort_buffer[0] = 0;
        for(int i = 0; i < schema.n_sort_attrs; i++){
            
            //Find start of sort attribute
            int sort_offset = 0;
            for(int s = 0; s < schema.sort_attrs[i]; s++){
                sort_offset += schema.attrs[s]->length + 1;
            }
            
            strncat(sort_buffer, buffer + sort_offset, schema.attrs[schema.sort_attrs[i]]->length);
            std::cout << sort_buffer << std::endl;
        }
        
        leveldb::Slice key = sort_buffer;
        buffer[schema.record_size-1] = 0;
        leveldb::Slice value = buffer;
        db->Put(write_opts, key, value);
        
        fgets(buffer, schema.record_size+2, input_file);
        counter++;
    }
    fclose(input_file);
    
    //Read records from B+ tree and write to 
    std::cout << "Iterating B+ tree" << std::endl;
    leveldb::Iterator* it = db->NewIterator(leveldb::ReadOptions());
    counter = 0;
    for (it->SeekToFirst(); it->Valid(); it->Next()) {
        leveldb::Slice key = it->key();
        leveldb::Slice value = it->value();
        
        std::cout << counter << ":" << key.ToString() << " - " << value.ToString() << std::endl;
        out_index << value.ToString() << std::endl;
        counter++;
    }
    out_index.close();
    delete it;
    delete db;
    
    return 0;
}