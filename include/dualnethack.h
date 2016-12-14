#include <pthread.h>

#include "hack.h"

typedef struct {
  int sockfd;  /* Socket */
  pthread_t tid;  /* Thread id */

  winid WIN_MESSAGE, WIN_STATUS, WIN_MAP, WIN_INVEN;
  char plname[PL_NSIZ];
  int old_gbuf[ROWNO][COLNO];

  struct flag flags;
  struct instance_flags iflags;

  char pl_character[PL_CSIZ];

  struct Role urole;
  struct Race urace;

  struct q_score quest_status;

  struct you u;
  struct monst youmonst;
  
  struct obj *invent, *uarm, *uarmc, *uarmh, *uarms, *uarmg, *uarmf,
    *uarmu, /* under-wear, so to speak */
    *uskin, *uamul, *uleft, *uright, *ublindf, *uwep, *uswapwep, *uquiver,
    *uchain, *uball;

  struct spell spl_book[MAXSPELL + 1];

  struct context_info context;

  int multi;
  const char *multi_reason;
  int nroom;
  int nsubroom;
  int occtime;
  int NDECL((*occupation));
  int in_doagain;
  stairway dnstair, upstair, dnladder, upladder, sstairs;
  dest_area updest, dndest;

  gbuf_entry gbuf[ROWNO][COLNO];
  char gbuf_start[ROWNO];
  char gbuf_stop[ROWNO];

  /* We only use the [glyph] and [waslit] fields */
  struct rm locations[COLNO][ROWNO];
  
  boolean finished_turn;
} player;

extern player player1;
extern player player2;
extern __thread player* you_player;
extern __thread player* other_player;
extern player* current_player;

extern __thread int playerid;


extern pthread_barrier_t barrier;

void dualnh_save_WIN();
void dualnh_save_stairs();

void dualnh_switch_to_myself();

void dualnh_p1_wait();
void dualnh_p2_wait();
void dualnh_wait();
