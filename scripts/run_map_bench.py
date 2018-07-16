#!/usr/bin/env python3

import os
import subprocess
import sys
import itertools
import concurrent.futures
import json
import re

from datetime import datetime
from pprint import pprint

TYPE_IDS = range(1, 20)
MAX_WORKERS = 8
TIME_CMD = '/usr/bin/time -l'


def run(type_id, build_cmd, speed_test_cmd):
    build_output = subprocess.run(build_cmd, stdout=subprocess.PIPE,
                                  stderr=subprocess.STDOUT, encoding='utf-8', shell=True)
    speed_test_output = subprocess.run(speed_test_cmd, stdout=subprocess.PIPE,
                                       stderr=subprocess.STDOUT, encoding='utf-8', shell=True)
    return type_id, build_output.stdout, speed_test_output.stdout


def parse_build_stdput(build_stdout):
    json_text = re.search(r'(\{.*\})', build_stdout, flags=(re.DOTALL))
    json_dict = json.loads(json_text.group())
    build_stdout = build_stdout[json_text.end():]
    rss_text = re.search(r'(\d+)  maximum resident set size\n',
                         build_stdout, flags=(re.MULTILINE))
    json_dict['max_rss'] = rss_text.group(1)
    return json_dict


def parse_speed_test_stdout(speed_test_stdout):
    json_dict = json.loads(speed_test_stdout)
    return json_dict


if __name__ == "__main__":
    argvs = sys.argv
    argc = len(argvs)

    build_exe = argvs[1]
    speed_test_exe = argvs[2]
    keys_fn = argvs[3]
    queries_fn = argvs[4]
    capa_bits = argvs[5]

    executor = concurrent.futures.ThreadPoolExecutor(max_workers=MAX_WORKERS)

    futures = []
    for type_id in TYPE_IDS:
        build_cmd = '{} ./{} {} {} {}'.format(
            TIME_CMD, build_exe, type_id, keys_fn, capa_bits)
        speed_test_cmd = './{} {} {} {} {}'.format(
            speed_test_exe, type_id, keys_fn, queries_fn, capa_bits)
        futures.append(executor.submit(
            run, type_id, build_cmd, speed_test_cmd))

    results = []
    for future in concurrent.futures.as_completed(futures):
        type_id, build_stdout, speed_test_stdout = future.result()
        results.append((type_id, build_stdout, speed_test_stdout))
    executor.shutdown()

    results.sort(key=lambda x: x[0])

    result_dicts = {}
    for result in results:
        type_id = result[0]
        result_dicts[type_id] = {}
        result_dicts[type_id]['build'] = parse_build_stdput(result[1])
        result_dicts[type_id]['spped'] = parse_speed_test_stdout(result[2])

    print(json.dumps(result_dicts, indent=4))
