#!/usr/bin/python2
# coding=utf8

from __future__ import print_function
import os
import socket
import select
import sys
import curses
import struct
import locale
locale.setlocale(locale.LC_ALL, "")

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

def recvall(sock, count):
    buf = b''
    while count:
        newbuf = sock.recv(count)
        if not newbuf: return None
        buf += newbuf
        count -= len(newbuf)
    return buf

def recv_one_message(sock):
    lengthbuf = recvall(sock, 4)
    length, = struct.unpack('!I', lengthbuf)
    return recvall(sock, length)

def main(scr):
    server = connect()
    curses.curs_set(0)
    curses.init_pair(1, curses.COLOR_RED, curses.COLOR_BLACK)
    curses.init_pair(2, curses.COLOR_YELLOW, curses.COLOR_BLACK)
    curses.init_pair(3, curses.COLOR_BLUE, curses.COLOR_BLACK)
    curses.init_pair(4, curses.COLOR_GREEN, curses.COLOR_BLACK)
    curses.init_pair(5, curses.COLOR_WHITE, curses.COLOR_BLACK)
    
    while True:
        i,o,e = select.select([sys.stdin, server],[],[],60)
        for s in i:
            if s == sys.stdin:
                key = sys.stdin.read(1)
                server.sendall(key)
            if s == server:
                cmd = recvall(server,3)
                if not cmd:
                    server.close()
                    sys.exit()
                if cmd == "REF":
                    scr.refresh()
                if cmd == "CLR":
                    scr.clear()
                if cmd == "TER":
                    scr.addstr(0,0,recv_one_message(server))
                if cmd == "MON":
                    x = ord(recvall(server, 1))
                    y = ord(recvall(server, 1))
                    char = recvall(server, 1)
                    color = ord(recvall(server, 1))
                    bd = ord(recvall(server, 1))
                    bold = curses.A_DIM
                    if bd == 1:
                        bold = curses.A_BOLD
                    if bd == 2:
                        bold = curses.A_REVERSE

                    scr.addstr(y, x, char, bold | curses.color_pair(color))

curses.wrapper(main)
