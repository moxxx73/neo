#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>

#include "../include/io.h"
#include "../include/err.h"

void editor(void){
    char key = 0;
    for(;;){
        draw_screen();
        key = get_keypress();
        switch(key){
            case CTRL_KEY('q'):
                exit(0);
            case 27:
                key = get_keypress();
                if(key == 91){
                    key = get_keypress();
                    switch(key){
                        case 68:
                            cursor_left();
                            break;
                        case 67:
                            cursor_right();
                            break;
                        case 66:
                            cursor_down();
                            break;
                        case 65:
                            cursor_up();
                            break;
                        default:
                            break;
                    }
                }
                break;
            case 'i':
                edit_byte();
                break;
            default:
                break;
        }
    }
    return;
}

void cleanup(void){
    if(user_file.fpath) free(user_file.fpath);
    if(user_file.buf) free(user_file.buf);
    memset(&user_file, 0, sizeof(nFile));

    if(screen.sbuf) free(screen.sbuf);
    return;
}

void usage(char *bin){
    printf("Usage: %s <Path/to/file>\n", bin);
    exit(0);
}

int main(int argc, char **argv){

    if(argc < 2) usage(argv[0]);
    if((user_file.fsize = file_size(argv[1])) < 0){
        fprintf(stderr, "file_size(): %s\n", (neo_err ? n_strerror(neo_err) : strerror(errno)));
        return 1;
    }
    user_file.fpath = strdup(argv[1]);
    printf("File: %s (%ld Bytes)\n", user_file.fpath, user_file.fsize);

    atexit(&cleanup);

    if(save_term() < 0){
        fprintf(stderr, "term_save(): %s\n", strerror(errno));
        return 1;
    }
    if(init_term() < 0){
        fprintf(stderr, "init_term(): %s\n", strerror(errno));
        return 1;
    }
    atexit(&restore_term);
    if(read_file() < 0) exit(1);
    editor();

    return 0;
}