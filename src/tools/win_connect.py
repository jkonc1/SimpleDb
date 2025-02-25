import os

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


fd = os.open(r'\\.\pipe\db_pipe', os.O_RDWR)

print(read_msg(fd))

send_msg(fd, 'peter')

print(read_msg(fd))