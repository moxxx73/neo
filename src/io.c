#include "../include/io.h"
#include "../include/err.h"
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/fcntl.h>
#include <sys/ioctl.h>

struct termios tm_save;
nFile user_file;
n_screen screen;

off_t file_size(char *path){
    struct stat stat_r;
    if(!path){
        neo_err = NEO_ERR_NULL;
        return -1;
    }
    memset(&stat_r, 0, sizeof(struct stat));
    if(stat(path, &stat_r) < 0) return -1;
    return stat_r.st_size;
}

int read_file(void){
    ssize_t bytes = 0;
    int fd = 0;
    if(!user_file.fpath || user_file.fsize <= 0) return -1;
    user_file.buf = (char *)malloc(user_file.fsize+1);
    if(user_file.buf){
        memset(user_file.buf, 0, user_file.fsize);
        fd = open(user_file.fpath, O_RDONLY);
        if(fd >= 0){
            bytes = read(fd, user_file.buf, user_file.fsize);
            close(fd);
            user_file.start = user_file.buf;
            user_file.end = user_file.buf+user_file.fsize;
            if(bytes == user_file.fsize) return 0;
        }
    }
    return -1;
}

int save_term(void){
    memset(&tm_save, 0, sizeof(struct termios));
    return tcgetattr(STDIN_FILENO, &tm_save);
}

int init_term(void){
    struct termios term;
    
    memset(&term, 0, sizeof(struct termios));
    memcpy(&term, &tm_save, sizeof(struct termios));

    term.c_iflag &= ~(IXON | ICRNL);
    term.c_oflag &= ~(OPOST);
    term.c_lflag &= ~(ECHO | ICANON | ISIG | IEXTEN);
    term.c_cc[VMIN] = 1;
    term.c_cc[VTIME] = 0;

    if(tcsetattr(STDIN_FILENO, TCSAFLUSH, &term) < 0) return -1;
    if(write(STDOUT_FILENO, ENABLE_ALTSCRN, sizeof(ENABLE_ALTSCRN)-1) != (sizeof(ENABLE_ALTSCRN)-1)) return -1;
    screen.windows[0].cur_x = 1;
    screen.windows[0].cur_y = 1;
    screen.windows[1].cur_x = 1;
    screen.windows[1].cur_y = 1;
    //write(STDOUT_FILENO, HIDECURSOR, 6);
    return 0;
}

void restore_term(void){
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &tm_save);
    write(STDOUT_FILENO, CLRSCRN, sizeof(CLRSCRN)-1);
    write(STDOUT_FILENO, DISABLE_ALTSCRN, sizeof(DISABLE_ALTSCRN)-1);
    //write(STDOUT_FILENO, SHOWCURSOR, 6);
    return;
}

char get_keypress(void){
    ssize_t r = 0;
    char key = 0;
    while((r = read(STDIN_FILENO, &key, 1)) != 1){
        if(r == -1 && errno != EAGAIN) return -1;
    }
    return key;
}

int get_termsize(int *x, int *y){
    struct winsize wz;
    memset(&wz, 0, sizeof(struct winsize));

    if(!x || !y) return -1;

    if(ioctl(STDOUT_FILENO, TIOCGWINSZ, &wz) == -1 || wz.ws_col == 0) return -1;
    else{
        *x = wz.ws_col;
        *y = wz.ws_row;
        return 0;
    }
    return -1;
}

void set_curpos(int x, int y){
    char buf[256];
    memset(buf, 0, 256);
    snprintf(buf, 254, "\x1b[%d;%dH", y, x);
    write(STDOUT_FILENO, buf, strlen(buf));
    return;
}

#define CURSOR_UP "\x1b[A"
#define CURSOR_DOWN "\x1b[B"
#define CURSOR_LEFT "\x1b[C"
#define CURSOR_RIGHT "\x1b[D"

void cursor_up(){
    if(((screen.windows[0].cur_y-1) > 0) && (user_file.start+user_file.cur_index-((screen.windows[0].x)/3)) >= user_file.buf){
        screen.windows[0].cur_y--;
        user_file.cur_index -= ((screen.windows[0].x)/3);
        write(STDOUT_FILENO, CURSOR_UP, 3);
    }else if(screen.windows[0].cur_y > 0 && (user_file.start+user_file.cur_index-((screen.windows[0].x)/3)) >= user_file.buf){
        user_file.start -= ((screen.windows[0].x)/3);
    }
    return;
}

void cursor_down(){
    if((screen.windows[0].cur_y < screen.windows[0].y)  && (user_file.start+user_file.cur_index+((screen.windows[0].x)/3)) < user_file.end){
        screen.windows[0].cur_y++;
        user_file.cur_index += ((screen.windows[0].x)/3);
        write(STDOUT_FILENO, CURSOR_DOWN, 3);
    }else if(screen.windows[0].cur_y == screen.windows[0].y && (user_file.start+user_file.cur_index+((screen.windows[0].x)/3)) < user_file.end){
        user_file.start += ((screen.windows[0].x)/3);
    }
    return;
}

void cursor_left(){
    if(((screen.windows[0].cur_x-3) > 0) && (user_file.start+(user_file.cur_index)) > user_file.buf){
        screen.windows[0].cur_x -= 3;
        user_file.cur_index--;
        write(STDOUT_FILENO, CURSOR_LEFT, 3);
    }
    return;
}

void cursor_right(){
    if(((screen.windows[0].cur_x+3) < (screen.windows[0].x)) && ((user_file.start+user_file.cur_index+1) < user_file.end)){
        screen.windows[0].cur_x += 3;
        user_file.cur_index++;
        write(STDOUT_FILENO, CURSOR_RIGHT, 3);
    }
    return;
}

void add_string(char *str, int x, int y, size_t len){
    if(!str || !screen.sbuf || len == 0 || (int)len >= screen.x) return;
    memcpy((char *)(screen.sbuf+x+(screen.x*y)), str, len);
    return;
}

void draw_hex(void){
    long index = 0;
    int yp = 0, xp = 0;
    if(!screen.sbuf || !user_file.buf || user_file.fsize <= 0) return;
    for(; yp < screen.windows[0].y; yp++){
        for(xp = 0; xp < (screen.windows[0].x-1); xp+=3){
            if((user_file.start+index) != user_file.end){;
                snprintf((screen.sbuf+xp+(screen.x*yp)), 4, "%02x ", *(unsigned char *)(user_file.start+index));
                index++;
            }else{
                *(screen.sbuf+xp+(screen.x*yp)) = ' ';
                *(screen.sbuf+xp+1+(screen.x*yp)) = ' ';
            }
        }
    }

    return;
}

void draw_border(void){
    int yp = 0;
    if(!screen.sbuf || !user_file.buf || user_file.fsize <= 0) return;
    for(; yp < screen.windows[1].y; yp++){
        *(char *)(screen.sbuf+screen.windows[0].x+(screen.x*yp)) = '|';
    }
    return;
}

void draw_ascii(void){
    long index = 0;
    int yp = 0, xp = 0;
    if(!screen.sbuf || !user_file.buf || user_file.fsize <= 0) return;
    draw_border();
    for(; yp < screen.windows[1].y; yp++){
        for(xp = screen.windows[0].x+2; xp < (screen.x-1); xp++){
            if((user_file.start+index) != user_file.end){
                if(user_file.start[index] < 127 && user_file.start[index] >= 32) *(char *)(screen.sbuf+xp+(screen.x*yp)) = user_file.start[index];
                else *(char *)(screen.sbuf+xp+(screen.x*yp)) = '.';
                index++;
            }else *(char *)(screen.sbuf+xp+(screen.x*yp)) = ' ';
        }
    }
    return;
}

void draw_screen(void){
    int xp = 0, yp = 0;
    char fmt_buf[256];
    long sbuf_size = 0;

    memset(fmt_buf, 0, 256);
    write(STDOUT_FILENO, CLRSCRN, sizeof(CLRSCRN)-1);
    set_curpos(0, 1);
    screen.last_x = screen.x;
    screen.last_y = screen.y;
    get_termsize(&screen.x, &screen.y);
    screen.windows[0].x = screen.x;
    screen.windows[0].y = screen.y-2;

    sbuf_size = (screen.x*screen.y)+1;
    if(sbuf_size != screen.sbuf_size){
        if(!(screen.sbuf = (char *)realloc(screen.sbuf, sbuf_size))) exit(1);
        screen.sbuf_size = sbuf_size;
        memset(screen.sbuf, 0, screen.sbuf_size);
        memset(screen.sbuf, 0x20, screen.sbuf_size-1);
    }
    if(screen.sbuf){

        draw_hex();

        snprintf(fmt_buf, 254, 
            "%s: %ld Bytes | +0x%08lx: %02x | %d:%d ", 
            user_file.fpath, 
            user_file.fsize, 
            ((user_file.start+user_file.cur_index)-user_file.buf), 
            *(unsigned char *)(user_file.start+user_file.cur_index),
            screen.windows[0].cur_x,
            screen.windows[0].cur_y
        );
        add_string(fmt_buf, 0, (screen.y-2), strlen(fmt_buf));
        memset((char *)(screen.sbuf+strlen(fmt_buf)+((screen.y-2)*screen.x)), ' ', screen.x-strlen(fmt_buf));
        add_string(SAC, screen.x-strlen(SAC)-1, (screen.y-2), strlen(SAC));

        for(yp = 0; yp < (screen.y-1); yp++){
            if(yp == (screen.y-2)) write(STDOUT_FILENO, "\x1b[7m", 4);
            write(STDOUT_FILENO, (char *)(screen.sbuf+(screen.x*yp)), (screen.x));
            write(STDOUT_FILENO, "\r\n", 2);
            if(yp == (screen.y-2)) write(STDOUT_FILENO, "\x1b[0m", 4);
        }
    }

    set_curpos(screen.windows[0].cur_x, screen.windows[0].cur_y);

    return;
}

void edit_byte(void){
    long nibble = 0;
    char byte = 0;
    char r[2] = {0, 0};
    if(!screen.sbuf || !user_file.start || (user_file.start+user_file.cur_index) >= user_file.end) return;
    
    r[0] = get_keypress();
    if((r[0] >= 'a' && r[0] <= 'f') || (r[0] >= 'A' && r[0] <= 'F') || (r[0] >= '0' && r[0] <= '9')){
        nibble = strtoul(r, NULL, 16);
        byte = (nibble&0x0f)<<4;
        r[0] = get_keypress();
        if((r[0] >= 'a' && r[0] <= 'f') || (r[0] >= 'A' && r[0] <= 'F') || (r[0] >= '0' && r[0] <= '9')){

            nibble = strtoul(r, NULL, 16);
            byte |= (nibble&0xf);
            *(unsigned char *)(user_file.start+user_file.cur_index) = (unsigned char)(byte&0xff);
        }
    }
    return;
}