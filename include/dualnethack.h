#include <pthread.h>

typedef struct {
    int sockfd;  /* Socket */
    int server_socket;
    pthread_t tid;  /* Thread id */

    winid p_WIN_MESSAGE, p_WIN_STATUS, p_WIN_MAP, p_WIN_INVEN;
    char p_plname[PL_NSIZ];

    struct flag p_flags;
    struct instance_flags p_iflags;

    char p_pl_character[PL_CSIZ];

    struct Role p_urole;
    struct Race p_urace;

    struct q_score p_quest_status;

    struct you p_u;
    struct monst p_youmonst;

    struct obj *p_invent, *p_uarm, *p_uarmc, *p_uarmh, *p_uarms, *p_uarmg, *p_uarmf,
        *p_uarmu, /* under-wear, so to speak */
        *p_uskin, *p_uamul, *p_uleft, *p_uright, *p_ublindf, *p_uwep, *p_uswapwep, *p_uquiver,
        *p_uchain, *p_uball;

    struct spell p_spl_book[MAXSPELL + 1];

    struct context_info p_context;

    int p_multi;
    const char *p_multi_reason;
    int p_nroom;
    int p_nsubroom;
    int p_occtime;
    int NDECL((*p_occupation));
    int p_in_doagain;
    stairway p_dnstair, p_upstair, p_dnladder, p_upladder, p_sstairs;
    dest_area p_updest, p_dndest;

    int old_gbuf[ROWNO][COLNO];
    gbuf_entry p_gbuf[ROWNO][COLNO];
    char p_gbuf_start[ROWNO];
    char p_gbuf_stop[ROWNO];

    /* The current level
     * When the two players are on the same level, they should be kept in sync.
     */
    schar p_lastseentyp[COLNO][ROWNO];
    dlevel_t *p_level;
    dlevel_t p_level_actual;
    rm_sub p_locations_sub[2][COLNO][ROWNO];

    boolean finished_turn;
} player;

#define DUMMY_PLAYER                         \
    {.sockfd = -1,                           \
     .server_socket = -1,                    \
     .tid = NULL,                            \
     .p_WIN_MESSAGE = WIN_ERR,               \
     .p_WIN_STATUS = WIN_ERR,                \
     .p_WIN_MAP = WIN_ERR,                   \
     .p_WIN_INVEN = WIN_ERR,                 \
     .p_plname = DUMMY,                      \
     .p_flags = DUMMY,                       \
     .p_iflags = DUMMY,                      \
     .p_pl_character = DUMMY,                \
     .p_urole = DUMMY_ROLE,                  \
     .p_urace = DUMMY_RACE,                  \
     .p_quest_status = DUMMY,                \
     .p_u = DUMMY,                           \
     .p_youmonst = DUMMY,                    \
     .p_invent = (struct obj *) 0,           \
     .p_uarm = (struct obj *) 0,             \
     .p_uarmc = (struct obj *) 0,            \
     .p_uarmh = (struct obj *) 0,            \
     .p_uarms = (struct obj *) 0,            \
     .p_uarmg = (struct obj *) 0,            \
     .p_uarmf = (struct obj *) 0,            \
     .p_uarmu = (struct obj *) 0,            \
     .p_uskin = (struct obj *) 0,            \
     .p_uamul = (struct obj *) 0,            \
     .p_uleft = (struct obj *) 0,            \
     .p_uright = (struct obj *) 0,           \
     .p_ublindf = (struct obj *) 0,          \
     .p_uwep = (struct obj *) 0,             \
     .p_uswapwep = (struct obj *) 0,         \
     .p_uquiver = (struct obj *) 0,          \
     .p_uchain = (struct obj *) 0,           \
     .p_uball = (struct obj *) 0             \
     .p_spl_book = { DUMMY },                \
     .p_context = DUMMY,                     \
     .p_multi = DUMMY,                       \
     .p_multi_reason = NULL,                 \
     .p_nroom = 0,                           \
     .p_nsubroom = 0,                        \
     .p_occtime = 0,                         \
     /*.p_occupation */                      \
     .p_in_doagain = 0,                      \
     .p_dnstair = { 0, 0, { 0, 0 }, 0 },     \
     .p_upstair = { 0, 0, { 0, 0 }, 0 },     \
     .p_dnladder = { 0, 0, { 0, 0 }, 0 },    \
     .p_upladder = { 0, 0, { 0, 0 }, 0 },    \
     .p_sstairs = { 0, 0, { 0, 0 }, 0 },     \
     .p_updest = { 0, 0, 0, 0, 0, 0, 0, 0 }, \
     .p_dndest = { 0, 0, 0, 0, 0, 0, 0, 0 }, \
     .old_gbuf = DUMMY,                      \
     .p_gbuf = DUMMY,                        \
     .p_gbuf_start = DUMMY,                  \
     .p_gbuf_stop = DUMMY,                   \
     .p_lastseentyp = DUMMY,                 \
     .p_level = NULL,                        \
     .p_level_actual = DUMMY,                \
     .p_locations_sub = DUMMY,               \
     .finished_turn = 0                      \
   }

boolean newsym_table[COLNO][ROWNO];

extern player player1;
extern player player2;
extern __thread player* you_player;
extern __thread player* other_player;
extern player* current_player;

extern __thread int playerid;


extern pthread_barrier_t barrier;

void dualnh_init_players();

void dualnh_p1_wait();
void dualnh_p2_wait();
void dualnh_wait();

boolean dualnh_is_empty();
void dualnh_push(int);
int dualnh_pop();
char* dualnh_queue_str();
char* dualnh_queue_tosend();
void dualnh_zero_queue();

void dualnh_process_and_queue(char *);
void dualnh_ghost_update();
