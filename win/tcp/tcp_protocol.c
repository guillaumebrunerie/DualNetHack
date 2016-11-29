#include "hack.h"
#include "wintcp.h"

#include <arpa/inet.h>
#include <stdarg.h>

static void
debug(const char *format, ...)
{
     va_list args;
     va_start(args, format);

     vfprintf(stderr, format, args);

     va_end(args);
}
/* The socket used implicitely by all functions here */

static int sockfd;

void
tcp_set_sockfd(sfd)
int sfd;
{
     sockfd = sfd;
}

/* Initialization */

void
tcp_init_connection()
{
     int sock;
     char buffer[1024];
     struct sockaddr_in serverAddr;
     struct sockaddr_storage serverStorage;
     socklen_t addr_size;

     /* Create the socket */
     sock = socket(PF_INET, SOCK_STREAM, 0);
  
     /* Configure server address */
     serverAddr.sin_family = AF_INET;
     serverAddr.sin_port = htons(4242);
     serverAddr.sin_addr.s_addr = INADDR_ANY;

     /* Set all bits of the padding field to 0 */
     memset(serverAddr.sin_zero, '\0', sizeof serverAddr.sin_zero);  

     /* Bind the address to the socket */
     bind(sock, (struct sockaddr *) &serverAddr, sizeof(serverAddr));

     /* Listen on the socket */
     if(listen(sock,2)==0)
          fprintf(stderr, "Listening\n");
     else
          fprintf(stderr, "Error\n");

     /* Accept call creates a new socket for the incoming connection */
     addr_size = sizeof serverStorage;
     sockfd = accept(sock, (struct sockaddr *) &serverStorage, &addr_size);

     fprintf(stderr, "Connected!\n");
}


/* In order to check which variables have changed */

static winid old_WIN_STATUS;
static winid old_WIN_MESSAGE;
static winid old_WIN_MAP;
static winid old_WIN_INVEN;
static int old_iflags_window_inited;
static int old_iflags_prevmsg_window;
static int old_context_rndencode;
static int old_context_botlx;
static int old_context_botl;
static int old_gbuf[ROWNO][COLNO];

struct changed_vars {
     const char* name;
     int *actual_var;
     int *old_var;
} changed_vars[] = { {"v:WIN_STATUS", &WIN_STATUS, &old_WIN_STATUS},
                     {"v:WIN_MESSAGE", &WIN_MESSAGE, &old_WIN_MESSAGE},
                     {"v:WIN_MAP", &WIN_MAP, &old_WIN_MAP},
                     {"v:WIN_INVEN", &WIN_INVEN, &old_WIN_INVEN},
                     {"v:iflags_window_inited", (int*) &iflags.window_inited, &old_iflags_window_inited},
                     {"v:iflags_prevmsg_window", (int*) &iflags.prevmsg_window, &old_iflags_prevmsg_window},
                     {"v:context_rndencode", &context.rndencode, &old_context_rndencode},
                     {"v:context_botlx", (int*) &context.botlx, &old_context_botlx},
                     {"v:context_botl", (int*) &context.botl, &old_context_botl},
                     {"", NULL, NULL}};

static void
tcp_send_changed_variables()
{
     struct changed_vars *var;
     for (var = changed_vars; var->name[0]; var++) {
          if (*(var->actual_var) != *(var->old_var)) {
               debug(stderr,"Sending variable %s : %d (was %d)\n", var->name, *(var->actual_var), *(var->old_var));
               tcp_send_string(var->name);
               tcp_send_int(*(var->actual_var));
               *(var->old_var) = *(var->actual_var);
          }
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

     for (i = 0; i < ROWNO; i++) {
          cstart[i] = -1;
          cend[i] = -1;
          for (j = 0; j < COLNO; j++) {
               if (gbuf[i][j].glyph != old_gbuf[i][j]) {
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
     if (rstart == -1) {
          rstart = 1;
          rend = 0;
     }

     tcp_send_string("v:gbuf");
     
     tcp_send_char((char) rstart);
     tcp_send_char((char) rend);
     for (i = rstart; i < rend + 1; i++) {
          tcp_send_char((char) cstart[i]);
          tcp_send_char((char) cend[i]);
          for (j = cstart[i]; j < cend[i] + 1; j++) {
               tcp_send_int(gbuf[i][j].glyph);
               old_gbuf[i][j] = gbuf[i][j].glyph;
          }
     }
}


void
tcp_recv_update_variable(toupdate)
char *toupdate;
{
     struct changed_vars *var;
     int i, j;
     char rstart, rend, cstart, cend;
     
     if (!strcmp(toupdate, "v:gbuf")) {
          rstart = tcp_recv_char();
          rend = tcp_recv_char();
          for (i = rstart; i < rend + 1; i++) {
               cstart = tcp_recv_char();
               cend = tcp_recv_char();
               for (j = cstart; j < cend + 1; j++) {
                    gbuf[i][j].glyph = tcp_recv_int();
               }
          }
          return;
     }
          
     for (var = changed_vars; var->name[0]; var++) {
          if (!strcmp(toupdate, var->name)) {
               *(var->actual_var) = tcp_recv_int();
               break;
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
     int n;

     while (len > 0) {
          n = send(sockfd, str, len, 0);
          if (n <= 0) break;
          str += n;
          len -= n;
     }
     if (n <= 0)
          fprintf(stderr, "Error sending bytes.\n");
     return (n <= 0 ? -1 : 0);
}

void
tcp_send_name_command(str)
const char *str;
{
     tcp_send_changed_variables();
     tcp_send_string(str);
}

void
tcp_send_string(str)
const char *str;
{
     if (str) {
          int len = strlen(str);
          tcp_send_int(len);
          tcp_sendall(sockfd, str, len);
     } else {
          tcp_send_int(-1);
     }
     debug("/s:%s\n", str);
}

void
tcp_send_int(n)
int n;
{
     uint32_t m = htonl(n);
     tcp_sendall(sockfd, &m, sizeof(uint32_t));

     debug("/i:%d\n", n);
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
     int n;
     while (len > 0) {
          n = recv(sockfd, buf, len, 0);
          if (n <= 0)
               break;
          buf += n;
          len -= n;
     }
     if (n <= 0)
          fprintf(stderr, "Error sending bytes.\n");
}

/* Returns 1 if the string is null */
int
tcp_recv_string(buf)
char *buf;
{
     int len = tcp_recv_int();
     if (len >= 0) {
          tcp_recvall(sockfd, buf, len);
          buf[len] = '\0';
          debug("s:%s\n", buf);
          return 0;
     } else {
          debug("s NULL\n");
          return 1;
     }
}

int
tcp_recv_int()
{
     uint32_t m;
     tcp_recvall(sockfd, &m, sizeof(uint32_t));

     int m2 = (int) ntohl(m);
     debug("i:%d\n", m2);
     return m2;
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
