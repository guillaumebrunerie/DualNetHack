#include <stdio.h>

#include "hack.h"
#include "dualnethack.h"

player player1 = DUMMY;
player player2 = DUMMY;

/* Pointers to either [player1] or [player2] */
__thread player* you_player = NULL;
__thread player* other_player = NULL;
player* current_player = NULL;

int __thread playerid = 0;

pthread_barrier_t barrier;

void
dualnh_save_WIN()
{
  you_player->WIN_MESSAGE = WIN_MESSAGE;
  you_player->WIN_STATUS = WIN_STATUS;
  you_player->WIN_MAP = WIN_MAP;
  you_player->WIN_INVEN = WIN_INVEN;
}

void
dualnh_save_stairs()
{
     player1.upstair = upstair;
     player1.dnstair = dnstair;
     player1.dnladder = dnladder;
     player1.upladder = upladder;
     player1.sstairs = sstairs;

     player2.upstair = upstair;
     player2.dnstair = dnstair;
     player2.dnladder = dnladder;
     player2.upladder = upladder;
     player2.sstairs = sstairs;
}

void
dualnh_save_player(p)
player *p;
{
     int i, j;
     
     p->flags = flags;
     p->iflags = iflags;
     strncpy(p->pl_character, pl_character, PL_NSIZ);
     p->urole = urole;
     p->urace = urace;
     p->quest_status = quest_status;
     p->u = u;
     p->youmonst = youmonst;
     p->invent = invent;
     p->uarm = uarm;
     p->uarmc = uarmc;
     p->uarmh = uarmh;
     p->uarms = uarms;
     p->uarmg = uarmg;
     p->uarmf = uarmf;
     p->uarmu = uarmu;
     p->uskin = uskin;
     p->uamul = uamul;
     p->uleft = uleft;
     p->uright = uright;
     p->ublindf = ublindf;
     p->uwep = uwep;
     p->uswapwep = uswapwep;
     p->uquiver = uquiver;
     p->uchain = uchain;
     p->uball = uball;
     memcpy(p->spl_book, spl_book, sizeof p->spl_book);
     p->context = context;
     p->multi = multi;
     p->multi_reason = multi_reason;
     p->nroom = nroom;
     p->nsubroom = nsubroom;
     p->occtime = occtime;
     p->occupation = occupation;
     p->in_doagain = in_doagain;
     p->dnstair = dnstair;
     p->upstair = upstair;
     p->dnladder = dnladder;
     p->upladder = upladder;
     p->sstairs = sstairs;
     p->updest = updest;
     p->dndest = dndest;

     memcpy(p->gbuf, gbuf, sizeof p->gbuf);
     memcpy(p->gbuf_start, gbuf_start, sizeof p->gbuf_start);
     memcpy(p->gbuf_stop, gbuf_stop, sizeof p->gbuf_stop);

     for (i=0; i<COLNO; i++) {
          for (j=0; j<ROWNO; j++) {
               p->locations[i][j].glyph = levl[i][j].glyph;
               p->locations[i][j].waslit = levl[i][j].waslit;
          }
     }
}

void
dualnh_load_player(p)
player p;
{
     int i, j;
     
     WIN_MESSAGE = p.WIN_MESSAGE;
     WIN_STATUS = p.WIN_STATUS;
     WIN_MAP = p.WIN_MAP;
     WIN_INVEN = p.WIN_INVEN;
     strncpy(plname, p.plname, PL_NSIZ);
     
     flags = p.flags;
     iflags = p.iflags;
     strncpy(pl_character, p.pl_character, PL_CSIZ);
     urole = p.urole;
     urace = p.urace;
     quest_status = p.quest_status;
     u = p.u;
     youmonst = p.youmonst;
     invent = p.invent;
     uarm = p.uarm;
     uarmc = p.uarmc;
     uarmh = p.uarmh;
     uarms = p.uarms;
     uarmg = p.uarmg;
     uarmf = p.uarmf;
     uarmu = p.uarmu;
     uskin = p.uskin;
     uamul = p.uamul;
     uleft = p.uleft;
     uright = p.uright;
     ublindf = p.ublindf;
     uwep = p.uwep;
     uswapwep = p.uswapwep;
     uquiver = p.uquiver;
     uchain = p.uchain;
     uball = p.uball;
     memcpy(spl_book, p.spl_book, sizeof p.spl_book);
     context = p.context;
     multi = p.multi;
     multi_reason = p.multi_reason;
     nroom = p.nroom;
     nsubroom = p.nsubroom;
     occtime = p.occtime;
     occupation = p.occupation;
     in_doagain = p.in_doagain;
     dnstair = p.dnstair;
     upstair = p.upstair;
     dnladder = p.dnladder;
     upladder = p.upladder;
     sstairs = p.sstairs;
     updest = p.updest;
     dndest = p.dndest;
     memcpy(gbuf, p.gbuf, sizeof p.gbuf);
     memcpy(gbuf_start, p.gbuf_start, sizeof p.gbuf_start);
     memcpy(gbuf_stop, p.gbuf_stop, sizeof p.gbuf_stop);

     for (i=0; i<COLNO; i++) {
          for (j=0; j<ROWNO; j++) {
               levl[i][j].glyph = p.locations[i][j].glyph;
               levl[i][j].waslit = p.locations[i][j].waslit;
          }
     }
}

void
dualnh_switch_to_myself()
{
     dualnh_save_player(other_player);
     dualnh_load_player(*you_player);
}

void
dualnh_p1_wait()
{
     if (playerid == 1)
          pthread_barrier_wait(&barrier);
}

void
dualnh_p2_wait()
{
     if (playerid == 2)
          pthread_barrier_wait(&barrier);
}

void
dualnh_wait()
{
     pthread_barrier_wait(&barrier);
}
     
