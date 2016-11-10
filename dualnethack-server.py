#!/usr/bin/python2
# coding=utf8

from __future__ import print_function
import os
import socket
import select
import sys
import Queue
import random
import struct

port = 4242

w, h = 79, 18
field_str = u"""
                                                  ┌───┐
                    ┌──────┐     ┌────┐         ##.....###  ┌─────┐
          ##########.......│    #......#        # │...│  #  │.....│
      #####         │......│  ###│....│#      ### │...│  ###│......#
      #             │.......#####.....│#      #   └───┘    #......│#
 ┌────.────┐        │......│  #  │....│#      #             │.....│#
 │..........#       └─.────┘     └────┘#      #             └─────┘#
 │.........│#         ##               #      #                    #
 │.........│#          #               #      #                    ###
 │.........│#          #               #      #     ┌────────┐       #
 │.........│#          #               # ┌───┐#     │........│       ###
 └─────────┘#    ┌─────.─────┐#        # │....#     │.........###      #
            ###  │............###########....│   ###.........│  #      ###┌──┐
              #  │...........│ #       # │...│  ##  └────────┘  #        #...│
              ###│............##       ##.....###               ###       │..│
                #............│           └───┘                    ########...│
                 └───────────┘                                            └──┘

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

def connect():
    print("Hostname: ", socket.gethostbyname(socket.gethostname()))
    clients = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    clients.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    clients.bind(('',port))
    clients.listen(2)

    (client1, address1) = clients.accept()
    print(address1)
    (client2, address2) = clients.accept()
    print(address2)
    return (clients, client1, client2)
    
class Player():
    def __init__(self, x, y, speed, symbol):
        self.x = x
        self.y = y
        self.waiting_queue = Queue.Queue(0)
        self.speed = speed
        self.symbol = symbol
        self.mv_points = speed
        self.color = 5

    def set_other(self, other):
        self.other = other

    def is_waiting(self):
        return (not self.waiting_queue.empty())
        
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
        newpos = field[newy][newx]
        if newpos == "." or newpos == "#":
            self.x = newx
            self.y = newy
            return True
        return False
            #self.mv_points -= 12

    def push(self, key):
        self.waiting_queue.put(key)

    def pop(self):
        x = self.waiting_queue.get()
        if self.act(x):
            self.mv_points -= 12
        return(x)

    def num_wait(self):
        if self.waiting_queue.qsize() < 10:
            return('0' + str(self.waiting_queue.qsize()))
        else:
            return(str(self.waiting_queue.qsize()))

class Monster():
    def __init__(self, speed, symbol, color):
        while True:
            x = random.randint(0, w-1)
            y = random.randint(0, h-1)
            if field[y][x] == "." or field[y][x] == "#":
                self.x = x
                self.y = y
                break
        self.speed = speed
        self.symbol = symbol
        self.mv_points = speed
        self.color = color

    def act(self):
        dir = random.randint(1,4)
        newx = self.x
        newy = self.y
        if dir == 1:
            newx -= 1
        if dir == 2:
            newy += 1
        if dir == 3:
            newy -= 1
        if dir == 4:
            newx += 1
        newpos = field[newy][newx]
        if newpos == "." or newpos == "#":
            self.x = newx
            self.y = newy
            self.mv_points -= 12
        
p1 = Player(6,8,12,'@')
p2 = Player(7,8,12,'h')
p1.set_other(p2)
p2.set_other(p1)

monsters = [Monster(18,"q",1), Monster(6,"'",2), Monster(36,"E",3), Monster(9,"D",4)]

turn_counter = 0

def send_one_message(sock, data):
    length = len(data)
    sock.sendall(struct.pack('!I', length))
    sock.sendall(data)

def send_screen(p, client):
    client.sendall("CLR")
    client.sendall("TER")
    send_one_message(client, field_str.encode("utf-8"))

    def send_monster(m, bold):
        client.sendall("MON")
        client.sendall(chr(m.x))
        client.sendall(chr(m.y))
        client.sendall(m.symbol)
        client.sendall(chr(m.color))
        client.sendall(chr(bold))
    
    for m in monsters:
        send_monster(m, 1)
    send_monster(p1, 2)
    send_monster(p2, 2)

    def fake_moves(p):
        oldx = p.x
        oldy = p.y
        q = Queue.Queue()
        while not p.waiting_queue.empty():
            x = p.waiting_queue.get()
            p.act(x)
            q.put(x)
            send_monster(p, 0)
        p.waiting_queue = q
        p.x = oldx
        p.y = oldy
    fake_moves(p1)
    fake_moves(p2)

    client.sendall("REF")
    
    # if p.symbol == '@':
    #     init = "You are the human (" + p.num_wait() + ")  T:" + str(turn_counter) + "\n\r"
    # if p.symbol == 'h':
    #     init = "You are the dwarf (" + p.num_wait() + ")  T:" + str(turn_counter) + "\n\r"

    # return (init + "\n\r".join("".join(a) for a in screen)).encode("utf-8")

def check_turn():
    global turn_counter
    some_more = False
    for m in monsters:
        if m.mv_points >= 12:
            m.act()
        if m.mv_points >= 12:
            some_more = True
    if some_more and p1.mv_points < 12 and p2.mv_points < 12:
        check_turn()
        return
    if p1.mv_points >= 12 or p2.mv_points >= 12:
        return
    p1.mv_points += p1.speed
    p2.mv_points += p2.speed
    for m in monsters:
        m.mv_points += m.speed
    turn_counter += 1

# def push(p,key):
#     if p.mv_points < 12:
#         # The other player is supposed to play
#         p.push(key)
#     else:
#         if p.other.mv_points < 12:
#             p.act(key)
#             check_turn()
#         else:
#             if p.other.is_waiting():
#                 p.act(key)
#                 p.other.pop()
#                 check_turn()
#             else:
#                 p.push(key)

def run():
    wecango = True
    if not p1.is_waiting() and p1.mv_points >= 12:
        wecango = False
    if not p2.is_waiting() and p2.mv_points >= 12:
        wecango = False
    if wecango:
        if p1.mv_points >= 12:
            p1.pop()
        if p2.mv_points >= 12:
            p2.pop()
        check_turn()

def remove_last(q):
    r = Queue.Queue()
    while (not q.empty()):
        x = q.get()
        if q.empty():
            return r
        else:
            r.put(x)
    return r
        
def main():
    (clients, client1, client2) = connect()

    def upd():
        print("upd")
        send_screen(p1, client1)
        send_screen(p2, client2)

    upd()

    try:
        while True:
            i,o,e = select.select([client1,client2],[],[],60)
            for s in i:
                key = s.recv(1)
                if len(key) < 1:
                    print("Aborting")
                    clients.close()
                    sys.exit()
                if s == client1:
                    p = p1
                if s == client2:
                    p = p2

                if key == "":
                    p.waiting_queue = remove_last(p.waiting_queue)
                if key == "":
                    p.waiting_queue = Queue.Queue()
                if key != "" and key != "":
                    p.push(key)
                    run()
                upd()
    finally:
        client1.shutdown(socket.SHUT_RDWR)
        client1.close()
        client2.shutdown(socket.SHUT_RDWR)
        client2.close()
        clients.shutdown(socket.SHUT_RDWR)
        clients.close()

main()
