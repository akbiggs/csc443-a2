#!/bin/bash

if [ `make test` -ne 0 ] ; then
    exit 1
fi

python generate_data.py test_files/schema_example.json test_files/test_data.csv 1000
./test
