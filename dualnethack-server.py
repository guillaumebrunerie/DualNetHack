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
    def __init__(self, x, y, speed, symbol, parity):
        self.x = x
        self.y = y
        self.waitlist = []
        self.speed = speed
        self.symbol = symbol
        self.mv_points = speed
        self.color = 5
        self.parity = parity
        self.moncanmove = False

    def set_other(self, other):
        self.other = other

    def empty_queue(self):
        self.waitlist = []

    def remove_last(self):
        self.waitlist.pop()
    
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

    def push(self, key):
        self.waitlist.append(key)

    def pop(self, for_real):
        x = self.waitlist.pop(0)
        if self.act(x) and for_real:
            self.mv_points -= 12
        return x

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
        self.parity = random.choice([True, False])

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
        
p1 = Player(6,8,12,'@', False)
p2 = Player(7,8,12,'h', True)
p1.set_other(p2)
p2.set_other(p1)

monsters = [Monster(18,"q",1), Monster(6,"'",2), Monster(36,"E",3), Monster(9,"D",4)]

turn_counter = 0

def send_one_message(sock, data):
    length = len(data)
    sock.sendall(struct.pack('!I', length))
    sock.sendall(data)

def send_screen(p, client, current_p):
    global turn_counter
    client.sendall("CLR")
    client.sendall("TER")

    terrain = field_str.encode("utf-8")
    
    if p.symbol == '@':
        terrain += "You are the human, "
    if p.symbol == 'h':
        terrain += "You are the dwarf, "
    if p == current_p:
        terrain += "it's your turn,     "
    else:
        terrain += "it's not your turn, "
    terrain += "T: " + str(turn_counter)

    # return (init + "\n\r".join("".join(a) for a in screen)).encode("utf-8")
    send_one_message(client, terrain) #field_str.encode("utf-8"))

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
        old_waitlist = list(p.waitlist)
        while p.waitlist:
            p.pop(False)
            send_monster(p, 0)
        p.waitlist = old_waitlist
        p.x = oldx
        p.y = oldy
    fake_moves(p1)
    fake_moves(p2)

    client.sendall("REF")

def movemon(parity):
    moncanmove = False
    for m in monsters:
        if m.parity != parity:
            continue
        if m.mv_points >= 12:
            m.act()
        if m.mv_points >= 12:
            moncanmove = True
    return moncanmove

current_p = None
current_client = None
other_p = None
other_client = None

def main():
    global current_p
    global current_client
    global other_p
    global other_client
    global turn_counter
    
    (clients, client1, client2) = connect()

    def upd(current_p):
        send_screen(p1, client1, current_p)
        send_screen(p2, client2, current_p)

    current_p = p1
    current_client = client1

    other_p = p2
    other_client = client2

    def swap_players():
        global current_p
        global current_client
        global other_p
        global other_client
        if current_p == p1:
            current_p = p2
            current_client = client2
            other_p = p1
            other_client = client1
        else:
            current_p = p1
            current_client = client1
            other_p = p2
            other_client = client2

    while True:
        if current_p.mv_points >= 12:
            while not current_p.waitlist:
                upd(current_p)
                i,o,e = select.select([client1,client2],[],[],60)
                for s in i:
                    key = s.recv(1)
                    if len(key) < 1:
                        print("Aborting")
                        return
                    if s == client1:
                        p = p1
                    if s == client2:
                        p = p2
                        
                    if key == "":
                        p.remove_last()
                    if key == "":
                        p.empty_queue()
                    if key != "" and key != "":
                        p.push(key)
            current_p.pop(True)

        current_p.moncanmove = movemon(current_p.parity)
        
        if other_p.mv_points >= 12 or other_p.moncanmove:
            swap_players()
            continue
        elif current_p.mv_points >= 12 or current_p.moncanmove:
            continue

        # End of the turn

        p1.mv_points += p1.speed
        p2.mv_points += p2.speed
        if current_p == p2:
            swap_players()

        p1.moncanmove = False
        p2.moncanmove = False
            
        for m in monsters:
            m.mv_points += m.speed

        turn_counter += 1

main()
