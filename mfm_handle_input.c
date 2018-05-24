#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <search.h>
#include <errno.h>
#include <error.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>

#include "mfm_general.h"
#include "mfm_handle_input.h"
#include "mfm_input.h"
#include "mfm_copy.h"
#include "mfm_goto.h"

int mfm_handle_key(
    mfm_key key,
    mfm_state* st,
    int h,
    int w
);
void mfm_mark(mfm_tab* tab);
void mfm_run_shell(mfm_state* st);
void mfm_action(mfm_tab* tab, void** f_cmd);
void mfm_rename(mfm_tab* tab, int h, int w);
void mfm_delete(mfm_tab* tab, int h, int w);
void mfm_new_tab(mfm_state* st);
int mfm_jump(mfm_tab* tab, int dir, int h, int w);
int mfm_close_tab(mfm_state* st);
void mfm_exec_single(
    mfm_tab* tab,
    int h,
    int w,
    int delay
);

/**
 * Handle user input
 * @param st
 * @param h
 * @param w
 * @return
 */
int mfm_handle_input(
    mfm_state* st
) {
    //Current menu
    mfm_tab* tab = st->tabs + st->cur;

    //Current position in menu
    int act = tab->act;

    //Get user input
    char buf[8];
    mfm_key key;
    mfm_read_key(buf, &key);

    //Get the screen size
    int h, w;
    mfm_scr_size(&h, &w);

    //Is it necessary to reinit tab
    int ri = 0;
    //And to save active position
    int sa = 0;

    switch (buf[0]) {
    case '=': case '+':
        for (int i = 1; i < tab->len; i++) {
            tab->items[i].props |= MFM_SEL;
        }
        break;
    case '-': case '_':
        for (int i = 1; i < tab->len; i++) {
            tab->items[i].props &= ~MFM_SEL;
        }
        break;
    case ' ':
        mfm_mark(tab);
        break;
    case 13:
        mfm_action(tab, &(st->f_cmd));
        break;
    case 'q': case 'Q': //Exit from program
        return 1;
    case 's': case 'S': //Go to the shell
        mfm_run_shell(st);
        ri = 1;
        break;
    case 'r': case 'R': //Rename the file
        mfm_rename(tab, h, w);
        ri = 1;
        sa = 1;
        break;
    case '\t':          //Create new tab
        mfm_new_tab(st);
        break;
    case 'e': case 'E': //Execute single command from user
        mfm_exec_single(tab, h, w, 1);
        ri = 1;
        break;
    case 5:
        mfm_exec_single(tab, h, w, 0);
        ri = 1;
        break;
    case 'u': case 'U':
        ri = 1;
        break;
    case 'd': case 'D': {
        printf("\e[%i;1H\e[37;41m\e[2K", h);
        char* new_dir = mfm_read_line(h, 1, w, NULL);
        mfm_mkdir(new_dir);
        free(new_dir);
        ri = 1;
        break;
    }
    case 'h': case 'H':
        tab->hidden = !tab->hidden;
        ri = 1;
        break;
    case 'g': case 'G':
        mfm_goto(st->bookmarks);
        ri = 1;
        break;
    case 7:
        mfm_input_goto(h, w);
        ri = 1;
        break;
    case 'j': case 'J':
        tab->act = mfm_jump(tab, 1, h, w);
        break;
    case 10:
        tab->act = mfm_jump(tab, 0, h, w);
        break;
    case 'c': case 'C':
        mfm_copy(st);
        ri = 1;
        break;
    case 'm': case 'M':
        mfm_move(st);
        ri = 1;
        break;
    case 23:
        return mfm_close_tab(st);
    case 27:
        return mfm_handle_key(key, st, h, w);
    }
    if (ri) {
        mfm_init_tab(tab, &(st->f_cmd));
        if (sa) {
            tab->act = act;
        }
    }
    return 0;
}

/**
 * Handle the special key
 * @param key
 * @param st
 * @param h
 * @param w
 * @return
 */
int mfm_handle_key(
    mfm_key key,
    mfm_state* st,
    int h,
    int w
) {
    mfm_tab* tab = st->tabs + st->cur;
    int ri = 0;
    int act = tab->act;
    switch (key) {
    case MFM_KEY_UP:
        tab->act--;
        break;
    case MFM_KEY_DOWN:
        tab->act++;
        break;
    case MFM_KEY_LEFT:
        if (st->cur) {
            st->cur--;
        }
        break;
    case MFM_KEY_RIGHT:
        if (st->cur + 1 < st->len) {
            st->cur++;
        }
        break;
    case MFM_KEY_PGUP:
        tab->act -= 5;
        break;
    case MFM_KEY_PGDN:
        tab->act += 5;
        break;
    case MFM_KEY_HOME:
        tab->act = 0;
        break;
    case MFM_KEY_END:
        tab->act = tab->len - 1;
        break;
    case MFM_KEY_INSERT:
        mfm_mark(tab);
        break;
    case MFM_KEY_DELETE:
        mfm_delete(tab, h, w);
        ri = 1;
        break;
    }
    if (ri) {
        mfm_init_tab(tab, &(st->f_cmd));
    }
    return 0;
}

/**
 * Mark the menu element
 * @param tab
 */
void mfm_mark(mfm_tab* tab)
{
    //Current active element
    int act = tab->act;

    //Can't mark the ".." directory
    if (!act) {
        return;
    }

    //Mark or unmark the element
    if (tab->items[act].props & MFM_SEL) {
        tab->items[act].props &= ~MFM_SEL;
    } else {
        tab->items[act].props |= MFM_SEL;
    }

    //Go to next element
    tab->act++;
}

/**
 * Run shell
 * @param st
 */
void mfm_run_shell(mfm_state* st)
{
    //Current menu
    mfm_tab* tab = st->tabs + st->cur;

    //Current position in menu
    int act = tab->act;

    //Buffer for various purposes
    void* tmp;

    //Total lenfth of string
    int sum = 0;

    //Set files to env vars
    for (int i = 0; i < tab->len; i++) { //Count needed memory
        if (tab->items[i].props & MFM_SEL) {
            sum += strlen(tab->items[i].text) + 1;
        }
    }

    //Put data to the memory
    if (sum) {
        tmp = malloc(sum);
        sum = 0;
        for (int i = 0; i < tab->len; i++) {
            if (tab->items[i].props & MFM_SEL) {
                strcpy(tmp + sum, tab->items[i].text);
                sum += strlen(tab->items[i].text);
                ((char*)tmp)[sum] = '/';
                sum++;
            }
        }
        sum--;
        strcpy(tmp + sum, "\0");
        setenv("FF", tmp, 1);
        free(tmp);
    } else {
        setenv("FF", tab->items[act].text, 1);
    }

    //Run shell
    mfm_command(getenv("SHELL"));

    //Unset env. var
    setenv("FF", "", 1);
}

/**
 * Action on file:
 *  - enter to the directory
 *  - apply action to the file
 *  - execute executable
 * @param tab
 * @param f_cmd
 */
void mfm_action(mfm_tab* tab, void** f_cmd)
{
    int act = tab->act;

    //Go to the dir
    if (tab->items[act].props & MFM_DIR) {
        int res = chdir(tab->items[act].text);
        mfm_init_tab(tab, f_cmd);
        return;
    }

    //Apply command to the file of this type
    char*** comm = NULL;
    for (int i = 0; i < tab->len && !comm; i++) {
        if (tab->items[i].props & MFM_SEL) {
            comm = tfind(tab->items + i, f_cmd, mfm_get_kv);
        }
    }

    if (!comm) {
        comm = tfind(tab->items[act].text, f_cmd, mfm_get_kv);
    }

    if (comm) {
        char* full_command = mfm_substitute(tab, (*comm)[1]);
        mfm_command(full_command);
        free(full_command);
        return;
    }

    //If file is executable - run it
    if (tab->items[act].props & MFM_EXE) {
        void* tmp =
            malloc(strlen(tab->items[act].text) + 3);
        sprintf(tmp, "./%s", tab->items[act].text);
        mfm_command(tmp);
        free(tmp);
    }
}

/**
 * Rename the file
 * @param tab
 * @param h
 * @param w
 */
void mfm_rename(mfm_tab* tab, int h, int w)
{
    int act = tab->act;
    if (!act) {
        return;
    }
    printf("\e[%i;1H\e[37;41m\e[2K", h);
    char* new_name = mfm_read_line(
        h,
        1,
        w,
        tab->items[act].text);
    if (!new_name) {
        return;
    }
    rename(tab->items[act].text, new_name);
    free(new_name);
}

/**
 * Delete selected or under-cursor file or directory
 * @param tab
 * @param h
 * @param w
 */
void mfm_delete(mfm_tab* tab, int h, int w)
{
    int act = tab->act;
    char buf[8];
    mfm_key key;
    int ex;

    //Ask user
    for (;;) {
        printf("\e[%i;1H\e[37;41m\e[2K", h);
        printf("Are you sure? Y/N");
        mfm_read_key(buf, &key);
        ex = 0;
        switch (*buf) {
        case 'y': case 'Y':
            ex = 2;
            break;
        case 'n': case 'N':
            ex = 1;
            break;
        case '\e':
            ex = !buf[1];
            break;
        }
        if (ex) {
            break;
        }
    }
    if (ex < 2) {
        return;
    }

    //If user confirmed - delete selected files
    ex = 0;
    struct stat st;
    for (int i = 0; i < tab->len; i++) {
        mfm_tab_item it = tab->items[i];
        if (it.props & MFM_SEL) {
            if (stat(it.text, &st) != -1) {
                mfm_delete_item(it.text, &st, NULL);
            }
            ex = 1;
        }
    }

    //If no files selected - delete the current
    if (!ex) {
        if (stat(tab->items[act].text, &st) != -1) {
            mfm_delete_item(tab->items[act].text, &st, NULL);
        }
    }
}

/**
 * Create the new tab
 * @param st
 */
void mfm_new_tab(mfm_state* st)
{
    if (st->len == 10) {
        return;
    }
    char tmp[200];
    st->len++;
    st->tabs = realloc(
        st->tabs,
        sizeof(mfm_tab) * (st->len)
    );
    memset(st->tabs + st->len - 1, 0, sizeof(mfm_tab));
    mfm_init_tab(st->tabs + st->len - 1, &(st->f_cmd));
    st->cur = st->len - 1;
}

/**
 * Close current tab
 * @param st
 * @return
 */
int mfm_close_tab(mfm_state* st)
{
    int i = st->cur;
    mfm_destroy_tab(st->tabs + st->cur);
    st->len--;
    while (i < st->len) {
        st->tabs[i] = st->tabs[i+1];
        i++;
    }
    if (!(st->len)) {
        return 1;
    }
    st->tabs = realloc(
        st->tabs,
        sizeof(mfm_tab) * st->len);
    if (st->cur >= st->len) {
        st->cur = st->len - 1;
    }
    return 0;
}

/**
 * Execute single command
 * @param tab
 * @param h
 * @param w
 * @param delay
 */
void mfm_exec_single(
    mfm_tab* tab,
    int h,
    int w,
    int delay
) {
    printf("\e[%i;1H\e[37;41m\e[2K", h);
    char* user_input = mfm_read_line(h, 1, w, NULL);
    if (!user_input) {
        return;
    }
    char* full_command = mfm_substitute(tab, user_input);
    mfm_command(full_command);
    free(user_input);
    free(full_command);
    if (delay) {
        char buf[8];
        mfm_key key;
        mfm_read_key(buf, &key);
    }
}

/**
 * Jump to the file or directory
 * @param tab
 * @param prop
 * @param h
 * @param w
 * @return finded position
 */
int mfm_jump(mfm_tab* tab, int dir, int h, int w)
{
    printf("\e[%i;1H\e[33;41m\e[1K", h);
    char* s = mfm_read_line(h, 1, w, NULL);
    if (!s) {
        return tab->act;
    }
    int ls = strlen(s);

    int res = -1;
    dir = !!dir;
    for (int i = 0; i < tab->len; i++) {
        if (
            strncmp(s, tab->items[i].text, ls) == 0 &&
            dir == !!(tab->items[i].props & MFM_DIR)
        ) {
            res = i;
            break;
        }
    }

    free(s);
    return res;
}
