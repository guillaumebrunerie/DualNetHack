#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>

#include <hack.h>
#include <ctype.h>
#include <pwd.h>
#include <errno.h>
#include <sys/stat.h>

#include "wintcp.h"

/* #ifdef CHDIR */
/* static void FDECL(chdirx, (const char *, BOOLEAN_P)); */
/* #endif /\* CHDIR *\/ */
/* static boolean NDECL(whoami); */
/* static void FDECL(process_options, (int, char **)); */
/* static void NDECL(wd_message); */

int main(argc, argv)
int *argc;
char **argv;
{
  int clientSocket;
  char buffer[1024];
  struct sockaddr_in serverAddr;
  socklen_t addr_size;

  /* Create the socket */
  clientSocket = socket(PF_INET, SOCK_STREAM, 0);
  
  /* Configure the server address */
  serverAddr.sin_family = AF_INET;
  serverAddr.sin_port = htons(4242);
  serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
  /* Set all bits of the padding field to 0 */
  memset(serverAddr.sin_zero, '\0', sizeof serverAddr.sin_zero);  

  fprintf(stderr, "Connecting\n");
  
  /* Connect the socket to the server using the address struct ----*/
  addr_size = sizeof serverAddr;
  if (connect(clientSocket, (struct sockaddr *) &serverAddr, addr_size) != 0)
       return (errno);

  fprintf(stderr, "Connected!\n");
  tcp_set_sockfd(clientSocket);

  /* Use the standard tty window system */
  choose_windows("tty");
  /* Change directory in order to find symbol files, for instance */
  chdir(HACKDIR);
  /* Initialize options, will probably fail if the server and the client have different symset options */
  initoptions();

  while (1) {
       /* Read the command */
       tcp_recv_string(buffer);

       /* Execute the command */

       if (!strcmp(buffer, "init_nhwindows")) {
            init_nhwindows(argc, argv);
            tcp_send_void();

       } else if (!strcmp(buffer, "player_selection")) {
            /* We always ask the player for their name */
            askname();
            player_selection();
            tcp_send_string(plname);
            tcp_send_int(flags.initrole);
            tcp_send_int(flags.initrace);
            tcp_send_int(flags.initgend);
            tcp_send_int(flags.initalign);

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
            winid window = tcp_recv_int();
            boolean blocking = tcp_recv_boolean();
            display_nhwindow(window, blocking);

            /* A bit hacky, the client needs to tell that
               to the server, but not all the time or it
               slows down the game a lot */
            static int window_inited_sent = 0;
            if (!window_inited_sent) {
                 tcp_send_boolean(iflags.window_inited);
                 if (iflags.window_inited)
                      window_inited_sent = 1;
            }

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
            tcp_send_int(result);

       } else if (!strcmp(buffer, "nh_poskey")) {
            /* Mice are not supported */
            int result = nh_poskey(0, 0, 0);
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

       } else if (!strncmp(buffer, "v:", 2)) {
            /* Change in one of the variables */
            tcp_recv_update_variable(buffer);
       
       } else {
            fprintf(stderr, "Command unknown : %s", buffer);
            break;
       }
  }

  return 0;
}



/* /\*** Functions copied from unixmain.c ***\/ */

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

/* /\* Those functions are used by the server, so I copy-pasted them from unixmain.c *\/ */

/* #ifdef CHDIR */
/* static void */
/* chdirx(dir, wr) */
/* const char *dir; */
/* boolean wr; */
/* { */
/*     if (dir /\* User specified directory? *\/ */
/* #ifdef HACKDIR */
/*         && strcmp(dir, HACKDIR) /\* and not the default? *\/ */
/* #endif */
/*         ) { */
/* #ifdef SECURE */
/*         (void) setgid(getgid()); */
/*         (void) setuid(getuid()); /\* Ron Wessels *\/ */
/* #endif */
/*     } else { */
/* /\* non-default data files is a sign that scores may not be */
/*  * compatible, or perhaps that a binary not fitting this */
/*  * system's layout is being used. */
/*  *\/ */
/* #ifdef VAR_PLAYGROUND */
/*         int len = strlen(VAR_PLAYGROUND); */

/*         fqn_prefix[SCOREPREFIX] = (char *) alloc(len + 2); */
/*         Strcpy(fqn_prefix[SCOREPREFIX], VAR_PLAYGROUND); */
/*         if (fqn_prefix[SCOREPREFIX][len - 1] != '/') { */
/*             fqn_prefix[SCOREPREFIX][len] = '/'; */
/*             fqn_prefix[SCOREPREFIX][len + 1] = '\0'; */
/*         } */
/* #endif */
/*     } */

/* #ifdef HACKDIR */
/*     if (dir == (const char *) 0) */
/*         dir = HACKDIR; */
/* #endif */

/*     if (dir && chdir(dir) < 0) { */
/*         perror(dir); */
/*         error("Cannot chdir to %s.", dir); */
/*     } */

/*     /\* warn the player if we can't write the record file *\/ */
/*     /\* perhaps we should also test whether . is writable *\/ */
/*     /\* unfortunately the access system-call is worthless *\/ */
/*     if (wr) { */
/* #ifdef VAR_PLAYGROUND */
/*         fqn_prefix[LEVELPREFIX] = fqn_prefix[SCOREPREFIX]; */
/*         fqn_prefix[SAVEPREFIX] = fqn_prefix[SCOREPREFIX]; */
/*         fqn_prefix[BONESPREFIX] = fqn_prefix[SCOREPREFIX]; */
/*         fqn_prefix[LOCKPREFIX] = fqn_prefix[SCOREPREFIX]; */
/*         fqn_prefix[TROUBLEPREFIX] = fqn_prefix[SCOREPREFIX]; */
/* #endif */
/*         check_recordfile(dir); */
/*     } */
/* } */
/* #endif /\* CHDIR *\/ */

/* /\* returns True iff we set plname[] to username which contains a hyphen *\/ */
/* static boolean */
/* whoami() */
/* { */
/*     /\* */
/*      * Who am i? Algorithm: 1. Use name as specified in NETHACKOPTIONS */
/*      *			2. Use $USER or $LOGNAME	(if 1. fails) */
/*      *			3. Use getlogin()		(if 2. fails) */
/*      * The resulting name is overridden by command line options. */
/*      * If everything fails, or if the resulting name is some generic */
/*      * account like "games", "play", "player", "hack" then eventually */
/*      * we'll ask him. */
/*      * Note that we trust the user here; it is possible to play under */
/*      * somebody else's name. */
/*      *\/ */
/*     if (!*plname) { */
/*         register const char *s; */

/*         s = nh_getenv("USER"); */
/*         if (!s || !*s) */
/*             s = nh_getenv("LOGNAME"); */
/*         if (!s || !*s) */
/*             s = getlogin(); */

/*         if (s && *s) { */
/*             (void) strncpy(plname, s, sizeof plname - 1); */
/*             if (index(plname, '-')) */
/*                 return TRUE; */
/*         } */
/*     } */
/*     return FALSE; */
/* } */
