import sys
import os
import subprocess


model = sys.argv[1]
proc_num = int(sys.argv[2])

if model == '-s':
        exe = './discard_server '
        for p in range(0, proc_num):
                args = str(10000+p) + ' ' + str(20000+p) + ' > /dev/null &'
                proc = subprocess.Popen(exe+args, shell=True)

        print('All servers are online. Terminate?')
        while True:
                c = input()
                if c == 'y' or c == 'Y':
                        os.system('killall discard_server')
                        break

if model == '-c':
        server = sys.argv[3]
        exe = './client '
        for p in range(0, proc_num):
                args = f'{server} 9 0 ' + str(20000+p) + ' ' + str(10000+p) + ' > /dev/null 2>&1  &'
                proc = subprocess.Popen(exe+args, shell=True)
                print(exe+args)

        print('All clients are online. Terminate?')
        while True:
                c = input()
                if c == 'y' or c == 'Y':
                        os.system('killall client')
                        break
