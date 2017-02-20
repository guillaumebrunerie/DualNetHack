#ifndef WINTCP_H
#define WINTCP_H

extern struct window_procs tcp_procs;

extern void tcp_init_connection();
extern void tcp_set_sockfd(int);
extern int  tcp_get_sockfd();

extern void tcp_lock();
extern void tcp_unlock();

extern void tcp_send_changed_variables();
extern void tcp_send_WIN();
extern void tcp_send_gbuf();
extern void tcp_send_rndencode();
extern void tcp_send_name_command(const char*);
extern void tcp_send_string_to(int, const char*);
extern void tcp_send_string(const char*);
extern void tcp_send_int_to(int, int);
extern void tcp_send_int(int);
extern void tcp_send_boolean(boolean);
extern void tcp_send_xchar(xchar);
extern void tcp_send_char(char);
extern void tcp_send_d_level(d_level);
extern void tcp_send_void();
extern void tcp_send_anything(anything);
extern void tcp_send_long(long);

extern void tcp_send_list_menu_item(menu_item*, int);
extern void tcp_transfer_all(int);
extern int     tcp_recv_string_from(int, char*);
extern int     tcp_recv_string(char*);
extern int     tcp_recv_int();
extern boolean tcp_recv_boolean();
extern xchar   tcp_recv_xchar();
extern char    tcp_recv_char();
extern void    tcp_recv_d_level(d_level*);
extern void    tcp_recv_void();
extern void    tcp_recv_anything(anything*);
extern long    tcp_recv_long();

extern void    tcp_recv_list_menu_item(menu_item**, int);

extern void    tcp_recv_update_variable(char*);

#endif /* WINTCP_H */
