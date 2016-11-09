#!/usr/bin/python2

from __future__ import print_function
import os
import socket
import select
import sys
import curses

host = '127.0.0.1'
port = 4242

def connect():
    print("Connecting to ", host)
    # Connect to the server
    server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server.connect((host, port))

    return server

def print_screen(toprint):
    os.system("clear")
    print(toprint)
    
def main(scr):
    server = connect()
    
    while True:
        i,o,e = select.select([sys.stdin, server],[],[],60)
        for s in i:
            if s == sys.stdin:
                key = sys.stdin.read(1)
                server.send(key)
            if s == server:
                screen = server.recv(24+18*(79+2)-2)
                if len(screen) < 24+18*(79+2)-2:
                    print("Aborting, bytes received: ", len(screen))
                    sys.exit()
                print_screen(screen)

curses.wrapper(main)
