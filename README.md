# DualNetHack

This document explains the various features a two-player version of NetHack would have, and a
prototype implementation.

## Implemented features

Only the client/server architecture for one player is implemented so far.

## Compilation and usage

Only Linux is supported with the tty interface. A config files is given, other options might or
might not work (in particular different symbol sets). You need to have `make`, `gcc`, `bison`,
`flex`, `gdb` (no idea why) and `libncurses5-dev`. DualNetHack is compiled as follows:

    cd sys/unix
    sh setup.sh hints/linux
    cd ../..
    make all
    make install
    
By default, it will install the client and the server at
`~/dualnh/install/games/dualnethack-{server,client}`. The IP of the server is current hardcoded in
`sys/unix/client.c` and the port number (also hardcoded) is `4242`.

## Python prototype

Another prototype in Python is present in `prototype`, it describes how the movement system could
look like. Note that it might not be in sync with what is described below.

Usage:
- run the server `./dualnethack-server.py`,
- run two clients `./dualnethack-client.py <IP>` where `<IP>` is the IP address of the server, on two
  different computers,
- play simultaneously on both.


## Goal

The goal is to create a two-player cooperative version of NetHack. The idea is that the two players
have been sent as a team by their respective gods to retrieve the Amulet of Yendor and offer it to
them. Here are a few general guidelines governing most of the design decisions:

* the game should look like vanilla NetHack as much as possible, in particular
  * it should stay turn-based
  * the dungeon would contain basically the same items and monsters
  * it should not be possible to exploit the fact that there are two players to avoid the
    difficulties of NetHack
* consistency is very important to me: the features should all be justified first by common sense,
  not by playability. The game should obviously also be made as playable as possible (see section
  **Interface**), but not at the expense of logic.

The first point has various consequences:

* if the two players stay together, they can more easily kill the monsters as they can both hit them
  at the same time
* finding appropriate armor/weapons is potentially more complicated, as there will still be the same
  amount of items randomly (or non-randomly) generated
* in particular, getting reflection and magic resistance for both players will be tricky (there is
  still only one wand of wishing in the castle). You can’t decide that one player will have them and
  do the fighting while the other follows behind, as wands/spells/breath attacks can miss the first
  player and hit the second one
* nutrition will also be potentially much more problematic as there are two mouths to feed but still
  the same amount of food rations/monsters to kill
* both players need to reach lvl 14 to do the quests, and there is still the same number of monsters
  to kill, wraiths to eat and potions of gain level to quaff
  
It seems like some parts will be easier while other will be more complicated, so it looks
interesting.

## Game features

Here is a list (to be expanded…) of what needs to be adapted for two-players and how, in no
particular order.  The legend is as follows: :+1: means that it seems to be the best solution,
:question: means that it’s not yet clear what is the best solution

### Role :question:

Should the two players be allowed to be of the same role? A priori yes, except that they will have
to do both the same quest and that they may actually have the same god, which might complicate
things (each role has three different gods which are different from those of the other roles).

### Death :question:

If one of the two players dies, then his god will smite the other player to death, no way to survive
this. Hey, it’s a cooperative game. But should the other player have a chance of zapping a wand of
turn undead? Maybe not too much or with huge drawbacks as it would make it way too easy.

### Religion :+1:

Except for the very end of the game, the two gods are independent of each other. That means that one
can be angry at his player and not the other, in order to make offering/holy water you need to be at
the correct altar. If one player is lawful and the other chaotic, and the altar is chaotic, only the
chaotic can make offerings or holy water. If the lawful one tries, he will try to convert the altar
instead, which might make his companion’s god more angry (!). And I believe that we can only ever
convert a particular altar once (to check in vanilla NetHack with a helm of opposite alignment).

### Artifacts :+1:

No change about artifacts, in particular if both player want one, well, you need two, so it will be
more complicated (and they cannot both have the same).

### Time :question:

How to stay turn-based? Here is only a theoretical discussion of what being turn-based for a
2-player NetHack should mean. There will be an interface allowing you to prepare moves while you
wait for the other player to play, but this is discussed in the section **Interface**.

My initial idea was that at each turn, both player play *simultaneously*, then all monsters, then
both players simultaneously, then all monsters, and so on. This is problematic for various reasons,
for instance what should happen if both players move towards the center in the following situation?

    .....
    .@.@.
    .....

They could both stay in place, but it’s not very logical, or one of them could be chosen to be the
first one, but it would be nice to know which one. Another example is the following:

    .....
    .@@d.
    .....
    
Assume the player in the center goes down while the player on the left fires an arrow at the
jackal. Should the player in the center be hit? One feature of NetHack is that when it’s your turn,
you know that it’s really your turn, noone else will play before you. How to preserve this property
when there are *two* players?


Another possibility is to assume that at each turn, one of the two players play first and when he
presses a key, that move is actually done. In that way, you would always know whether it’s your turn
to play or the turn of the other player and there would be no surprise as above. When the player in
the middle goes down, either he actually goes in which case there is no problem, or he’s in a
waiting state, in which case it’s clear that he’s supposed to get hit.

The issue with that is: who plays first?
* If it’s always the same player, the game would probably be a bit unbalanced (as the monsters will
  always play after the same player)
* If it’s random, it will probably be very weird
* If it alternates, it might work but it would probably still be weird

Ideally, I guess that when there are no monsters it should always alternate, but when there are
monsters it’s not clear. Maybe one player plays at the beginning of a turn and then the second
player at the middle (after half the monsters). That might require reworking the speed system (but
it’s pretty strange already anyway). And what if there is exactly one monster? Will it play always
in the same half of the turn or will it be random, or alternate? Probably random is better. If the
players are `P` and `Q` and the monster is `M`, here is what a few turns would look like (in random
mode).

    PMQPQMPQMPMQPQMPQMPQMPQMPQMPMQPQM

Sometimes the players play three times between two attacks of the monsters, sometimes only once (in
which case it’s always `P` :-/ ).

We should also take a look at what actually happens in the source code (typically when there is
conflict), but I expect that the monsters always play in the same order.

Here is a (probably) mathematically ideal solution: for each turn the order of playing of every
monster and player is randomized, except for the fact that one player plays always first (as the
monsters can play before, after, or in between, we won’t really see the difference). We should test
it to see if that makes sense. Example with one monster:

    PMQ PQM PMQ MPQ MPQ MPQ PMQ MPQ PMQ PQM

Same one with a pause after each player,

    P MQ P Q MP MQ MP Q MP Q MP Q P MQ MP Q P MQ P Q M


Another point to consider is that if the two players are next to each other, should they be able to
swap positions in one turn? That cannot be made to work without tweaking if we assume they play one
after the other, or if there are monsters playing in the mean time. Maybe it should be treated
specially. In particular, this might be very helpful in corridors or mazes when fighting enemies,
but I don’t know if it actually makes sense logically. We should maybe take inspiration on what
happens with pets.


After thinking about it some more, what I described above isn’t good either, because when one of the
two players is fighting a monster the pattern will be irregular. Here is another attempt: Every
monster is intrinsically either "odd" or "even" (determined at creation time). Then the turn unrolls
in the following way: first all odd monsters, then the first player, then all even monsters, then
the second player. When there are no monsters the turns alternate, and when a player is fighting
with a monster, their turns alternate as well. When both players are fighting a monster, the turns
alternate in a predictable way (i.e. asymmetrically), but it is random depending on the
monster. When both players are fighting two monsters, it could be either M1 P1 M2 P2 or M1 M2 P1 P2
or P1 M1 M2 P2, which adds some diversity in the fights. Maybe there is some asymmetry that I
haven’t seen, but so far it seems good to me. I still haven’t really thought about how to factor
speed into that.

About swapping, here is a proposition: when it’s your turn and you attempt to move where the other
player is, it gives a message "You attempt to swap places with <your companion>". Then, if the other
player also attempts to swap places with you, it succeeds. If the other player stays in place or
even moves away, then the first player stays in place (even if the spot is now free) and get the
message "You bump into <your companion>". This is similar to what happens when bumping into
pets. Also, it would make fleeing into corridors nice: suppose we are in the following situation and
the players want to go left:

    ------
    ..@@..
    ------
    
If it’s the turn of the first player, no problem. If it the turn of the second player, then he first
tries to swap places with the first player, but the first player goes left and the second players
bumps into them, then the second player can go left again and we are in the first situation
(i.e. there will not be an infinite sequence of bumps, only one at the beginning). Of course it
would do the same thing if the second player just wait their turn.

### Conflict :question:

When generating conflict or wielding Stormbringer, attempting to swap places with the other player
will attack them instead (be careful if you wield Mjollnir or Vorper Blade!). Maybe we should still
be able to swap places using the "m" prefix.

### Vision :+1:

Should both player always see the other player and see what the other player is seeing?

I don’t believe so for various reasons:
* it doesn’t make sense
* it completely negates the drawbacks of blindness
* what if they are at different dungeon levels?
* one player could be permanently blind with telepathy and the other not blind for the best of both
  worlds!
  
So I think everyone should see only what he’s supposed to see. For instance player A would see
player B only if either
* player B is in the field of vision of player A, and it is lit,
* player B is in the field of vision of player A, it is dark, but player A has infravision,
* player B is in the field of vision of player A, and player B carries a lamp,
* … unless player B is invisible,
* … unless player A can see invisible,
* player A has telepathy,
* I probably forgot some cases, but you get the idea.

### Dungeon level :+1:

The two players can be at different dungeon levels. I first thought about forbidding it (by
requiring both to be at or next the stairs in order to go up or down), but then I cannot see how to
make trap doors or level teleport traps works. Either they apply to both players as soon as one
falls in, but that is not very logical especially for trap doors, or they work only when both
players are together, but it’s again not very logical and basically negates the threat of trap
doors, or we have to allow the two players to be on different dungeon levels.

This might actually be nice given that if they are used to fight together, it could make it much
harder to have to fight the monsters alone while they are separated, so there would be a strong
incentive to find each other again.

When they are on different dungeon levels, the game *stays* turn based. The idea that the game is
turn-based when the players and next to each other and not when they are not does not make
sense. Otherwise, one player who is hungry could stay idle for as long as they want while the other
one goes to a different level to get him food. Similarly for everything else that depends on the
turn counter. It just wouldn’t make sense that the turn counter works differently depending on
whether the players are on the same level or not.

### Monster generation :+1:

Monster generation depends on dungeon level and player level. In our case, I think it should depend
on dungeon level and the higher of the two player levels (to give an incentive to both to level up
and to compensate with the fact that they can both attack monsters together). Otherwise it would
incite the players to have different levels so that the monsters would be easy for the high-level
one.

### Discoveries :question:

The two players should have independent discovery list. For instance if one of the two figures out
that the brilliant blue potion is gain level, the other one won’t automatically know it. But if the
first player drinks it in front of the second one (and the second one is not blind), then the second
one will get the message "Your companion drinks a brilliant blue potion. Your companion seems more
experienced", and it will automatically identify it as is already the case when monsters do
it. Similarly for scrolls (the other player should not be deaf either), rings, amulets, and so on.

Problem: it kind of negates the threat posed by (master) mind flayers, but I can’t find a logical
explanation for having common discoveries, or for mind flayers acting on both players at the same
time. What if the other player is three levels away? But if you get both attacked by mind flayers,
then you could actually forget things, so beware.

Maybe there should be a way to share discoveries? (for instance so that the other player can write a
scroll that only you know) But I haven’t found a good way to do that. Maybe each time they are next
to each other (and are not sleeping/deaf/etc.), all discoveries are automatically shared. But then
it would really completely negate the threat of mind flayers, so maybe not.

### Luck :question:

Luck should be independent for every player (but note that there is still only one luckstone in
Mines End). Does that mean that only the player with the luckstone should do the luck-critical
actions (e.g. wishing)? Maybe sometimes the luck taken into account should be the average of the two
(or the max, or the min, or …)

### Tame, peaceful, hostile monsters :question:

If one player is a gnome and the other is a human, who should be peaceful in the Gnomish Mines?

First question to ask is whether monsters can/should be able to be hostile to one player while being
peaceful to the other. It kind of makes sense to say that monsters recognize both players has a
team, so they would have the same behaviour towards both, but what if the players aren’t at the same
dungeon level? If the gnome enters the Gnomish Mines alone, should the gnomes somehow know that he
has a human friend and be hostile? That might make sense, after all orc and elfs are always hostile
to each other when one of them is the player, but a monster orc and a monster elf are not hostile to
each other (by the way, why not?). So maybe a monster is hostile to both iff it would hostile to any
of the two in a regular game.

What about unicorns? Should they all be hostile if the two players are of different alignment? Maybe
not, that would be too harsh. Maybe as an exception a unicorn would be peaceful to both iff it is of
the same alignment as one of the players. But throwing gems to increase luck only works for the
player of the right alignment (or for both if they both have the same alignement). And nothing
change when offering unicorns at an altar.

Tame monsters should be tame for both or for none, even if they are tamed by one player and fall
through a trap door to the level where the second player is (for instance). They are smart enough to
recognize the second player as being a friend of the first one. Otherwise it would be crazy if the
pet of one player starts attacking the other player.

Moreover, tameness should probably decrease by half the usual rate when they are on a level with
only one of the two players. In particular pets can become untame even next to you, if the other
player is far away for a long time (need to check the actual algorithm, to make something that makes
sense).

### Pets at the beginning of the game :+1:

The players should probably get only one pet for two at the beginning, or it would be chaos. Note
that some roles are guaranteed to have a cat and some are guaranteed to have a dog. To solve this,
for instance choose randomly one of the two players and give him the pet he would usually have. If
one of the players is a knight, should he always have a pony? That would not be very fair to the
other player and there is still a problem if both players are knights (if allowed). Better idea:
knights don’t always get their pony, but if a knight does not get a pony he gets a saddle instead,
so that he can tame a random horse in the dungeon if he wants to.

### Engulfing monsters :+1:

What happens when one of the players gets engulfed? Presumably, engulfing monsters cannot engulf
both players at the same time and then the other player can keep on hitting on the engulfing monster
from outside. Unless both get engulfed by different monsters, of course, for instance on the Plane
of Air. Hitting the engulfing monster should probably not harm the player who is inside.

### Vaults :question:

I guess the guard will have to check that both players are following (in the case both players were
inside).

### The Quest :question:

There should be two quests, one for each player. Each quest contains a different Bell of Opening
(maybe give a different name to one of them) and they are both needed for the invocation, so they
both need to be done. In order to access any of the two quests, the corresponding player needs to
have the usual requirements and go chat to his quest leader, it will unlock the quest for both
players (so that both can do both quests together).

Other possibilities and reasons for excluding them:
* only one quest based randomly on one of the two players: wouldn’t feel very fair to the other one
* each player can only enter their own quest: during this time, the game will not be very
  cooperative, so it doesn’t look like a very good solution. On the other hand it’s very logical,
  but the version above can be justified by saying that if you have a pet, it doesn’t need to be
  approved by your quest leader, only you need (the other player will be seen as a “pet” to the
  quest leader)
* each player can only enter their own quest and they have to do the quest simultaneously: some
  quests are much harder than others
* one quest twice as long with levels from each of the two quest interleaved: if there is only one
  Bell of Opening, it’s hard to justify killing both quest nemeses, if there are two it makes more
  sense to have two quests. And why have two quest leaders in the same branch? And which one is
  first (it is important because it means we can access the second one only when the first player
  has level 14) Nice idea, but not very logical.

### Covetous monsters :question:

Various monsters (like Quest nemeses) are covetous: they attack you and then teleport to the
upstairs to heal, and escape if you try to hit them there. With two players, it becomes a bit
trivial as one of the two players can just stay on the upstairs while the other beats the monster to
death (the other player preventing the nemesis to teleport to the upstairs). I’m not sure how to
solve that and whether to solve it at all. After all, it means that one player will have to fight
the Quest nemesis alone, so this technique does have drawbacks.

### Gehennom :question:

Maybe entering or leaving Gehennom should always be done together. Maybe all branches can only be
accessed together? I’m not yet convinced one way or another. 

### The invocation ritual :question:

During the invocation, both player need to ring their own Bell of Opening while standing on the
vibrating square (it cannot be the same player for both bells, and both should be rung on the
vibrating square), and then one of them has to read the Book of the Dead on the vibrating
square.

What if the other player has run away in the mean time? (e.g. teleportitis) Maybe the Book of the
Dead should only work when both players are standing next to each other and then create the fire
traps and moat around both, but I don’t see a good reason for why this would be required. Maybe it’s
ok if one of the two players end up inside a fire trap of a moat.

### The Planes :+1:

I think that going from Dlvl1 to the Plane of Earth, and then from one Plane to the next one should
always be done together. I.e. if one of the two players tries to take the stairs/magic portal
without the other one, it should give a message like "You feel like you cannot do that alone".

### The Amulet :+1:

The bad effects of carrying the Amulet of Yendor should apply to both players as soon as one of them
has the amulet. The mysterious force acts independently on both, which means they risk becoming
separated often, even if they try hard to stay together. The Wizard wants the Amulet, so he will
appears at the level where the player carrying the amulet is.

### Offering the Amulet :question:

Offering the Amulet can be done at either high altar matching the alignment of one of the players,
by the correct player, but with the other player immediately next to him. In particular, if both
players have the same alignment, they need to find the correct high altar and any one can offer the
amulet. If they do not have the same alignment, then they can use any of the two corresponding high
altars, but they have to remember to give the amulet to the player having the same alignment as the
altar. I’m not sure if that’s logical, after all if the gods decided to send a team, it should not
matter who offers the Amulet.

## Interface

Not yet written
