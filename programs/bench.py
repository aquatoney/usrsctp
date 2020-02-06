import sys
import os
import subprocess


model = sys.argv[1]
proc_num_range = sys.argv[2]
proc_num_start = int(proc_num_range.split(':')[0])
proc_num_end = int(proc_num_range.split(':')[1])
proc_num = proc_num_end - proc_num_start + 1


if model == '-s':
        exe = './discard_server_upcall '
        for p in range(proc_num_start, proc_num_end):
                args = str(10000+p) + ' ' + str(20000+p) + ' > /dev/null &'
                proc = subprocess.Popen(exe+args, shell=True)

        print('All servers are online. Terminate?')
        while True:
                c = input()
                if c == 'y' or c == 'Y':
                        os.system('killall discard_server_upcall')
                        break

if model == '-c':
        server = sys.argv[3]
        exe = './client_upcall '
        for p in range(proc_num_start, proc_num_end):
                args = f'{server} 9 {p} ' + str(20000+p) + ' ' + str(10000+p) + ' > /dev/null 2>&1  &'
                proc = subprocess.Popen(exe+args, shell=True)
                print(exe+args)

        print('All clients are online. Terminate?')
        while True:
                c = input()
                if c == 'y' or c == 'Y':
                        os.system('killall client_upcall')
                        break
