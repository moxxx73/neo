#ifndef NEO_STDIO_H
#define NEO_STDIO_H

#include <stdio.h>
#include <stdint.h>
#include <termios.h>
#include <sys/stat.h>
#include <sys/types.h>

#define SAC "~~(8:>"

extern struct termios tm_save;

typedef struct neo_file{
    char *fpath;
    char *buf;
    off_t fsize;
    int64_t cur_index;
    char *start;
    char *end;
}nFile;

extern nFile user_file;

typedef struct _neo_window{
    int x;
    int y;
    int cur_x;
    int cur_y;
    int x_save;
    int y_save;
}neo_window;

typedef struct neo_screen{
    int last_x;
    int last_y;
    int x;
    int y;
    int64_t sbuf_size;
    char *sbuf;
    neo_window windows[2];
}n_screen;

extern n_screen screen;

off_t file_size(char *path);

int read_file(void);

int save_term(void);

#define ENABLE_ALTSCRN "\x1b[?1049h\0"
#define DISABLE_ALTSCRN "\x1b[?1049l\0"

int init_term(void);

void restore_term(void);

#define CTRL_KEY(k) ((k)&0x1f)

char get_keypress(void);

#define CLRSCRN "\x1b[2J\0"
#define HIDECURSOR "\x1b[?25l"
#define SHOWCURSOR "\x1b[?25h"

int get_termsize(int *x, int *y);

void set_curpos(int x, int y);

void cursor_up(void);

void cursor_down(void);

void cursor_left(void);

void cursor_right(void);

void draw_screen(void);

void edit_byte(void);

#endif