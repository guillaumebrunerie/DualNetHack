#!/usr/bin/python2

from __future__ import print_function
import os
import socket
import select
import sys

port = 4242

w, h = 80, 24
field = [['.' for x in range(w)] for y in range(h)]
for x in range(w):
    for y in range(h):
        if x == 0 or x == w-1 or y == 0 or y == h-1:
            field[y][x] = "X"

def init():
    print("Hostname: ", socket.gethostbyname(socket.gethostname()))
    clients = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    clients.bind(('',port))
    clients.listen(2)

    (client1, address1) = clients.accept()
    print(address1)
    return client1
    
class Player():
    def __init__(self, x, y):
        self.x = x
        self.y = y
        self.mvpt = 0
        self.speed = 12

    def act(self, key):
        newx = self.x
        newy = self.y
        if key == "h" or key == "c":
            newx -= 1
        if key == "j" or key == "t":
            newy += 1
        if key == "k" or key == "s":
            newy -= 1
        if key == "l" or key == "r":
            newx += 1
        if field[newy][newx] == ".":
            self.x = newx
            self.y = newy

p1 = Player(w/2,h/2)
# p2 = Player(w/2+1,h/2)

def output_screen():
    screen = [[0 for x in range(w)] for y in range(h)]
    for x in range(w):
        for y in range(h):
            screen[y][x] = field[y][x]
    screen[p1.y][p1.x] = '@'
    # screen[p2.y][p2.x] = '@'

    return "\n\r".join("".join(a) for a in screen)

def main():
    client1 = init()
    
    client1.send(output_screen())
    while True:
        i,o,e = select.select([client1],[],[],60)
        for s in i:
            if s == client1:
                key = client1.recv(1)
                if len(key) < 1:
                    print("Aborting")
                    sys.exit()
                p1.act(key)
                client1.send(output_screen())

main()
