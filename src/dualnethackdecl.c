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
    queue_end = (queue_end + QUEUE_SIZE - 1) % QUEUE_SIZE;
    return (queue[queue_end]);
}

int
dualnh_queue_length()
{
    return ((queue_end - queue_start + QUEUE_SIZE) % QUEUE_SIZE);
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

static __thread struct tmp_glyph {
    int savedx;
    int savedy;
    boolean ispresent;
} tg = {0};

void
dualnh_ghost_update()
{
    int x = u.ghost_x;
    int y = u.ghost_y;
    if (!isok(x, y))
        return;
    if (tg.ispresent) { /* not first call, so reset previous pos */
        newsym(tg.savedx, tg.savedy);
        tg.ispresent = 0; /* display is presently up to date */
    }
    if (current_player == you_player)
        return;
    tg.savedx = x;
    tg.savedy = y;
    tg.ispresent = 1;
    show_glyph(x, y, hero_glyph); /* show it */
    flush_screen(0);                 /* make sure it shows up */
}
