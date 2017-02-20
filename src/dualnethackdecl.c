#include <stdio.h>
#include <ctype.h>

#include "hack.h"

int __thread playerid = 0;
boolean newsym_table[COLNO][ROWNO] = DUMMY;

pthread_barrier_t barrier;

void
dualnh_init_players()
{
    player1.p_level = &player1.p_level_actual;
    player2.p_level = &player1.p_level_actual;
    clear_levl_s(player1.p_locations_sub[0]);
    clear_levl_s(player1.p_locations_sub[1]);
    clear_levl_s(player2.p_locations_sub[0]);
    clear_levl_s(player2.p_locations_sub[1]);
}

/* To get rid of */
void
dualnh_save_stairs()
{
     player2.p_upstair = player1.p_upstair;
     player2.p_dnstair = player1.p_dnstair;
     player2.p_dnladder = player1.p_dnladder;
     player2.p_upladder = player1.p_upladder;
     player2.p_sstairs = player1.p_sstairs;
}

void
dualnh_save_glyphmap()
{
     int i, j;
     /* memcpy(player2.locations, player1.locations, sizeof player1.locations); */

     /* for (i=0; i<COLNO; i++) { */
     /*      for (j=0; j<ROWNO; j++) { */
     /*           levl[i][j].glyph = player2.locations[i][j].glyph; */
     /*           levl[i][j].waslit = player2.locations[i][j].waslit; */
     /*      } */
     /* } */
}

void
dualnh_save_player(p)
player *p;
{
     int i, j;
     
     /* for (i=0; i<COLNO; i++) { */
     /*      for (j=0; j<ROWNO; j++) { */
     /*           p->locations[i][j].glyph = levl[i][j].glyph; */
     /*           p->locations[i][j].waslit = levl[i][j].waslit; */
     /*      } */
     /* } */
}

void
dualnh_load_player(p)
player p;
{
     int i, j;
     
     /* for (i=0; i<COLNO; i++) { */
     /*      for (j=0; j<ROWNO; j++) { */
     /*           levl[i][j].glyph = p.locations[i][j].glyph; */
     /*           levl[i][j].waslit = p.locations[i][j].waslit; */
     /*      } */
     /* } */
}

void
dualnh_copy_level()
{
    /* memcpy(&(you_player->p_level_actual), &(other_player->p_level_actual), sizeof &(you_player->p_level_actual)); */
}

void
dualnh_switch_to_myself()
{
     /* if (loaded_player == you_player) */
     /*      return; */
     /* dualnh_save_player(other_player); */
     /* dualnh_load_player(*you_player); */
     /* loaded_player = you_player; */
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

#define QUEUE_SIZE 100
int __thread queue[QUEUE_SIZE] = DUMMY;
int __thread queue_start = 0;
int __thread queue_end = 0;

/* Should be CO - 1, but CO is not defined
 * It comes from the window system, so we would need the client to send it.
 */
char __thread queue_str[80] = DUMMY;

boolean
dualnh_is_empty()
{
     return (queue_start == queue_end);
}

void
dualnh_push(cmd)
int cmd;
{
    if ((queue_end + 1) % QUEUE_SIZE == queue_start)
        return;  /* Overflow */

    queue[queue_end] = cmd;
    queue_end = (queue_end + 1) % QUEUE_SIZE;
}

int
dualnh_pop()
{
    int cmd;
    if (queue_start == queue_end)
        return -1;  /* Empty */
    cmd = queue[queue_start];
    queue_start = (queue_start + 1) % QUEUE_SIZE;
    return cmd;
}

char*
dualnh_queue_str()
{
     int i, j;
     int tmp;
     for (i = queue_start, j = 0; i != queue_end; (i++) % QUEUE_SIZE, j++) {
         tmp = queue[i];
         if (tmp <= 26 && tmp >= 1) {
             queue_str[j] = '^';
             j++;
             tmp = tmp + 'A' - 1;
         }
         queue_str[j] = tmp;
     }
     for (; j < 79; j++)
          queue_str[j] = ' ';
     queue_str[79] = '\0';
     return queue_str;
}

char __thread queue_ints[QUEUE_SIZE + 1] = DUMMY;

char*
dualnh_queue_tosend()
{
     int i, j;
     for (i = queue_start, j = 0; i != queue_end; (i++) % QUEUE_SIZE, j++) {
          queue_ints[j] = queue[i];
     }
     queue_ints[j] = '\0';
     return queue_ints;
}

void
dualnh_zero_queue()
{
     queue_start = 0;
     queue_end = 0;

     /* This should not be here */
     u.dist_from_mv_queue = 0;
     u.ghost_x = u.ux;
     u.ghost_y = u.uy;
}

int
dualnh_pop_from_end()
{
    if (queue_start == queue_end)
        return -1; /* Empty */
    queue_end = (queue_end - 1) % QUEUE_SIZE;
    return (queue[queue_end]);
}

int
dualnh_queue_length()
{
    if (queue_end >= queue_start)
        return (queue_end - queue_start);
    else
        return (queue_end - queue_start + QUEUE_SIZE);
}

void
dualnh_process_and_queue(cmd)
int cmd;
{
    int lcmd = tolower(cmd);
    boolean movement = FALSE;
    int dir = 0;

    if (cmd == 27) { /* ESC */
        dualnh_zero_queue();
        return;
    } else if (cmd == 127) { /* DEL */
        lcmd = dualnh_pop_from_end();
        dir = -1;
    } else if (cmd) {
        dualnh_push(cmd);
        dir = 1;
    }

    if (u.dist_from_mv_queue > 0) {
        u.dist_from_mv_queue += dir;
        return;
    }

    if (lcmd == 'h' || lcmd == 'y' || lcmd == 'b') {
        u.ghost_x -= dir;
        movement = TRUE;
    }
    if (lcmd == 'j' || lcmd == 'b' || lcmd == 'n') {
        u.ghost_y += dir;
        movement = TRUE;
    }
    if (lcmd == 'k' || lcmd == 'y' || lcmd == 'u') {
        u.ghost_y -= dir;
        movement = TRUE;
    }
    if (lcmd == 'l' || lcmd == 'u' || lcmd == 'n') {
        u.ghost_x += dir;
        movement = TRUE;
    }
    if (cmd == '.' || cmd == 's')
        movement = TRUE;

    if (dir == 1 && !movement)
        u.dist_from_mv_queue = 1;
}
