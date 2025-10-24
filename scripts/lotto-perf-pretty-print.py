import json
import subprocess
import sys


def print_header():
    print("================")


data_path = '../build/lotto-perf.json'

if len(sys.argv) == 2:
    data_path = sys.argv[1]
elif len(sys.argv) > 2:
    print("To many arguments!")
    exit(0)

# Open the JSON file and load the data
with open(data_path, 'r') as f:
    data = json.load(f)

print("Count per category")
print_header()
for item in data['count_per_category']:
    print(item)
print_header()
print("Count per Task")
print_header()
for item in data['count_per_task']:
    print(item)
print_header()
print("Count per Line")
print_header()
for item in data['count_per_line']:
    line = item['line']
    cnt = item['cnt']
    # addr2line:
    file_path, offset = line.split(' ')
    file_path = file_path[1:-1]  # remove quotes from begging and end
    offset = hex(int(offset))
    command = ["addr2line", "-e", file_path, offset]
    line = subprocess.check_output(command)[:-1].decode("utf-8")

    if line[0] == '?':
        line = file_path + ' ' + offset
    #
    print('line: ', line, ', cnt: ', cnt)
print_header()
