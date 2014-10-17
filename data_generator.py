import csv

from json import load
from random import choice, uniform, gauss
from string import ascii_lowercase, digits
from sys import argv, exit
from decimal import Decimal

def generate_data(schema, out_file, nrecords):
    '''
    Generate data according to the `schema` given,
    and write to `out_file`.
    `schema` is an list of dictionaries in the form of:
    [
        {
            'name' : <attribute_name>,
            'length' : <fixed_length>,
            ...
        },
        ...
    ]
    `out_file` is the name of the output file.
    The output file must be in csv format, with a new line
    character at the end of every record.
    '''
    print "Generating {} records".format(nrecords)
    with open(out_file, 'w') as records_file:
        csv_writer = csv.writer(records_file)
        for i in range(nrecords):
            csv_writer.writerow(generate_record(schema))


def generate_record(schema):
    record = []
    for col in schema:
        distribution = col.get('distribution')
        length = col.get('length')

        record_data = None
        if distribution:
            value = None
            if distribution.get('name') == 'uniform':
                value = int(uniform(distribution.get('min'), distribution.get('max')))
            elif distribution.get('name') == 'normal':
                value = round(gauss(mu=distribution.get('mu'), sigma=distribution.get('sigma')), 2)
            if value:
                if col.get('type') == 'integer':
                    record_data = str(value).zfill(length)
                else:
                    digits_before_point = len(str(value).split(".")[0])
                    format_str = "10." + "0" * (length - digits_before_point - 1)
                    record_data = str(Decimal(value).quantize(Decimal(format_str)))

        else:
            alphabet = digits + ascii_lowercase
            record_data = ''.join(choice(alphabet) for _ in range(length))
        
        if record_data:
            record.append(record_data)

    print 'creating record: {}'.format(','.join(record))
    return record


if __name__ == '__main__':
    if not len(argv) == 4:
        print "data_generator.py <schema json file> <output csv file> <# of records>"
        exit(2)

    schema = load(open(argv[1]))
    output = argv[2]
    nrecords = int(argv[3])
    print schema

    generate_data(schema, output, nrecords)
