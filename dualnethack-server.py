#!/usr/bin/python2

from __future__ import print_function
import os
import socket
import select
import sys
import Queue

port = 4242

w, h = 79, 18
field_str = """
                                                  -----
                    --------     ------         ##.....###  -------
          ##########.......|    #......#        # |...|  #  |.....|
      #####         |......|  ###|....|#      ### |...|  ###|......#
      #             |.......#####.....|#      #   -----    #......|#
 -----.-----        |......|  #  |....|#      #             |.....|#
 |..........#       --.-----     ------#      #             -------#
 |.........|#         ##               #      #                    #
 |.........|#          #               #      #                    ###
 |.........|#          #               #      #     ----------       #
 |.........|#          #               # -----#     |........|       ###
 -----------#    ------.------#        # |....#     |.........###      #
            ###  |............###########....|   ###.........|  #      ###----
              #  |...........| #       # |...|  ##  ----------  #        #...|
              ###|............##       ##.....###               ###       |..|
                #............|           -----                    ########...|
                 -------------                                            ----

"""

field = [[' ' for x in range(w)] for y in range(h)]
x = 0
y = 0
for i in list(field_str):
    if i == "\n":
        x = 0
        y = y + 1
    else:
        field[y][x] = i
        x = x + 1

def init():
    print("Hostname: ", socket.gethostbyname(socket.gethostname()))
    clients = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    clients.bind(('',port))
    clients.listen(2)

    (client1, address1) = clients.accept()
    print(address1)
    (client2, address2) = clients.accept()
    print(address2)
    return (client1, client2)
    
class Player():
    def __init__(self, x, y, symbol):
        self.x = x
        self.y = y
        self.symbol = symbol
        self.waiting_queue = Queue.Queue(0)

    def set_other(self, other):
        self.other = other

    def is_waiting(self):
        return (not self.waiting_queue.empty())
        
    def act(self, key):
        print(self.symbol, " act")
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
        newpos = field[newy][newx]
        if newpos == "." or newpos == "#":
            self.x = newx
            self.y = newy

    def push(self, key):
        print(self.symbol, " push")
        if self.other.is_waiting():
            self.act(key)
            self.other.pop()
        else:
            self.waiting_queue.put(key)

    def pop(self):
        print(self.symbol, " pop")
        key = self.waiting_queue.get()
        self.act(key)

    def num_wait(self):
        if self.waiting_queue.qsize() < 10:
            return('0' + str(self.waiting_queue.qsize()))
        else:
            return(str(self.waiting_queue.qsize()))
    
p1 = Player(6,8,'@')
p2 = Player(7,8,'h')
p1.set_other(p2)
p2.set_other(p1)

def output_screen(p):
    screen = [[0 for x in range(w)] for y in range(h)]
    for x in range(w):
        for y in range(h):
            screen[y][x] = field[y][x]
    screen[p1.y][p1.x] = p1.symbol
    screen[p2.y][p2.x] = p2.symbol

    if p.symbol == '@':
        init = "You are the human (" + p.num_wait() + ")\n\r"
    if p.symbol == 'h':
        init = "You are the dwarf (" + p.num_wait() + ")\n\r"

    return init + "\n\r".join("".join(a) for a in screen)

def main():
    (client1, client2) = init()

    def upd():
        client1.send(output_screen(p1))
        client2.send(output_screen(p2))

    upd()
    while True:
        i,o,e = select.select([client1,client2],[],[],60)
        for s in i:
            key = s.recv(1)
            if len(key) < 1:
                print("Aborting")
                clients.close()
                sys.exit()
            if s == client1:
                p1.push(key)
            if s == client2:
                p2.push(key)
            upd()

main()
