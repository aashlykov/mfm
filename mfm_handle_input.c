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

//Mark the menu element
void mfm_mark(mfm_tab* tab)
{
    //Current active element
    int act = tab->act;

    //Can't mark the ".." directory
    if (!act) {
        return;
    }

    //Mark or unmark the element
    if (tab->items[act]->props & MFM_SEL) {
        tab->items[act]->props &= ~MFM_SEL;
    } else {
        tab->items[act]->props |= MFM_SEL;
    }

    //Go to next element
    if (tab->items[act + 1]) {
        tab->act++;
    }
}

//Run shell
void mfm_run_shell(mfm_state* st)
{
    //Current menu
    mfm_tab* tab = st->tabs[st->cur];

    //Current position in menu
    int act = tab->act;

    //Buffer for various purposes
    void* tmp;

    //Total lenfth of string
    int sum = 0;

    //Set files to env vars
    for (int i = 0; tab->items[i]; i++) { //Count needed memory
        if (tab->items[i]->props & MFM_SEL) {
            sum += strlen(tab->items[i]->text) + 1;
        }
    }
    //Put data to the memory
    if (sum) {
        tmp = malloc(sum);
        sum = 0;
        for (int i = 0; tab->items[i]; i++) {
            if (tab->items[i]->props & MFM_SEL) {
                strcpy(tmp+sum, tab->items[i]->text);
                sum += strlen(tab->items[i]->text);
                ((char*)tmp)[sum] = '/';
                sum++;
            }
        }
        sum--;
        strcpy(tmp + sum, "\0");
        setenv("FF", tmp, 1);
        free(tmp);
    } else {
        setenv("FF", tab->items[act]->text, 1);
    }

    //Run shell
    mfm_command(getenv("SHELL"));

    //Unset env. var
    setenv("FF", "", 1);
}

//Action on file:
// - enter to the directory
// - apply action to the file
// - execute executable
void mfm_action(mfm_tab* tab, void** f_cmd)
{
    int act = tab->act;

    //Go to the dir
    if (tab->items[act]->props & MFM_DIR) {
        int res = chdir(tab->items[act]->text);
        mfm_init_tab(tab, f_cmd);
        return;
    }

    //Apply command to the file of this type
    char*** comm = NULL;
    for (int i = 0; tab->items[i] && !comm; i++) {
        if (tab->items[i]->props & MFM_SEL) {
            comm = tfind(tab->items[i], f_cmd, mfm_get_kv);
        }
    }

    if (!comm) {
        comm = tfind(tab->items[act]->text, f_cmd, mfm_get_kv);
    }

    if (comm) {
        char* full_command = mfm_substitute(tab, (*comm)[1]);
        mfm_command(full_command);
        free(full_command);
        return;
    }

    //If file is executable - run it
    if (tab->items[act]->props & MFM_EXE) {
        void* tmp =
            malloc(strlen(tab->items[act]->text)+3);
        sprintf(tmp, "./%s", tab->items[act]->text);
        mfm_command(tmp);
        free(tmp);
    }
}

//Rename the file
void mfm_rename(mfm_tab* tab, int h, int w)
{
    int act = tab->act;
    if (!act) return;
    printf("\e[%i;1H\e[37;41m\e[2K", h);
    char* new_name = mfm_read_line(
        h,
        1,
        w,
        tab->items[act]->text);
    if (!new_name) {
        return;
    }
    rename(tab->items[act]->text, new_name);
    free(new_name);
}

//Delete selected or under-cursor file or directory
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
    for (int i = 0; tab->items[i]; i++) {
        mfm_tab_item* it = tab->items[i];
        if (it->props & MFM_SEL) {
            if (stat(it->text, &st) != -1) {
                mfm_delete_item(it->text, &st, NULL);
            }
            ex = 1;
        }
    }

    //If no files selected - delete the current
    if (!ex) {
        if (stat(tab->items[act]->text, &st) != -1) {
            mfm_delete_item(tab->items[act]->text, &st, NULL);
        }
    }
}

//Create the new tab
void mfm_new_tab(mfm_state* st)
{
    int i = 0;
    while (st->tabs[i]) i++;
    if (i == 10) {
        return;
    }
    st->tabs = realloc(
        st->tabs,
        sizeof(mfm_tab*)*(i+2)
    );
    st->tabs[i + 1] = NULL;
    st->tabs[i] = calloc(sizeof(mfm_tab), 1);
    mfm_init_tab(st->tabs[i], &(st->f_cmd));
    st->cur = i;
}

//Close current tab
int mfm_close_tab(mfm_state* st)
{
    int i = st->cur;
    mfm_destroy_tab(st->tabs[i]);
    free(st->tabs[i]);
    while (st->tabs[i]) {
        st->tabs[i] = st->tabs[i+1];
        i++;
    }
    if (!(st->tabs[0])) {
        return 1;
    }
    i = 0;
    while (st->tabs[i]) {
        i++;
    }
    st->tabs = realloc(
        st->tabs,
        sizeof(mfm_tab*)*(i + 1));
    if (st->cur > i - 1) {
        st->cur = i - 1;
    }
    return 0;
}

//Execute single command
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

//Handle the special key
int mfm_handle_key(
    mfm_key key,
    mfm_state* st,
    int h,
    int w
);

//Handle user input
int mfm_handle_input(
    mfm_state* st,
    int h,
    int w
) {
    //Current menu
    mfm_tab* tab = st->tabs[st->cur];

    //Current position in menu
    int act = tab->act;

    //Get user input
    char buf[8];
    mfm_key key;
    mfm_read_key(buf, &key);

    //Is it necessary to reinit tab
    int ri = 0;
    //And to save active position
    int sa = 0;

    switch (buf[0]) {
    case '=': case '+':
        for (int i = 1; tab->items[i]; i++) {
            tab->items[i]->props |= MFM_SEL;
        }
        break;
    case '-': case '_':
        for (int i = 1; tab->items[i]; i++) {
            tab->items[i]->props &= ~MFM_SEL;
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
        printf("\e[%i;1H\e[41m\e[2K", h);
        char* new_dir = mfm_read_line(h, 1, w, NULL);
        mkdir(new_dir,
        S_IRUSR | S_IWUSR | S_IXUSR |
        S_IRGRP | S_IXGRP |
        S_IROTH | S_IXOTH);
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
    case 'c': case 'C':
        mfm_copy(st);
        ri = 1;
        break;
    case 'm': case 'M':
        mfm_move(st);
        ri = 1;
        break;
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

//Handle the special key
int mfm_handle_key(
    mfm_key key,
    mfm_state* st,
    int h,
    int w
) {
    mfm_tab* tab = st->tabs[st->cur];
    int ri = 0;
    int act = tab->act;
    switch (key) {
    case MFM_KEY_UP:
        if (tab->act) {
            tab->act--;
        }
        break;
    case MFM_KEY_DOWN:
        if (!tab->items[act+1]) {
            break;
        }
        tab->act++;
        break;
    case MFM_KEY_LEFT:
        if (st->cur) {
            st->cur--;
        }
        break;
    case MFM_KEY_RIGHT:
        if (st->tabs[st->cur+1]) {
            st->cur++;
        }
        break;
    case MFM_KEY_PGUP:
        tab->act -= 5;
        if (tab->act < 0) {
            tab->act = 0;
        }
        break;
    case MFM_KEY_PGDN:
        for(
            int i = 0;
            i < 6 && tab->items[++act];
            i++
        );
        tab->act = act - 1;
        break;
    case MFM_KEY_HOME:
        tab->act = 0;
        break;
    case MFM_KEY_END:
        for (; tab->items[++act];);
        tab->act = act - 1;
        break;
    case MFM_KEY_INSERT:
        mfm_mark(tab);
        break;
    case MFM_KEY_DELETE:
        mfm_delete(tab, h, w);
        ri = 1;
        break;
    case MFM_KEY_ESC:
        return mfm_close_tab(st);
    }
    if (ri) {
        mfm_init_tab(tab, &(st->f_cmd));
    }
    return 0;
}
