#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <netdb.h>

#include <hack.h>
#include <ctype.h>
#include <pwd.h>
#include <errno.h>
#include <sys/stat.h>

#include "wintcp.h"
#include "decl.h"

#ifdef CHDIR
static void FDECL(chdirx, (const char *, BOOLEAN_P));
#endif /* CHDIR */
static boolean NDECL(whoami);
/* static void FDECL(process_options, (int, char **)); */
/* static void NDECL(wd_message); */

void
client_init()
{
#ifdef CHDIR
    register char *dir;
#endif
    boolean exact_username;

  /* Use the standard tty window system */
  choose_windows("tty");

#ifdef CHDIR /* otherwise no chdir() */
             /*
              * See if we must change directory to the playground.
              * (Perhaps hack runs suid and playground is inaccessible
              *  for the player.)
              * The environment variable HACKDIR is overridden by a
              *  -d command line option (must be the first option given)
              */
    dir = nh_getenv("NETHACKDIR");
    if (!dir)
        dir = nh_getenv("HACKDIR");
#endif

/*
 * Change directories before we initialize the window system so
 * we can find the tile file.
 */
#ifdef CHDIR
    chdirx(dir, 1);
#endif

#ifdef _M_UNIX
    check_sco_console();
#endif
#ifdef __linux__
    check_linux_console();
#endif
    initoptions();
#ifdef PANICTRACE
    /* ARGV0 = argv[0]; /\* save for possible stack trace *\/ */
#ifndef NO_SIGNAL
    panictrace_setsignals(TRUE);
#endif
#endif

    exact_username = whoami();

#ifdef _M_UNIX
    init_sco_cons();
#endif
#ifdef __linux__
    init_linux_cons();
#endif

#ifdef DEF_PAGER
    if (!(catmore = nh_getenv("HACKPAGER"))
        && !(catmore = nh_getenv("PAGER")))
        catmore = DEF_PAGER;
#endif
#ifdef MAIL
    getmailstatus();
#endif

    /* wizard mode access is deferred until here */
    set_playmode(); /* sets plname to "wizard" for wizard mode */
    if (exact_username) {
        /*
         * FIXME: this no longer works, ever since 3.3.0
         * when plnamesuffix() was changed to find
         * Name-Role-Race-Gender-Alignment.  It removes
         * all dashes rather than just the last one,
         * regardless of whether whatever follows each
         * dash matches role, race, gender, or alignment.
         */
        /* guard against user names with hyphens in them */
        int len = strlen(plname);
        /* append the current role, if any, so that last dash is ours */
        if (++len < (int) sizeof plname)
            (void) strncat(strcat(plname, "-"), pl_character,
                           sizeof plname - len - 1);
    }
    /* strip role,race,&c suffix; calls askname() if plname[] is empty
       or holds a generic user name like "player" or "games" */
    plnamesuffix();
    you_player = &player1;
}

int main(argc, argv)
int argc;
char *argv[];
{
  int clientSocket;
  char buffer[1024];
  struct sockaddr_in serverAddr;
  socklen_t addr_size;
  int i;
  char queue[1000];

  char *host = argc >= 2 ? argv[1] : "127.0.0.1";
  
  dualnh_init_players();
  client_init();
  
  struct addrinfo hints, *res;
  
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  int err = getaddrinfo(host, "4242", &hints, &res);
  if (err != 0) {
       fprintf(stderr, "Failed to resolve remote socket address (err=%d)\n", err);
       return err;
  }
  
  /* Create the socket */
  clientSocket = socket(res->ai_family, res->ai_socktype, res->ai_protocol);

  /* /\* Configure the server address *\/ */
  /* serverAddr.sin_family = AF_INET; */
  /* serverAddr.sin_port = htons(4242); */
  /* serverAddr.sin_addr.s_addr = inet_addr(host); */
  /* /\* Set all bits of the padding field to 0 *\/ */
  /* memset(serverAddr.sin_zero, '\0', sizeof serverAddr.sin_zero);   */

  fprintf(stdout, "Connecting to host %s\n", host);
  
  /* Connect the socket to the server using the address struct ----*/
  if (connect(clientSocket, res->ai_addr, res->ai_addrlen) != 0)
       return errno;

  fprintf(stdout, "Connected!\n");
  tcp_set_sockfd(clientSocket);

  while (1) {
       /* Read the command */
       fprintf(stderr, "Waiting\n");
       tcp_recv_string(buffer);
       fprintf(stderr, "Parsing %s\n", buffer);

       /* Execute the command */

       if (!strcmp(buffer, "init_nhwindows")) {
            init_nhwindows(&argc, argv);
            clear_glyph_buffer();
            tcp_send_void();

       } else if (!strcmp(buffer, "player_selection")) {
            /* We always ask the player for their name */
            askname();
            tcp_send_string(plname);
            player_selection();
            tcp_send_int(uflags.initrole);
            tcp_send_int(uflags.initrace);
            tcp_send_int(uflags.initgend);
            tcp_send_int(uflags.initalign);

       } else if (!strcmp(buffer, "get_nh_event")) {
            get_nh_event();

       } else if (!strcmp(buffer, "suspend_nhwindows")) {
            char str[BUFSZ];
            tcp_recv_string(str);
            suspend_nhwindows(str);

       } else if (!strcmp(buffer, "resume_nhwindows")) {
            resume_nhwindows();

       } else if (!strcmp(buffer, "exit_nhwindows")) {
            char str[BUFSZ];
            tcp_recv_string(str);
            exit_nhwindows(str);

       } else if (!strcmp(buffer, "create_nhwindow")) {
            int type = tcp_recv_int();
            winid id = create_nhwindow(type);
            tcp_send_int(id);

       } else if (!strcmp(buffer, "clear_nhwindow")) {
            winid window = tcp_recv_int();
            clear_nhwindow(window);
            
       } else if (!strcmp(buffer, "display_nhwindow")) {
            fprintf(stderr, "Dealing 1\n");
            winid window = tcp_recv_int();
            boolean blocking = tcp_recv_boolean();
            display_nhwindow(window, blocking);
            fprintf(stderr, "Dealing 2\n");

            /* A bit hacky, the client needs to tell that
               to the server, but not all the time or it
               slows down the game a lot */
            static int window_inited_sent = 0;
            if (!window_inited_sent) {
                 tcp_send_boolean(iflags.window_inited);
                 if (iflags.window_inited)
                      window_inited_sent = 1;
            }
            fprintf(stderr, "Dealing 3\n");

       } else if (!strcmp(buffer, "destroy_nhwindow")) {
            winid window = tcp_recv_int();
            destroy_nhwindow(window);

       } else if (!strcmp(buffer, "curs")) {
            winid window = tcp_recv_int();
            int x = tcp_recv_int();
            int y = tcp_recv_int();
            curs(window, x, y);

       } else if (!strcmp(buffer, "putstr")) {
            winid window = tcp_recv_int();
            int attr = tcp_recv_int();
            char str[1024];
            tcp_recv_string(str);
            putstr(window, attr, str);
            
       } else if (!strcmp(buffer, "putmixed")) {
            winid window = tcp_recv_int();
            int attr = tcp_recv_int();
            char str[1024];
            tcp_recv_string(str);
            putmixed(window, attr, str);

       } else if (!strcmp(buffer, "display_file")) {
            char fname[1024];
            int fname_isnull = tcp_recv_string(fname);
            boolean complain = tcp_recv_boolean();
            display_file(fname_isnull ? NULL : fname, complain);

       } else if (!strcmp(buffer, "start_menu")) {
            winid window = tcp_recv_int();
            start_menu(window);

       } else if (!strcmp(buffer, "add_menu")) {
            winid window = tcp_recv_int();
            int glyph = tcp_recv_int();

            anything identifier;
            tcp_recv_anything(&identifier);
            
            
            char ch = tcp_recv_char();
            char gch = tcp_recv_char();
            int attr = tcp_recv_int();
            char str[1024];
            tcp_recv_string(str);
            boolean preselected = tcp_recv_boolean();
            add_menu(window, glyph, &identifier, ch, gch, attr, str, preselected);

       } else if (!strcmp(buffer, "end_menu")) {
            winid window = tcp_recv_int();
            char prompt[1024];
            int prompt_isnull = tcp_recv_string(prompt);
            end_menu(window, prompt_isnull ? NULL : prompt);

       } else if (!strcmp(buffer, "select_menu")) {
            winid window = tcp_recv_int();
            int how = tcp_recv_int();
            menu_item *menu_list;
            int res = select_menu(window, how, &menu_list);
            tcp_send_int(res);
            if (res > 0) {
                 tcp_send_list_menu_item(menu_list, res);
            }
            /* free menu_list? */

       } else if (!strcmp(buffer, "message_menu")) {
            char let = tcp_recv_char();
            int how = tcp_recv_int();
            char mesg[BUFSZ];
            tcp_recv_string(mesg);
            char res = message_menu(let, how, mesg);
            tcp_send_char(res);

       } else if (!strcmp(buffer, "update_inventory")) {
            update_inventory();

       } else if (!strcmp(buffer, "mark_synch")) {
            mark_synch();
            tcp_send_void();

       } else if (!strcmp(buffer, "wait_synch")) {
            wait_synch();
            tcp_send_void();

       } else if (!strcmp(buffer, "cliparound")) {
            int x = tcp_recv_int();
            int y = tcp_recv_int();
            extern boolean restoring;
            restoring = tcp_recv_boolean();
            tcp_recv_d_level(&u.uz0);
            tcp_recv_d_level(&u.uz);
            cliparound(x, y);

       } else if (!strcmp(buffer, "print_glyph")) {
            winid window = tcp_recv_int();
            xchar x = tcp_recv_xchar();
            xchar y = tcp_recv_xchar();
            int glyph = tcp_recv_int();
            int bkglyph = tcp_recv_int();
            print_glyph(window, x, y, glyph, bkglyph);

       } else if (!strcmp(buffer, "raw_print")) {
            char str[1024];
            tcp_recv_string(str);
            raw_print(str);

       } else if (!strcmp(buffer, "raw_print_bold")) {
            char str[1024];
            tcp_recv_string(str);
            raw_print_bold(str);

       } else if (!strcmp(buffer, "nhgetch")) {
            int result = nhgetch();
            if (result != -42)
              tcp_send_int(result);

       } else if (!strcmp(buffer, "nhgetch_queue_length")) {
            int result = nhgetch_queue_length();
            tcp_send_int(result);

       } else if (!strcmp(buffer, "nh_poskey")) {
            /* Mice are not supported */
            int result = nh_poskey(0, 0, 0);
            if (result != -42)
                 tcp_send_int(result);

       } else if (!strcmp(buffer, "nhbell")) {
            nhbell();

       } else if (!strcmp(buffer, "doprev_message")) {
            int res = nh_doprev_message();
            tcp_send_int(res);

       } else if (!strcmp(buffer, "yn_function")) {
            char a[1024];
            char b[1024];
            int a_isnull = tcp_recv_string(a);
            int b_isnull = tcp_recv_string(b);
            CHAR_P c = tcp_recv_xchar();
            char result = yn_function(a_isnull ? NULL : a, b_isnull ? NULL : b, c);
            tcp_send_xchar(result);

       } else if (!strcmp(buffer, "getlin")) {
            char a[BUFSZ];
            tcp_recv_string(a);
            char b[BUFSZ];
            getlin(a, b);
            tcp_send_string(b);

       } else if (!strcmp(buffer, "get_ext_cmd")) {
            int res = get_ext_cmd();
            tcp_send_int(res);

       } else if (!strcmp(buffer, "delay_output")) {
            delay_output();

       } else if (!strcmp(buffer, "start_screen")) {
            start_screen();

       } else if (!strcmp(buffer, "end_screen")) {
            end_screen();

       } else if (!strcmp(buffer, "outrip")) {
            winid a = tcp_recv_int();
            int b = tcp_recv_int();
            time_t c = tcp_recv_long();
            outrip(a, b, c);

       } else if (!strcmp(buffer, "getmsghistory")) {
            BOOLEAN_P a = tcp_recv_boolean();
            char *str = getmsghistory(a);
            tcp_send_string(str);

       } else if (!strcmp(buffer, "putmsghistory")) {
            char buf[1024];
            int buf_isnull = tcp_recv_string(buf);
            BOOLEAN_P b = tcp_recv_boolean();
            putmsghistory(buf_isnull ? NULL : buf, b);

       } else if (!strcmp(buffer, "v:gbuf")) {
            int i, j;
            char rstart, rend, cstart, cend;
            fprintf(stderr, "Doing stuff with gbuf\n");
            rstart = tcp_recv_char();
            rend = tcp_recv_char();
            for (i = rstart; i < rend + 1; i++) {
                 cstart = tcp_recv_char();
                 cend = tcp_recv_char();
                 for (j = cstart; j < cend + 1; j++) {
                      gbuf[i][j].glyph = tcp_recv_int();
                 }
            }

       } else if (!strncmp(buffer, "v:", 2)) {
            /* Change in one of the variables */
            tcp_recv_update_variable(buffer);

       } else if (!strcmp(buffer, "QUEUE")) {
            tcp_recv_string(queue);
            fprintf(stderr, "Queue received : %s\n", queue);
            for (i = 0; queue[i]; i++)
                 dualnh_push(queue[i]);

       } else if (!strcmp(buffer, "")) {
            
       } else {
            fprintf(stderr, "Command unknown : %s", buffer);
            break;
       }
  }

  return 0;
}



/* /\*** Functions copied from unixmain.c ***\/ */

#ifdef CHDIR
static void
chdirx(dir, wr)
const char *dir;
boolean wr;
{
    if (dir /* User specified directory? */
#ifdef HACKDIR
        && strcmp(dir, HACKDIR) /* and not the default? */
#endif
        ) {
#ifdef SECURE
        (void) setgid(getgid());
        (void) setuid(getuid()); /* Ron Wessels */
#endif
    } else {
/* non-default data files is a sign that scores may not be
 * compatible, or perhaps that a binary not fitting this
 * system's layout is being used.
 */
#ifdef VAR_PLAYGROUND
        int len = strlen(VAR_PLAYGROUND);

        fqn_prefix[SCOREPREFIX] = (char *) alloc(len + 2);
        Strcpy(fqn_prefix[SCOREPREFIX], VAR_PLAYGROUND);
        if (fqn_prefix[SCOREPREFIX][len - 1] != '/') {
            fqn_prefix[SCOREPREFIX][len] = '/';
            fqn_prefix[SCOREPREFIX][len + 1] = '\0';
        }
#endif
    }

#ifdef HACKDIR
    if (dir == (const char *) 0)
        dir = HACKDIR;
#endif

    if (dir && chdir(dir) < 0) {
        perror(dir);
        error("Cannot chdir to %s.", dir);
    }

    /* warn the player if we can't write the record file */
    /* perhaps we should also test whether . is writable */
    /* unfortunately the access system-call is worthless */
    if (wr) {
#ifdef VAR_PLAYGROUND
        fqn_prefix[LEVELPREFIX] = fqn_prefix[SCOREPREFIX];
        fqn_prefix[SAVEPREFIX] = fqn_prefix[SCOREPREFIX];
        fqn_prefix[BONESPREFIX] = fqn_prefix[SCOREPREFIX];
        fqn_prefix[LOCKPREFIX] = fqn_prefix[SCOREPREFIX];
        fqn_prefix[TROUBLEPREFIX] = fqn_prefix[SCOREPREFIX];
#endif
        check_recordfile(dir);
    }
}
#endif /* CHDIR */

/* returns True iff we set plname[] to username which contains a hyphen */
static boolean
whoami()
{
    /*
     * Who am i? Algorithm: 1. Use name as specified in NETHACKOPTIONS
     *			2. Use $USER or $LOGNAME	(if 1. fails)
     *			3. Use getlogin()		(if 2. fails)
     * The resulting name is overridden by command line options.
     * If everything fails, or if the resulting name is some generic
     * account like "games", "play", "player", "hack" then eventually
     * we'll ask him.
     * Note that we trust the user here; it is possible to play under
     * somebody else's name.
     */
    if (!*plname) {
        register const char *s;

        s = nh_getenv("USER");
        if (!s || !*s)
            s = nh_getenv("LOGNAME");
        if (!s || !*s)
            s = getlogin();

        if (s && *s) {
            (void) strncpy(plname, s, sizeof plname - 1);
            if (index(plname, '-'))
                return TRUE;
        }
    }
    return FALSE;
}

static boolean wiz_error_flag = FALSE;

static struct passwd *
get_unix_pw()
{
    char *user;
    unsigned uid;
    static struct passwd *pw = (struct passwd *) 0;

    if (pw)
        return pw; /* cache answer */

    uid = (unsigned) getuid();
    user = getlogin();
    if (user) {
        pw = getpwnam(user);
        if (pw && ((unsigned) pw->pw_uid != uid))
            pw = 0;
    }
    if (pw == 0) {
        user = nh_getenv("USER");
        if (user) {
            pw = getpwnam(user);
            if (pw && ((unsigned) pw->pw_uid != uid))
                pw = 0;
        }
        if (pw == 0) {
            pw = getpwuid(uid);
        }
    }
    return pw;
}

void
sethanguphandler(handler)
void FDECL((*handler), (int));
{
#ifdef SA_RESTART
    /* don't want reads to restart.  If SA_RESTART is defined, we know
     * sigaction exists and can be used to ensure reads won't restart.
     * If it's not defined, assume reads do not restart.  If reads restart
     * and a signal occurs, the game won't do anything until the read
     * succeeds (or the stream returns EOF, which might not happen if
     * reading from, say, a window manager). */
    struct sigaction sact;

    (void) memset((genericptr_t) &sact, 0, sizeof sact);
    sact.sa_handler = (SIG_RET_TYPE) handler;
    (void) sigaction(SIGHUP, &sact, (struct sigaction *) 0);
#ifdef SIGXCPU
    (void) sigaction(SIGXCPU, &sact, (struct sigaction *) 0);
#endif
#else /* !SA_RESTART */
    (void) signal(SIGHUP, (SIG_RET_TYPE) handler);
#ifdef SIGXCPU
    (void) signal(SIGXCPU, (SIG_RET_TYPE) handler);
#endif
#endif /* ?SA_RESTART */
}

/* validate wizard mode if player has requested access to it */
boolean
authorize_wizard_mode()
{
    struct passwd *pw = get_unix_pw();
    if (pw && sysopt.wizards && sysopt.wizards[0]) {
        if (check_user_string(sysopt.wizards))
            return TRUE;
    }
    wiz_error_flag = TRUE; /* not being allowed into wizard mode */
    return FALSE;
}

boolean
check_user_string(optstr)
char *optstr;
{
    struct passwd *pw = get_unix_pw();
    int pwlen;
    char *eop, *w;
    char *pwname;
    if (optstr[0] == '*')
        return TRUE; /* allow any user */
    if (!pw)
        return FALSE;
    if (sysopt.check_plname)
        pwname = plname;
    else
        pwname = pw->pw_name;
    pwlen = strlen(pwname);
    eop = eos(optstr);
    w = optstr;
    while (w + pwlen <= eop) {
        if (!*w)
            break;
        if (isspace(*w)) {
            w++;
            continue;
        }
        if (!strncmp(w, pwname, pwlen)) {
            if (!w[pwlen] || isspace(w[pwlen]))
                return TRUE;
        }
        while (*w && !isspace(*w))
            w++;
    }
    return FALSE;
}
