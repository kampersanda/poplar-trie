#!/usr/bin/env python3

import os
import subprocess
import sys
import itertools
import concurrent.futures
import platform
from datetime import datetime
from argparse import ArgumentParser
from pprint import pprint


def run_command(cmd, pid):
    output = subprocess.run(cmd, stdout=subprocess.PIPE,
                            stderr=subprocess.STDOUT, encoding='utf-8', shell=True)
    return pid, cmd, output.stdout


def parse_stdout(txt):
    ret = {}
    for line in txt.split('\n'):
        if platform.system() == 'Darwin' and 'maximum resident set size' in line:
            ret['max_rss_kb'] = int(line.split()[0]) / 1024
            continue
        elif platform.system() == 'Linux' and 'Maximum resident set size (kbytes)' in line:
            ret['max_rss_kb'] = int(line.split()[5])
            continue
        elif line.startswith('elapsed_sec:'):
            ret['elapsed_sec'] = line.split(':')[1]
        elif line.startswith('\t\tmax_factor:'):
            ret['max_factor'] = line.split(':')[1]
        elif line.startswith('\t\tdsp1st_bits:'):
            ret['dsp1st_bits'] = line.split(':')[1]
        elif line.startswith('\t\tnum_resize:'):
            ret['num_resize'] = line.split(':')[1]
    return ret


if __name__ == "__main__":
    parser = ArgumentParser()
    parser.add_argument('-e', '--exe_cmd', required=True, type=str)
    parser.add_argument('-k', '--key_fn', required=True, type=str)
    parser.add_argument('-b', '--capa_bits', default=16, type=int)
    parser.add_argument('-o', '--output_fn', default='tmp', type=str)
    parser.add_argument('-w', '--max_workers', default=1, type=int)
    args = parser.parse_args()

    executor = concurrent.futures.ThreadPoolExecutor(
        max_workers=args.max_workers)
    futures = []

    time_opt = ''
    if platform.system() == 'Darwin':
        time_opt = '-l'
    elif platform.system() == 'Linux':
        time_opt = '--verbose'
    else:
        print("invalid platform")
        exit()

    max_factors = [80, 85, 90, 95]
    dsp_bits = [3, 4, 5]

    pid = 0
    for db, mf in itertools.product(dsp_bits, max_factors):
        cmd = '/usr/bin/time {opt} ./{exe} -k {key} -t {mf}_{db} -b {capa}'.format(
            opt=time_opt, exe=args.exe_cmd, key=args.key_fn, mf=mf, db=db, capa=args.capa_bits)
        futures.append(executor.submit(run_command, cmd, pid))
        pid += 1

    results = []
    for future in concurrent.futures.as_completed(futures):
        results.append(future.result())
    executor.shutdown()

    results.sort(key=lambda x: x[0])
    time_str = datetime.now().strftime("%Y_%m_%d_%H_%M_%S")

    with open('load_factor_analyze.{}.{}.stdout.txt'.format(args.output_fn, time_str), 'w') as f:
        for res in results:
            f.write(res[1]+'\n')  # cmd
            f.write(res[2]+'\n')  # stdout

    basic_stats = []
    for res in results:
        stdout_dict = parse_stdout(res[2])
        basic_stats.append(stdout_dict)

    with open('load_factor_analyze.{}.{}.basic.txt'.format(args.output_fn, time_str), 'w') as f:
        f.write('max_factor\tdsp1st_bits\telapsed_sec\tmax_rss_kb\tnum_resize\n')
        for bs in basic_stats:
            f.write(
                '{max_factor}\t{dsp1st_bits}\t{elapsed_sec}\t{max_rss_kb}\t{num_resize}\n'.format(**bs))
