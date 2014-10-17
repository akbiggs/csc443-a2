#!/bin/bash

make test
python data_generator.py test_files/schema_example.json test_files/test_data.csv 1000
./test
