#!/usr/bin/python2

from __future__ import print_function
import os
import socket
import select
import sys
import curses

host = '127.0.0.1'
if len(sys.argv) > 1:
    host = sys.argv[1]

port = 4242

def connect():
    print("Connecting to ", host, end="\n\r")
    # Connect to the server
    server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server.connect((host, port))
    print("Connected!")

    return server

def print_screen(toprint):
    os.system("clear")
    print(toprint)

def recvall(sock, count):
    buf = b''
    while count:
        newbuf = sock.recv(count)
        if not newbuf: return None
        buf += newbuf
        count -= len(newbuf)
    return buf

def main(scr):
    server = connect()
    
    while True:
        i,o,e = select.select([sys.stdin, server],[],[],60)
        for s in i:
            if s == sys.stdin:
                key = sys.stdin.read(1)
                server.sendall(key)
            if s == server:
                screen = recvall(server,24+18*(79+2)-2)
                if not screen:
                    server.close()
                    sys.exit()
                print_screen(screen)

curses.wrapper(main)
