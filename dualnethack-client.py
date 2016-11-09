#!/usr/bin/python2

from __future__ import print_function
import os
import socket
import select
import sys
import curses

host = "172.56.28.91" #'127.0.0.1'
port = 4242

def init():
    # Initializes curses
    curses.initscr()
    curses.noecho()
    curses.cbreak()

    print("Connecting to ", host)
    # Connect to the server
    server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server.connect((host, port))

    return server

def print_screen(scr,toprint):
#    scr.clear()
#    scr.addstr(0,0,toprint)
    os.system("clear")
    print(toprint)
    
def main(scr):
    server = init()
    
    while True:
        i,o,e = select.select([sys.stdin, server],[],[],60)
        for s in i:
            if s == sys.stdin:
                key = sys.stdin.read(1)
                server.send(key)
            if s == server:
                screen = server.recv(1966)
                if len(screen) < 1966:
                    print("Aborting, bytes received: ", len(screen))
                    sys.exit()
                print_screen(scr,screen)

main(None)
