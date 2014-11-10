from subprocess import call
import sys

if __name__ == '__main__':
    call(['./msort', 'test_files/schema_example.json',
        'test_files/verylarge.csv', 'test_files/out.csv', '65536', '4',
        'cgpa', 'start_year']);
    with open('test_files/out.csv') as f:
        prev_cgpa = -1.0
        n_lines = 0
        for line in f:
            n_lines += 1
            cgpa = float(line.split(',')[3])
            if cgpa < prev_cgpa:
                print("FAILURE AT CGPA {}".format(cgpa))
                sys.exit(1)
            prev_cgpa = cgpa
    
    if n_lines < 100000:
        print("FAILURE: NUM LINES IS {}".format(n_lines))
    else:
        print("SUCCESS")
