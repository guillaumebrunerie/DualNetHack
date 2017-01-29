#include "hack.h"
#include "wintcp.h"
#include "dualnethack.h"

#include <arpa/inet.h>
#include <stdarg.h>
#include <errno.h>

static void
debug(const char *format, ...)
{
     va_list args;
     va_start(args, format);

     fprintf(stderr, "%d ", playerid);
     vfprintf(stderr, format, args);

     va_end(args);
}

/* The socket used implicitely by all functions here */
static __thread int sockfd;

void
tcp_set_sockfd(sfd)
int sfd;
{
     sockfd = sfd;
}

int
tcp_get_sockfd()
{
     return sockfd;
}

/* Initialization */

void
tcp_init_connection(sock1, sock2)
int *sock1;
int *sock2;
{
     int serverSock;
     struct sockaddr_in serverAddr;
     struct sockaddr_storage serverStorage;
     socklen_t addr_size;

     /* Create the socket */
     serverSock = socket(PF_INET, SOCK_STREAM, 0);

     if (setsockopt(serverSock, SOL_SOCKET, SO_REUSEADDR, &(int){ 1 }, sizeof(int)) < 0)
          error("setsockopt(SO_REUSEADDR) failed");

     /* Configure server address */
     serverAddr.sin_family = AF_INET;
     serverAddr.sin_port = htons(4242);
     serverAddr.sin_addr.s_addr = INADDR_ANY;

     /* Set all bits of the padding field to 0 */
     memset(serverAddr.sin_zero, '\0', sizeof serverAddr.sin_zero);

     /* Bind the address to the socket */
     bind(serverSock, (struct sockaddr *) &serverAddr, sizeof(serverAddr));

     /* Listen on the socket */
     if(listen(serverSock,2)==0)
          fprintf(stderr, "Listening\n");
     else
          fprintf(stderr, "Error\n");

     /* Accept call creates a new socket for the incoming connection */
     addr_size = sizeof serverStorage;

     *sock1 = accept(serverSock, (struct sockaddr *) &serverStorage, &addr_size);
     fprintf(stderr, "First player connected!\n");

     *sock2 = accept(serverSock, (struct sockaddr *) &serverStorage, &addr_size);
     fprintf(stderr, "Second player connected!\n");
}


/* In order to check which variables have changed */

static int old_ux = -1;
static int old_uy = -1;
static int old_ghost_x = -1;
static int old_ghost_y = -1;

typedef struct {
     const char* name;
     int *actual_var;
} int_vars;

typedef struct {
     const char* name;
     xchar *actual_var;
     xchar *old_var;
} xchar_vars;

/* The references are client-side */
int_vars common_int_vars[] = {
     {"v:WIN_STATUS", &WIN_STATUS, 0},
     {"v:WIN_MESSAGE", &WIN_MESSAGE, 0},
     {"v:WIN_MAP", &WIN_MAP, 0},
     {"v:WIN_INVEN", &WIN_INVEN, 0},
     {"v:rndencode", &context.rndencode, 0},
     {"", NULL}};

xchar_vars common_xchar_vars[] = {
     {"v:u.ux", &u.ux, &old_ux},
     {"v:u.uy", &u.uy, &old_uy},
     {"v:u.ghost_x", &u.ghost_x, &old_ghost_x},
     {"v:u.ghost_y", &u.ghost_y, &old_ghost_y},
     {"", NULL}};

void
tcp_send_changed_variables()
{
     xchar_vars *var;
     fprintf(stderr, "Sending changed variables\n");
     for (var = common_xchar_vars; var->name[0]; var++) {
          if (var->old_var && *(var->actual_var) != *(var->old_var)) {
               fprintf(stderr, "Sending variable %s : %d (was %d)\n", var->name, *(var->actual_var), (var->old_var ? *(var->old_var) : "NULL"));
               tcp_send_string(var->name);
               tcp_send_xchar(*(var->actual_var));
               if (var->old_var)
                    *(var->old_var) = *(var->actual_var);
          }
     }
}

void
tcp_send_WIN()
{
     if (current_player->WIN_STATUS != -1) {
          tcp_send_string("v:WIN_STATUS");
          tcp_send_int(current_player->WIN_STATUS);
     }

     if (current_player->WIN_MESSAGE != -1) {
          tcp_send_string("v:WIN_MESSAGE");
          tcp_send_int(current_player->WIN_MESSAGE);
     }

     if (current_player->WIN_MAP != -1) {
          tcp_send_string("v:WIN_MAP");
          tcp_send_int(current_player->WIN_MAP);
     }

     if (current_player->WIN_INVEN != -1) {
          tcp_send_string("v:WIN_INVEN");
          tcp_send_int(current_player->WIN_INVEN);
     }
}

void
tcp_send_gbuf()
{
     int i, j;
     int rstart = -1;
     int rend = -1;
     int cstart[ROWNO];
     int cend[ROWNO];

     if (you_player != current_player) {
       fprintf(stderr, "!!!!!!!!!!!!!!!\n");
       return;
     }

     for (i = 0; i < ROWNO; i++) {
          cstart[i] = -1;
          cend[i] = -1;
          for (j = 0; j < COLNO; j++) {
               if (gbuf[i][j].glyph != you_player->old_gbuf[i][j]) {
                    if (cstart[i] == -1)
                         cstart[i] = j;
                    cend[i] = j;
               }
          }
          if (cstart[i] == -1) {
               cstart[i] = 1;
               cend[i] = 0;
          } else {
               if (rstart == -1)
                    rstart = i;
               rend = i;
          }
     }
     if (rstart == -1)
          return; /* Nothing changed */
     

     tcp_send_string("v:gbuf");

     tcp_send_char((char) rstart);
     tcp_send_char((char) rend);
     for (i = rstart; i < rend + 1; i++) {
          tcp_send_char((char) cstart[i]);
          tcp_send_char((char) cend[i]);
          for (j = cstart[i]; j < cend[i] + 1; j++) {
               tcp_send_int(gbuf[i][j].glyph);
               you_player->old_gbuf[i][j] = gbuf[i][j].glyph;
          }
     }
}

void
tcp_send_rndencode()
{
     tcp_send_string("v:rndencode");
     tcp_send_int(context.rndencode);
}

void
tcp_recv_update_variable(toupdate)
char *toupdate;
{
     int_vars *ivar;
     xchar_vars *cvar;

     for (ivar = common_int_vars; ivar->name[0]; ivar++) {
          if (!strcmp(toupdate, ivar->name)) {
               *(ivar->actual_var) = tcp_recv_int();
               break;
          }
     }
     
     for (cvar = common_xchar_vars; cvar->name[0]; cvar++) {
          if (!strcmp(toupdate, cvar->name)) {
               *(cvar->actual_var) = tcp_recv_xchar();
               break;
          }
     }
}

void
tcp_transfer_all(from)
int from;
{
     char block[100];
     int n;
     
     while (1) {
          n = recv(from, block, 100, MSG_DONTWAIT);
          if (n == -1) {
               if (errno == EAGAIN || errno == EWOULDBLOCK)
                    break;
               fprintf(stderr, "ERROR\n");
          } else if (n == 0) {
               break;
          } else {
               send(sockfd, block, n, 0);
          }
     }
}

/* Sending functions */

/* Send a given amount of bytes */
static int
tcp_sendall(sockfd, str, len)
int sockfd;
const void *str;
int len;
{
     int n = -1;

     if (len < 0) {
       fprintf(stderr, "Are you trying to send a message of length %d?\n", len);
       return (-1);
     }
     if (len == 0)
       return 0;

     while (len > 0) {
          n = send(sockfd, str, len, 0);
          if (n <= 0) break;
          str += n;
          len -= n;
     }
     if (n < 0)
          fprintf(stderr, "Error sending bytes : %d.\n", errno);
     return (n < 0 ? -1 : 0);
}

void
tcp_send_name_command(str)
const char *str;
{
     tcp_send_string(str);
}

void
tcp_send_string_to(sockfd, str)
int sockfd;
const char *str;
{
     if (str) {
          int len = strlen(str);
          tcp_send_int_to(sockfd, len);
          if (len)
            tcp_sendall(sockfd, str, len);
     } else {
          tcp_send_int(-1);
     }
     debug("/s:%s\n", str);
}

void
tcp_send_string(str)
const char *str;
{
     tcp_send_string_to(sockfd, str);
}

void
tcp_send_int_to(sockfd, n)
int sockfd;
int n;
{
     uint32_t m = htonl(n);
     tcp_sendall(sockfd, &m, sizeof(uint32_t));

     debug("/i:%d\n", n);
}

void
tcp_send_int(n)
int n;
{
     return tcp_send_int_to(sockfd, n);
}

void
tcp_send_boolean(b)
boolean b;
{
     char b_char = (char) b;
     tcp_sendall(sockfd, &b_char, 1);

     debug("/b:%d\n", (int) b);
}

void
tcp_send_xchar(c)
xchar c;
{
     tcp_sendall(sockfd, &c, sizeof(xchar));

     debug("/c:%i\n", (int) c);
}

void
tcp_send_char(c)
char c;
{
     tcp_sendall(sockfd, &c, sizeof(char));

     debug("/c:%i\n", (int) c);
}

void
tcp_send_d_level(dlev)
d_level dlev;
{
     tcp_send_xchar(dlev.dnum);
     tcp_send_xchar(dlev.dlevel);
}

void
tcp_send_void()
{
     tcp_sendall(sockfd,"\0",1);
}

/* Not safe at all */
void
tcp_send_anything(any)
anything any;
{
     uint64_t m = (uint64_t) any.a_void;
     tcp_sendall(sockfd, &m, 8);

     debug("/a:%d\n", m);
}

/* Not safe at all */
void
tcp_send_long(n)
long n;
{
     uint64_t m = (uint64_t) n;
     tcp_sendall(sockfd, &m, 8);

     debug("/l:%ld\n", n);
}

/* Not safe at all */
void
tcp_send_list_menu_item(menu_list, len)
menu_item *menu_list;
int len;
{
     int i;
     debug("count=%d\n", len);
     for (i = 0; i<len; i++) {
          tcp_send_anything(menu_list->item);
          tcp_send_long(menu_list->count); /* Why is that a long, seriously? */
          menu_list++;
     }
}

/* Receiving functions */

static void
tcp_recvall(sockfd, buf, len)
int sockfd;
void *buf;
int len;
{
     int n = -1;

     if (len <= 0) {
          fprintf(stderr, "Are you trying to receive a message of length %d?\n", len);
          return;
     }

     while (len > 0) {
          n = recv(sockfd, buf, len, 0);
          if (n <= 0)
               break;
          buf += n;
          len -= n;
     }
     if (n == 0)
          fprintf(stderr, "EOF\n");
     if (n < 0)
          fprintf(stderr, "%i Error receiving bytes : %d, %d.\n", playerid, errno, sockfd);
}

int
tcp_recv_int_from(sockfd)
int sockfd;
{
     uint32_t m;
     tcp_recvall(sockfd, &m, 4);

     int m2 = (int) ntohl(m);
     debug("i:%d\n", m2);
     return m2;
}

int
tcp_recv_int()
{
     return tcp_recv_int_from(sockfd);
}

/* Returns 1 if the string is null */
int
tcp_recv_string_from(sockfd, buf)
int sockfd;
char *buf;
{
     int len = tcp_recv_int_from(sockfd);
     if (len == 0) {
          debug("s:[empty]\n");
          *buf = '\0';
          return 0;
     } else if (len > 0) {
          tcp_recvall(sockfd, buf, len);
          buf[len] = '\0';
          debug("s:%s\n", buf);
          return 0;
     } else {
          debug("s NULL\n");
          return 1;
     }
}

/* Returns 1 if the string is null */
int
tcp_recv_string(buf)
char *buf;
{
     return tcp_recv_string_from(sockfd, buf);
}


boolean
tcp_recv_boolean()
{
     boolean b;
     tcp_recvall(sockfd, &b, 1);

     debug("b:%d\n", b);
     return b;
}

xchar
tcp_recv_xchar()
{
     xchar c;
     tcp_recvall(sockfd, &c, sizeof(xchar));

     debug("c:%i\n", (int) c);
     return c;
}

char
tcp_recv_char()
{
     char c;
     tcp_recvall(sockfd, &c, sizeof(char));

     debug("c:%i\n", (int) c);
     return c;
}

void
tcp_recv_d_level(dlev)
d_level *dlev;
{
     dlev->dnum = tcp_recv_xchar();
     dlev->dlevel = tcp_recv_xchar();
}

void
tcp_recv_void()
{
     char unused[1];
     tcp_recvall(sockfd, unused, 1);
}

/* Not safe at all */
void
tcp_recv_anything(any)
anything *any;
{
     uint64_t m;
     tcp_recvall(sockfd, &m, 8);
     any->a_void = m;

     debug("a:%d\n", m);
}

/* Not safe at all */
long
tcp_recv_long()
{
     uint64_t m;
     tcp_recvall(sockfd, &m, 8);
     long result = (long) m;

     debug("l:%d\n", m);
     return result;
}

/* Not safe at all */
void
tcp_recv_list_menu_item(menu_list, n)
menu_item **menu_list;
int n;
{
     menu_item *mi;
     debug("Receiving %d items\n", n);
     *menu_list = (menu_item *) alloc(n * sizeof(menu_item));
     for (mi = *menu_list; n; n--, mi++) {
          debug("Receiving item\n");
          tcp_recv_anything(&mi->item);
          mi->count = tcp_recv_long();
     }
}
