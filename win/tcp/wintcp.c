#include "hack.h"
#include "wintcp.h"
#include "dualnethack.h"

#include <arpa/inet.h>

#ifdef TCP_GRAPHICS

/* Functions from the protocol */

void
tcp_init_nhwindows(argcp, argv)
int *argcp UNUSED;
char **argv UNUSED;
{
     /* tcp_init_connection(); */
     tcp_send_name_command("init_nhwindows");
     tcp_recv_void();
}


void
tcp_player_selection()
{
     tcp_send_name_command("player_selection");
     tcp_recv_string(you_player->plname);
     you_player->flags.initrole = tcp_recv_int();
     you_player->flags.initrace = tcp_recv_int();
     you_player->flags.initgend = tcp_recv_int();
     you_player->flags.initalign = tcp_recv_int();
}

void
tcp_askname()
{
     /* Not yet implemented (not sure if it is ever used) */
     tcp_send_name_command("askname");
}

void
tcp_get_nh_event()
{
     tcp_send_name_command("get_nh_event");
}

void
tcp_suspend_nhwindows(str)
const char *str;
{
     tcp_send_name_command("suspend_nhwindows");
     tcp_send_string(str);
}

void
tcp_resume_nhwindows()
{
     tcp_send_name_command("resume_nhwindows");
}

void
tcp_exit_nhwindows(str)
const char *str;
{
     tcp_send_name_command("exit_nhwindows");
     tcp_send_string(str);
}

winid
tcp_create_nhwindow(type)
int type;
{
     tcp_send_name_command("create_nhwindow");
     tcp_send_int(type);
     return (tcp_recv_int());
}

void
tcp_clear_nhwindow(window)
winid window;
{
     /* We update the client-side gbuf, because we might need it to redraw
        what was behind the window */
     tcp_send_gbuf();
     
     tcp_send_name_command("clear_nhwindow");
     tcp_send_int(window);
}

void
tcp_display_nhwindow(window, blocking)
winid window;
boolean blocking;
{
     tcp_send_name_command("display_nhwindow");
     tcp_send_int(window);
     tcp_send_boolean(blocking);

     static __thread int window_inited_received = 0;
     if (!window_inited_received) {
          you_player->iflags.window_inited = tcp_recv_boolean();
          if (you_player->iflags.window_inited)
               window_inited_received = 1;
     }
}

void
tcp_dismiss_nhwindow(window)
winid window;
{
     /* Not yet implemented (not sure if the server ever uses this) */
     tcp_send_name_command("dismiss_nhwindow");
}

void
tcp_destroy_nhwindow(window)
winid window;
{
     tcp_send_gbuf();
     tcp_send_name_command("destroy_nhwindow");
     tcp_send_int(window);
}

void
tcp_curs(window, x, y)
winid window;
register int x, y; /* not xchar: perhaps xchar is unsigned and
                      curx-x would be unsigned as well */
{
     tcp_send_name_command("curs");
     tcp_send_int(window);
     tcp_send_int(x);
     tcp_send_int(y);
}

void
tcp_putstr(window, attr, str)
winid window;
int attr;
const char *str;
{
     tcp_send_name_command("putstr");
     tcp_send_int(window);
     tcp_send_int(attr);
     tcp_send_string(str);
}

void
tcp_putmixed(window, attr, str)
winid window;
int attr;
const char *str;
{
     tcp_send_name_command("putmixed");
     tcp_send_int(window);
     tcp_send_int(attr);
     tcp_send_string(str);
}

void
tcp_display_file(fname, complain)
const char *fname;
boolean complain;
{
     tcp_send_name_command("display_file");
     tcp_send_string(fname);
     tcp_send_boolean(complain);
}

void
tcp_start_menu(window)
winid window;
{
     tcp_send_name_command("start_menu");
     tcp_send_int(window);
}

/*ARGSUSED*/
/*
 * Add a menu item to the beginning of the menu list.  This list is reversed
 * later.
 */
void
tcp_add_menu(window, glyph, identifier, ch, gch, attr, str, preselected)
winid window;               /* window to use, must be of type NHW_MENU */
int glyph UNUSED;           /* glyph to display with item (not used) */
const anything *identifier; /* what to return if selected */
char ch;                    /* keyboard accelerator (0 = pick our own) */
char gch;                   /* group accelerator (0 = no group) */
int attr;                   /* attribute for string (like tty_putstr()) */
const char *str;            /* menu string */
boolean preselected;        /* item is marked as selected */
{
     tcp_send_name_command("add_menu");
     tcp_send_int(window);
     tcp_send_int(glyph);

     /* I don’t know why identifier is a pointer, so I just send its contents */
     tcp_send_anything(*identifier);

     tcp_send_char(ch);
     tcp_send_char(gch);
     tcp_send_int(attr);
     tcp_send_string(str);
     tcp_send_boolean(preselected);
}

/*
 * End a menu in this window, window must a type NHW_MENU.  This routine
 * processes the string list.  We calculate the # of pages, then assign
 * keyboard accelerators as needed.  Finally we decide on the width and
 * height of the window.
 */
void
tcp_end_menu(window, prompt)
winid window;       /* menu to use */
const char *prompt; /* prompt to for menu */
{
     tcp_send_name_command("end_menu");
     tcp_send_int(window);
     tcp_send_string(prompt);
}

int
tcp_select_menu(window, how, menu_list)
winid window;
int how;
menu_item **menu_list;
{
     tcp_send_name_command("select_menu");
     tcp_send_int(window);
     tcp_send_int(how);
     int res = tcp_recv_int();
     if (res > 0)
       tcp_recv_list_menu_item(menu_list, res);
     return res;
}

/* special hack for treating top line --More-- as a one item menu */
char
tcp_message_menu(let, how, mesg)
char let;
int how;
const char *mesg;
{
     tcp_send_name_command("message_menu");
     tcp_send_char(let);
     tcp_send_int(how);
     tcp_send_string(mesg);
     char res = tcp_recv_char();
     return res;
}

void
tcp_update_inventory()
{
     tcp_send_name_command("update_inventory");
}

void
tcp_mark_synch()
{
     tcp_send_name_command("mark_synch");
     tcp_recv_void();
}

void
tcp_wait_synch()
{
     tcp_send_name_command("wait_synch");
     tcp_recv_void();
}

#ifdef CLIPPING
void
tcp_cliparound(x, y)
int x, y;
{
     tcp_send_name_command("cliparound");
     tcp_send_int(x);
     tcp_send_int(y);
     tcp_send_boolean(restoring);
     tcp_send_d_level(u.uz0);
     tcp_send_d_level(u.uz);
}
#endif /* CLIPPING */

/*
 *  tcp_print_glyph
 *
 *  Print the glyph to the output device.  Don't flush the output device.
 *
 *  Since this is only called from show_glyph(), it is assumed that the
 *  position and glyph are always correct (checked there)!
 */

void
tcp_print_glyph(window, x, y, glyph, bkglyph)
winid window;
xchar x, y;
int glyph;
int bkglyph UNUSED;
{
     tcp_send_name_command("print_glyph");
     tcp_send_int(window);
     tcp_send_xchar(x);
     tcp_send_xchar(y);
     tcp_send_int(glyph);
     tcp_send_int(bkglyph);
}

void
tcp_raw_print(str)
const char *str;
{
     tcp_send_name_command("raw_print");
     tcp_send_string(str);
}

void
tcp_raw_print_bold(str)
const char *str;
{
     tcp_send_name_command("raw_print_bold");
     tcp_send_string(str);
}

int
tcp_nhgetch()
{
     tcp_send_name_command("nhgetch");
     int result = tcp_recv_int();
     return result;
}

/*
 * return a key, or 0, in which case a mouse button was pressed
 * mouse events should be returned as character postitions in the map window.
 * Since normal tty's don't have mice, just return a key.
 */
/*ARGSUSED*/
int
tcp_nh_poskey(x, y, mod)
int *x, *y, *mod;
{
     /* We do not support the arguments here */
     tcp_send_name_command("nh_poskey");
     int result = tcp_recv_int();
     return result;
}

void
tcp_nhbell()
{
     tcp_send_name_command("nhbell");
}

int
tcp_doprev_message()
{
     tcp_send_name_command("doprev_message");
     int res = tcp_recv_int();
     return res;
}

char
tcp_yn_function(a, b, c)
const char * a;
const char * b;
CHAR_P c;
{
     tcp_send_name_command("yn_function");
     tcp_send_string(a);
     tcp_send_string(b);
     tcp_send_xchar(c);
     char res = tcp_recv_char();
     return res;
}

void
tcp_getlin(a, b)
const char * a;
char * b;
{
     tcp_send_name_command("getlin");
     tcp_send_string(a);
     tcp_recv_string(b);
}

int
tcp_get_ext_cmd()
{
     tcp_send_name_command("get_ext_cmd");
     int res = tcp_recv_int();
     return res;
}

void
tcp_number_pad(a)
int a;
{
     /* Not yet implemented */
     tcp_send_name_command("number_pad");
}

void
tcp_delay_output()
{
     tcp_send_name_command("delay_output");
}

#ifdef POSITIONBAR
void
tcp_update_positionbar(posbar)
char *posbar;
{
     /* Not yet implemented */
     tcp_send_name_command("update_positionbar");
}
#endif

void
tcp_start_screen()
{
     tcp_send_name_command("start_screen");
}

void
tcp_end_screen()
{
     tcp_send_name_command("end_screen");
}

void
tcp_outrip(a, b, c)
winid a;
int b;
time_t c;
{
     /* Buggy, we need to send more things */
     tcp_send_name_command("outrip");
     tcp_send_int(a);
     tcp_send_int(b);
     tcp_send_long(c);
}

void
tcp_preference_update(a)
const char * a;
{
     /* Not yet implemented */
     tcp_send_name_command("preference_update");
}

char snapshot[BUFSZ];

char*
tcp_getmsghistory(a)
BOOLEAN_P a;
{
     tcp_send_name_command("getmsghistory");
     tcp_send_boolean(a);
     int res_isnull = tcp_recv_string(snapshot);
     return res_isnull ? NULL : snapshot;
}

void
tcp_putmsghistory(a, b)
const char * a;
BOOLEAN_P b;
{
     tcp_send_name_command("putmsghistory");
     tcp_send_string(a);
     tcp_send_boolean(b);
}

boolean
tcp_can_suspend()
{
     /* Not yet implemented */
     tcp_send_name_command("can_suspend");
}

#ifdef STATUS_VIA_WINDOWPORT

void
tcp_status_init()
{
     /* Not yet implemented */
     tcp_send_name_command("status_init");
}

/*
 *  *_status_update()
 *      -- update the value of a status field.
 *      -- the fldindex identifies which field is changing and
 *         is an integer index value from botl.h
 *      -- fldindex could be any one of the following from botl.h:
 *         BL_TITLE, BL_STR, BL_DX, BL_CO, BL_IN, BL_WI, BL_CH,
 *         BL_ALIGN, BL_SCORE, BL_CAP, BL_GOLD, BL_ENE, BL_ENEMAX,
 *         BL_XP, BL_AC, BL_HD, BL_TIME, BL_HUNGER, BL_HP, BL_HPMAX,
 *         BL_LEVELDESC, BL_EXP, BL_CONDITION
 *      -- fldindex could also be BL_FLUSH (-1), which is not really
 *         a field index, but is a special trigger to tell the
 *         windowport that it should redisplay all its status fields,
 *         even if no changes have been presented to it.
 *      -- ptr is usually a "char *", unless fldindex is BL_CONDITION.
 *         If fldindex is BL_CONDITION, then ptr is a long value with
 *         any or none of the following bits set (from botl.h):
 *              BL_MASK_STONE           0x00000001L
 *              BL_MASK_SLIME           0x00000002L
 *              BL_MASK_STRNGL          0x00000004L
 *              BL_MASK_FOODPOIS        0x00000008L
 *              BL_MASK_TERMILL         0x00000010L
 *              BL_MASK_BLIND           0x00000020L
 *              BL_MASK_DEAF            0x00000040L
 *              BL_MASK_STUN            0x00000080L
 *              BL_MASK_CONF            0x00000100L
 *              BL_MASK_HALLU           0x00000200L
 *              BL_MASK_LEV             0x00000400L
 *              BL_MASK_FLY             0x00000800L
 *              BL_MASK_RIDE            0x00001000L
 *      -- The value passed for BL_GOLD includes an encoded leading
 *         symbol for GOLD "\GXXXXNNNN:nnn". If the window port needs to use
 *         the textual gold amount without the leading "$:" the port will
 *         have to skip past ':' in the passed "ptr" for the BL_GOLD case.
 */
void
tcp_status_update(fldidx, ptr, chg, percent)
int fldidx, chg, percent;
genericptr_t ptr;
{
     /* Not yet implemented */
     tcp_send_name_command("status_update");
}

#ifdef STATUS_HILITES
/*
 *  status_threshold(int fldidx, int threshholdtype, anything threshold,
 *                   int behavior, int under, int over)
 *
 *        -- called when a hiliting preference is added, changed, or
 *           removed.
 *        -- the fldindex identifies which field is having its hiliting
 *           preference set. It is an integer index value from botl.h
 *        -- fldindex could be any one of the following from botl.h:
 *           BL_TITLE, BL_STR, BL_DX, BL_CO, BL_IN, BL_WI, BL_CH,
 *           BL_ALIGN, BL_SCORE, BL_CAP, BL_GOLD, BL_ENE, BL_ENEMAX,
 *           BL_XP, BL_AC, BL_HD, BL_TIME, BL_HUNGER, BL_HP, BL_HPMAX,
 *           BL_LEVELDESC, BL_EXP, BL_CONDITION
 *        -- datatype is P_INT, P_UINT, P_LONG, or P_MASK.
 *        -- threshold is an "anything" union which can contain the
 *           datatype value.
 *        -- behavior is used to define how threshold is used and can
 *           be BL_TH_NONE, BL_TH_VAL_PERCENTAGE, BL_TH_VAL_ABSOLUTE,
 *           or BL_TH_UPDOWN. BL_TH_NONE means don't do anything above
 *           or below the threshold.  BL_TH_VAL_PERCENTAGE treats the
 *           threshold value as a precentage of the maximum possible
 *           value. BL_TH_VAL_ABSOLUTE means that the threshold is an
 *           actual value. BL_TH_UPDOWN means that threshold is not
 *           used, and the two below/above hilite values indicate how
 *           to display something going down (under) or rising (over).
 *        -- under is the hilite attribute used if value is below the
 *           threshold. The attribute can be BL_HILITE_NONE,
 *           BL_HILITE_INVERSE, BL_HILITE_BOLD (-1, -2, or -3), or one
 *           of the color indexes of CLR_BLACK, CLR_RED, CLR_GREEN,
 *           CLR_BROWN, CLR_BLUE, CLR_MAGENTA, CLR_CYAN, CLR_GRAY,
 *           CLR_ORANGE, CLR_BRIGHT_GREEN, CLR_YELLOW, CLR_BRIGHT_BLUE,
 *           CLR_BRIGHT_MAGENTA, CLR_BRIGHT_CYAN, or CLR_WHITE (0 - 15).
 *        -- over is the hilite attribute used if value is at or above
 *           the threshold. The attribute can be BL_HILITE_NONE,
 *           BL_HILITE_INVERSE, BL_HILITE_BOLD (-1, -2, or -3), or one
 *           of the color indexes of CLR_BLACK, CLR_RED, CLR_GREEN,
 *           CLR_BROWN, CLR_BLUE, CLR_MAGENTA, CLR_CYAN, CLR_GRAY,
 *           CLR_ORANGE, CLR_BRIGHT_GREEN, CLR_YELLOW, CLR_BRIGHT_BLUE,
 *           CLR_BRIGHT_MAGENTA, CLR_BRIGHT_CYAN, or CLR_WHITE (0 - 15).
 */
void
tcp_status_threshold(fldidx, thresholdtype, threshold, behavior, under, over)
int fldidx, thresholdtype;
int behavior, under, over;
anything threshold;
{
     /* Not yet implemented */
     tcp_send_name_command("status_threshold");
}

#endif /* STATUS_HILITES */
#endif /*STATUS_VIA_WINDOWPORT*/


/* Interface definition, for windows.c */
struct window_procs tcp_procs = {
    "tcp",
        WC_COLOR | WC_HILITE_PET | WC_INVERSE | WC_EIGHT_BIT_IN,
        WC2_DARKGRAY,
    tcp_init_nhwindows, tcp_player_selection, tcp_askname, tcp_get_nh_event,
    tcp_exit_nhwindows, tcp_suspend_nhwindows, tcp_resume_nhwindows,
    tcp_create_nhwindow, tcp_clear_nhwindow, tcp_display_nhwindow,
    tcp_destroy_nhwindow, tcp_curs, tcp_putstr, tcp_putmixed,
    tcp_display_file, tcp_start_menu, tcp_add_menu, tcp_end_menu,
    tcp_select_menu, tcp_message_menu, tcp_update_inventory, tcp_mark_synch,
    tcp_wait_synch,
#ifdef CLIPPING
    tcp_cliparound,
#endif
#ifdef POSITIONBAR
    tcp_update_positionbar,
#endif
    tcp_print_glyph, tcp_raw_print, tcp_raw_print_bold, tcp_nhgetch,
    tcp_nh_poskey, tcp_nhbell, tcp_doprev_message, tcp_yn_function,
    tcp_getlin, tcp_get_ext_cmd, tcp_number_pad, tcp_delay_output,
#ifdef CHANGE_COLOR /* the Mac uses a palette device */
    tcp_change_color,
#ifdef MAC
    tcp_change_background, tcp_set_font_name,
#endif
    tcp_get_color_string,
#endif

    /* other defs that really should go away (they're tty specific) */
    tcp_start_screen, tcp_end_screen, tcp_outrip,
    tcp_preference_update,
    tcp_getmsghistory, tcp_putmsghistory,
#ifdef STATUS_VIA_WINDOWPORT
    tcp_status_init,
    tcp_status_finish, tcp_status_enablefield,
    tcp_status_update,
#ifdef STATUS_HILITES
    tcp_status_threshold,
#endif
#endif
    tcp_can_suspend,
};

#endif /* TCP_GRAPHICS */

/*wintcp.c*/
