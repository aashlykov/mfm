#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <error.h>
#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>

#include "mfm_input.h"
#include "mfm_general.h"

/**
 * Key and his code
 */
typedef struct
{
    unsigned char code[8];
    mfm_key key;
} mfm_key_map_element;

/**
 * Map for all keys
 */
mfm_key_map_element mfm_key_map[] = {
    //Xterm key codes
    {
        {27, 0, 0, 0, 0, 0, 0, 0},
        MFM_KEY_ESC
    }, {
        {27, 79, 80, 0, 0, 0, 0, 0},
        MFM_KEY_F1
    }, {
        {27, 79, 81, 0, 0, 0, 0, 0},
        MFM_KEY_F2
    }, {
        {27, 79, 82, 0, 0, 0, 0, 0},
        MFM_KEY_F3
    }, {
        {27, 79, 83, 0, 0, 0, 0, 0},
        MFM_KEY_F4
    }, {
        {27, 91, 49, 53, 126, 0, 0, 0},
        MFM_KEY_F5
    }, {
        {27, 91, 49, 55, 126, 0, 0, 0},
        MFM_KEY_F6
    }, {
        {27, 91, 49, 56, 126, 0, 0, 0},
        MFM_KEY_F7
    }, {
        {27, 91, 49, 57, 126, 0, 0, 0},
        MFM_KEY_F8
    }, {
        {27, 91, 50, 48, 126, 0, 0, 0},
        MFM_KEY_F9
    }, {
        {27, 91, 50, 49, 126, 0, 0, 0},
        MFM_KEY_F10
    }, {
        {27, 91, 50, 51, 126, 0, 0, 0},
        MFM_KEY_F11
    }, {
        {27, 91, 50, 52, 126, 0, 0, 0},
        MFM_KEY_F12
    }, {
        {27, 91, 65, 0, 0, 0, 0, 0},
        MFM_KEY_UP
    }, {
        {27, 91, 66, 0, 0, 0, 0, 0},
        MFM_KEY_DOWN
    }, {
        {27, 91, 68, 0, 0, 0, 0, 0},
        MFM_KEY_LEFT
    }, {
        {27, 91, 67, 0, 0, 0, 0, 0},
        MFM_KEY_RIGHT
    }, {
        {27, 91, 50, 126, 0, 0, 0, 0},
        MFM_KEY_INSERT
    }, {
        {27, 91, 51, 126, 0, 0, 0, 0},
        MFM_KEY_DELETE
    }, {
        {27, 91, 55, 126, 0, 0, 0, 0},
        MFM_KEY_HOME
    }, {
        {27, 91, 56, 126, 0, 0, 0, 0},
        MFM_KEY_END
    }, {
        {27, 91, 53, 126, 0, 0, 0, 0},
        MFM_KEY_PGUP
    }, {
        {27, 91, 54, 126, 0, 0, 0, 0},
        MFM_KEY_PGDN
    },
    //Linux console
    {
        {27, 91, 91, 65, 0, 0, 0, 0},
        MFM_KEY_F1
    }, {
        {27, 91, 91, 66, 0, 0, 0, 0},
        MFM_KEY_F2
    }, {
        {27, 91, 91, 67, 0, 0, 0, 0},
        MFM_KEY_F3
    }, {
        {27, 91, 91, 68, 0, 0, 0, 0},
        MFM_KEY_F4
    }, {
        {27, 91, 91, 69, 0, 0, 0, 0},
        MFM_KEY_F5
    }, {
        {27, 91, 49, 126, 0, 0, 0, 0},
        MFM_KEY_HOME
    }, {
        {27, 91, 52, 126, 0, 0, 0, 0},
        MFM_KEY_END
    }
};

/**
 * Read key
 * @param buf Place to put raw key code
 * @param key Place to put apecial key flag
 */
void mfm_read_key(char* buf, mfm_key* key)
{
    memset(buf, '\0', 8);
    *key = MFM_KEY_NULL;

    //Let's go
    fflush(stdout);
    ssize_t res = read(0, buf, 8);
    mfm_drain_input();

    //Return result if it's not a code of special key
    if (buf[0] != 27) {
        return;
    }

    //Clear escape sequence
    int f = 0;
    for (int i = 1; i <= 8; i++) {
        if (buf[i] == '\e') {
            f = 1;
        }
        if (f) {
            buf[i] = '\0';
        }
    }


    //Try to detect special key
    for (int i = 0; i < 30; i++) {
        if (!memcmp(buf, mfm_key_map[i].code, 8)) {
            *key = mfm_key_map[i].key;
        }
    }
}

/**
 * Read existing utf-8 string
 * @param exist String to be parsed
 * @param str   Place to put results
 * @param w     Available length
 */
void mfm_read_exist(
    unsigned char* exist,
    char** str,
    int w
);

int mfm_read_line_sp_keys(mfm_key key, char** str, int* cur, int* curr);

/**
 * Read the line
 * @param y Position on screen
 * @param x
 * @param w Available length
 * @param exist Previous string
 * @return
 */
char* mfm_read_line(
    int y,
    int x,
    int w,
    unsigned char* exist
) {
    //Buffer for multibyte keys
    unsigned char buf[8];

    //Special key
    mfm_key key;

    //Cursor in the string
    int cur = 0;

    //Buffer for current string
    char** str = calloc(sizeof(char*), w);
    mfm_read_exist(exist, str, w);

    //Temporary cursor
    int curr;

    printf("%s", "\e[?25h");

    //Main working cycle
    for (;;) {
        //Position to draw
        printf("\e[%i;%iH", y, x);

        //Draw the current string
        for (int i = 0; i < w; i++) {
            if (str[i]) {
                printf("%s", str[i]);
            } else {
                putchar(' ');
            }
        }
        printf("\e[%i;%iH", y, x + cur);

        //Read the key
        mfm_read_key(buf, &key);

        //Special keys
        if (buf[0] == 27) {
            if (mfm_read_line_sp_keys(key, str, &cur, &curr)) {
                return NULL;
            }
        //Input is over
        } else if (buf[0] == 13) {
            break;
        }
        //Backspace action
        else if (buf[0] == 127 && cur) {
            cur--;
            free(str[cur]);
            curr = cur;
            while (str[curr]) {
                str[curr] = str[curr + 1];
                curr++;
            }
        }
        //Write current symbol
        else if (cur < w - 1 && buf[0] > 31 && buf[0] != 127) {
            //move symbols after cursor
            curr = w - 1;
            while (curr > cur) {
                str[curr] = str[curr - 1];
                curr--;
            }
            free(str[w - 1]);
            str[w - 1] = NULL;
            str[cur] = malloc(8);
            memcpy(str[cur], buf, 8);
            cur++;
        }
    }

    printf("%s", "\e[?25l");

    //Count the total length
    int len = 0;
    for (char** st = str; *st; st++) {
        len += strlen(*st);
    }
    char* res = malloc(len + 1);
    //Copy data to the result string
    len = 0;
    for (char** st = str; *st; st++) {
        strcpy(res + len, *st);
        len += strlen(*st);
        free(*st);
    }
    free(str);
    return res;
}

/**
 * Read existing utf-8 string
 * @param exist String to be parsed
 * @param str   Place to put results
 * @param w     Available length
 */
void mfm_read_exist(
    unsigned char* exist,
    char** str,
    int w
) {
    if (!exist) {
        return;
    }
    int cur = 0;
    for (int i = 0; exist[i] && cur < w - 1; i++) {
        str[cur] = calloc(1, 8);
        if (exist[i] < 128) {
            str[cur++][0] = exist[i];
        } else {
            char s = exist[i];
            int ln = 0;
            while (s & 128) {
                ln++;
                s <<= 1;
            }
            memcpy(str[cur++], exist + i, ln);
            i += ln - 1;
        }
    }
}

/**
 * Handle special key pressing while input line
 * @param key
 * @param str
 * @param cur
 * @param curr
 * @return
 */
int mfm_read_line_sp_keys(mfm_key key, char** str, int* cur, int* curr)
{
    switch (key) {
    case MFM_KEY_ESC:
        for (char** st = str; *st; st++) {
            free(*st);
        }
        free(str);
        printf("%s", "\e[?25l");
        return 1;
    case MFM_KEY_LEFT:
        if (*cur) {
            (*cur)--;
        }
        break;
    case MFM_KEY_RIGHT:
        if (str[*cur]) {
            (*cur)++;
        }
        break;
    case MFM_KEY_DELETE:
        free(str[*cur]);
        *curr = *cur;
        while (str[*curr]){
            str[*curr] = str[*curr + 1];
            (*curr)++;
        }
        break;
    }
    return 0;
}
