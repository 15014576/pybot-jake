#!/usr/bin/env python

import botbasic, time
import botlib
from chvec import *
import math

#globals
armor = 100
enemyInSight = False
healthLow = False
armorLow = False
weaponLow = False

#
# bot nav - main priority, completely broken but should work
#

# look through nav code, try and understand why the bot doesn't move.


#
# item problem - second priotity
#

# get bot pos
# get array of medkit/weapon locations
# check difference between botpos and item locations to find closest one
# plot path to loc of closest medkit/weapon

#
# room problem - could be left until M3
#

# get bot pos
# create waypoints in each room
# check which waypoint is furthest away and plot path to there

#
# enemy in sight problem - quick
#

# fixed in coursework so check how coursework did it and test

# init binary variables
def init():
    global armor, healthLow, armorLow, weaponLow, enemyInSight

    # check if enemy in sight
    you = findYou (b)
    if b.aim(you): # doesnt search to see if player is in sight
        enemyInSight = True
    else:
        enemyInSight = False

    # check health levels
    if b.health() > 30:
        healthLow = True
    else:
        healthLow = False

    # check armor levels
    if armor > 30:
        armorLow = True
    else:
        armorLow = False

    # check how good weapon is
    if b.getCurrentWeapon() >= 0:
        weaponLow = True
    else:
        weaponLow = False

# binary tree
def tree (health, armor, weapon, emyinsig):
    me = b.me()
    you = findYou (b)

    if emyinsig == True and health == False and armor == False and weapon == False:
        print "Retreating from battle!"
        # getpos of another room
        # move to there

    elif emyinsig == True and health == True and armor == False and weapon == False:
        print "Shooting at enemy!"
        b.aim(you)
        b.start_firing ()
        b.select[("fire")]

    elif emyinsig == True and health == False and armor == True and weapon == False:
        print "Retreating from battle!"
        # see above
    elif emyinsig == True and health == False and armor == False and weapon == True:
        print "Shooting at enemy!"
        b.aim(you)
        b.start_firing ()
        b.select[("fire")]

    elif emyinsig == True and health == True and armor == True and weapon == False:
        print "Shooting at enemy!"
        b.aim(you)
        b.start_firing ()
        b.select[("fire")]

    elif emyinsig == True and health == False and armor == True and weapon == True:
        print "Shooting at enemy!"
        b.aim(you)
        b.start_firing ()
        b.select[("fire")]

    elif emyinsig == True and health == True and armor == False and weapon == True:
        print "Shooting at enemy!"
        b.aim(you)
        b.start_firing ()
        b.select[("fire")]

    elif emyinsig == True and health == True and armor == True and weapon == True:
        print "Shooting at enemy!"
        b.aim(you)
        b.start_firing ()
        b.select[("fire")]

    elif emyinsig == False and health == False and armor == False and weapon == False:
        print "Going to find health!"
        # get pos of health kit
        # go to health kit
    elif emyinsig == False and health == True and armor == False and weapon == False:
        print "Going to find weapon!"
    elif emyinsig == False and health == False and armor == True and weapon == False:
        print "Going to find health!"
    elif emyinsig == False and health == False and armor == False and weapon == True:
        print "Going to find health!"
    elif emyinsig == False and health == True and armor == True and weapon == False:
        print "Going to find weapon!"
    elif emyinsig == False and health == False and armor == True and weapon == True:
        print "Going to find health!"
    elif emyinsig == False and health == True and armor == False and weapon == True:
        print "Going to find armor!"
    elif emyinsig == False and health == True and armor == True and weapon == True:
        print "Going to find enemy!"
        moveTowards (you)
        b.face (you)


def walkSquare ():
    b.forward (100, 100)
    b.select (["move"])
    b.left (100, 100)
    b.select (["move"])
    b.back (100, 100)
    b.select (["move"])
    b.right (100, 100)
    b.select (["move"])


def runArc (a):
    b.forward (100, 100)
    b.turn (a, 1)
    b.select (["move"])
    b.select (["turn"])


def circle ():
    while True:
        for a in range (0, 360, 45):
            runArc (a+180)
        time.sleep (5)
        for w in range (0, 10):
            print "attempting to change to weapon", w,
            print "dhewm3 returns", b.changeWeapon (w)
            time.sleep (3)

def testturn (a):
    b.turn (a, 1)
    b.select (["turn"])

def sqr (x):
    return x * x

def calcDist (d0, d1):
    p0 = b.d2pv (d0)
    p1 = b.d2pv (d1)
    s = subVec (p0, p1)
    return math.sqrt (sqr (s[0]) + sqr (s[1]))

def moveTowards (i):
    b.reset ()
    print "will go and find", i
    print "I'm currently at", b.getpos (me), "and", i, "is at", b.getpos (i)
    if not equVec (b.d2pv (b.getpos (me)), [12, 9]):
        print "failed to find getpos at 12, 9 for python"
    if not equVec (b.d2pv (b.getpos (i)), [40, 3]):
        print "failed to find getpos at 40, 3 for player"
    print "bot is at", b.d2pv (b.getpos (me))
    print "you are at", b.d2pv (b.getpos (you))
    d = b.calcnav (i)
    print "object", i, "is", d, "units away"
    if d is None:
        print "cannot reach", i
        b.turn (90, 1)
        b.select (["turn"])
        b.forward (100, 100)
        b.select (["move"])
    else:
        print "distance according to dijkstra is", d
        b.journey (100, d, i)
        print "finished my journey to", i
        print "  result is that I'm currently at", b.getpos (me), "and", i, "is at", b.getpos (i)
        print "      penguin tower coords I'm at", b.d2pv (b.getpos (me)), "and", i, "is at", b.d2pv (b.getpos (i))


def findAll ():
    for i in b.allobj ():
        print "the location of python bot", me, "is", b.getpos (me)
        if i != me:
            b.aim (i)
            moveTowards (i)
            time.sleep (5)

def findYou (b):
    for i in b.allobj ():
        if i != b.me ():
            return i


def antiClock (b):
    print "finished west, north, east, south"
    print "west, north, east, south diagonal"
    for v in [[1, 1], [-1, 1], [-1, -1], [1, -1]]:
        print "turning",
        b.turnface (v, 1)
        b.sync ()
        print "waiting"
        time.sleep (10)
        print "next"
        b.reset ()


def clock (b):
    print "finished west, north, east, south"
    print "west, north, east, south diagonal"
    for v in [[1, 1], [1, -1], [-1, -1], [-1, 1]]:
        print "turning",
        b.turnface (v, -1)
        b.sync ()
        print "waiting"
        time.sleep (10)
        print "next"
        b.reset ()

def face (b):
    b.reset ()
    print "your pen location is", b.d2pv (b.getpos (you))
    print "my pen position is", b.d2pv (b.getpos (me))

    v = subVec (b.d2pv (b.getpos (you)), b.d2pv (b.getpos (me)))
    b.turnface (v)
    b.sync ()
    print "using method 2"
    b.aim (you)
    b.reset ()


#global h, a, w, eIS

b = botlib.bot ("localhost", 'python_doommarine_1')
#b = botbasic.basic ("localhost", 'python_doommarine_1')
print "success!  python doom marine is alive"

print "trying to get my id...",
me = b.me ()
print "yes"
print "the python marine id is", me
you = findYou (b)
a = b.allobj()

print "======================="
for i in a:
    print i
print "======================="

while True:

    moveTowards (you)
    b.face (you)
    #b.fire ()

    #init()

    #tree(healthLow, armorLow, weaponLow, enemyInSight)

    time.sleep (3)
