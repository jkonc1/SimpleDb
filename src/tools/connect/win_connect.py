#!/bin/env python3

import os, sys

if len(sys.argv) != 2:
    print("Usage: python3 win_connect.py <named pipe location>")
    quit()

pipe_path = sys.argv[1]

def read_msg(fd):
    res = ""

    while 1:
        c = os.read(fd, 1).decode()
        if c == "\n":
            break

        res += c
    
    return res

def send_msg(fd, msg):
    os.write(fd, (msg + "\n").encode("utf-8"))


fd = os.open(pipe_path, os.O_RDWR)

print(read_msg(fd))

send_msg(fd, 'peter')

print(read_msg(fd))