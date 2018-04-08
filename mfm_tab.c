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
#include <grp.h>
#include <pwd.h>

#include "mfm_general.h"
#include "mfm_tab.h"

/**
 * Is file executable?
 * @param fname Target file
 * @param st    Global state
 * @param f_cmd Commands for file types
 * @return
 */
mfm_tab_item_props mfm_is_exec(
    char* fname,
    struct stat* st,
    void** f_cmd
) {
    //If action for file type exist - non executable
    if (tfind(fname, f_cmd, mfm_get_kv)) {
        return 0;
    }

    //1. User have rights to execute
    uid_t uid = geteuid();
    if (uid == st->st_uid) {
        return st->st_mode & S_IEXEC ? MFM_EXE : 0;
    }

    //2. Group have rights to execute
    gid_t gid = getpwnam(getenv("USER"))->pw_gid;
    int gids_total = 10;
    gid_t* gids = malloc(gids_total*sizeof(gid_t));
    if (!gids) {
        return 0;
    }
    if (getgrouplist(getenv("USER"), gid, gids, &gids_total) == -1) {
        return 0;
    }
    if (gids_total != 10) {
        gids = realloc(gids, gids_total*sizeof(gid_t));
        getgrouplist(getenv("USER"), gid, gids, &gids_total);
    }
    for (int i = 0; i < gids_total; i++) {
        if (gids[i] == st->st_gid ) {
            free(gids);
            return st->st_mode & S_IXGRP ? MFM_EXE : 0;
        }
    }

    //3. Others have rights to execute
    if (st->st_mode & S_IXOTH) {
        return MFM_EXE;
    }

    //Finally
    return 0;
}

/**
 * Comparation function for sorting entries in directory
 * @param one
 * @param two
 * @return
 */
int mfm_cmp_tab_items(const void* one, const void* two)
{
    printf("%s\n", "2");
    mfm_tab_item_props p_one, p_two;
    p_one = ((mfm_tab_item*)one)->props;
    p_one &= MFM_DIR;
    p_two = ((mfm_tab_item*)two)->props;
    p_two &= MFM_DIR;
    if (p_one == MFM_DIR && p_two != MFM_DIR) {
        return -1;
    }
    if (p_one != MFM_DIR && p_two == MFM_DIR) {
        return 1;
    }
    return strcmp(
        ((mfm_tab_item*)one)->text,
        ((mfm_tab_item*)two)->text
    );
}

/**
 * Destroy the tab
 * @param tab
 */
void mfm_destroy_tab(mfm_tab* tab)
{
    if (!tab) {
        return;
    }
    free(tab->dir);
    int i = 0;
    mfm_tab_item* it;
    if (tab->items) {
        while (i < tab->len) {
            free(tab->items[i].text);
            i++;
        }
    }
    free(tab->items);
}

/**
 * User data for directory traverse callback
 */
typedef struct
{
    mfm_tab* tab;
    int temp_count;
    void** f_cmd;
} mfm_init_tab_item_udata;

int mfm_init_tab_item(char* name, struct stat* st, void* udata);

//Create the tab
int mfm_init_tab(mfm_tab* tab, void** f_cmd)
{
    //Clear this menu
    mfm_destroy_tab(tab);

    //Set current directory of menu
    tab->dir = malloc(256);
    tab->dir = getcwd(tab->dir, 256);
    tab->dir = realloc(tab->dir, strlen(tab->dir) + 1);

    //Set active element and position of view
    tab->act = 0;
    tab->pos_view = 0;

    //Add items from directory to menu
    int temp_count = 20;
    tab->items = calloc(sizeof(mfm_tab_item), temp_count);

    //Struct for info about file
    struct stat f_info;
    tab->items[0].props = MFM_DIR;
    tab->items[0].text = malloc(3);
    tab->items[0].text[0] = '.';
    tab->items[0].text[1] = '.';
    tab->items[0].text[2] = '\0';
    tab->len = 1;

    //Traverse directory
    mfm_init_tab_item_udata udata;
    udata.tab = tab;
    udata.temp_count = temp_count;
    udata.f_cmd = f_cmd;
    mfm_traverse_directory(
        ".",
        mfm_init_tab_item,
        (void*)&udata
    );

    //Resize the items array
    tab->items = realloc(
        tab->items,
        sizeof(mfm_tab_item) * (tab->len + 1)
    );

    //Sort list (directories in the beginning)
    qsort(
        tab->items,
        tab->len,
        sizeof(mfm_tab_item),
        mfm_cmp_tab_items
    );

    //Finish work
    return 0;
}

/**
 * Callback for directory traversing
 * @param name
 * @param st
 * @param udata
 * @return
 */
int mfm_init_tab_item(char* name, struct stat* st, void* udata)
{
    mfm_tab* tab = (*(mfm_init_tab_item_udata*)udata).tab;
    int temp_count = (*(mfm_init_tab_item_udata*)udata).temp_count;
    name += 2;
    void** f_cmd = (*(mfm_init_tab_item_udata*)udata).f_cmd;

    if (mfm_dots_dir(name)) {
        return 0;
    }
    if ((!tab->hidden) && (name[0] == '.')) {
        return 0;
    }
    int i = tab->len;

    //Allocate memory for new menu item
    tab->len++;
    if (tab->len + 1 > temp_count) {
        temp_count += 20;
        tab->items = realloc(
            tab->items,
            sizeof(mfm_tab_item) * temp_count
        );
    }

    //Save the name of file
    tab->items[i].text = malloc(strlen(name)+1);
    strcpy(tab->items[i].text, name);

    //Is it directory?
    if (S_ISDIR(st->st_mode)) {
        tab->items[i].props = MFM_DIR;
    } else if (S_ISREG(st->st_mode)) {
        tab->items[i].props = MFM_REG;
        //Or executable?
        tab->items[i].props &= mfm_is_exec(name, st, f_cmd);
    }

    //Finish work
    (*(mfm_init_tab_item_udata*)udata).temp_count = temp_count;
    return 0;
}

/**
 * Draw the current tab content
 * @param tab
 * @param h
 * @param w
 */
void mfm_draw_tab(
    mfm_tab* tab, //Menu to drawing
    int h,         //Screen size
    int w
) {
    //Draw the directory in the head
    printf("\e[1;1H\e[32;49m\e[2K%s", tab->dir);

        //Position on the screen
    int scr_pos,
        //Position in the menu
        tab_pos = tab->pos_view,
        //Offset by empty line between dirs and files
        off = 0;

    //Lines with text
    for(
        scr_pos = 2;
        scr_pos <= h - 2 && tab_pos < tab->len;
        tab_pos++, scr_pos++
    ) {
        if (off != !(tab->items[tab_pos].props & MFM_DIR)) {
            printf("\e[33;49m\e[%i;1H", scr_pos);
            for (int i = 1; i <= w; i++) {
                printf("%s", "─");
            }
        }
        off = !(tab->items[tab_pos].props & MFM_DIR);
        printf(
            "\e[%i;1H\e[%i;%im",
            scr_pos + off,
            tab->items[tab_pos].props & MFM_SEL ? 33 :
                tab->items[tab_pos].props & MFM_EXE ? 36 : 37,
            tab_pos == tab->act ? 42 : 49);
        printf("%s\e[0K", tab->items[tab_pos].text);
    }

    //Empty lines
    if (!off) {
        printf("\e[33;49m\e[%i;1H", scr_pos);
        for (int i = 1; i <= w; i++) {
            printf("%s", "─");
        }
    }
    printf("%s", "\e[49m");
    scr_pos++;
    for (; scr_pos <= h - 1; scr_pos++) {
        printf("\e[%i;1H\e[2K", scr_pos);
    }

    //Show current position in menu
    printf(
        "\e[33;49m\e[1;%iH\e[0K%i%c",
        w - 3,
        (tab->act + 1) * 100 / tab->len,
        '%'
    );
}

/**
 * Correct tab params
 * @param tab
 * @param h
 * @param w
 */
void mfm_correct_tab(mfm_tab* tab, int h, int w)
{
    if (tab->act < 0) {
        tab->act = 0;
    } else if (tab->act > tab->len - 1) {
        tab->act = tab->len - 1;
    }
    if (tab->pos_view > tab->act) {
        tab->pos_view = tab->act;
        return;
    }
    if (tab->act - tab->pos_view > h - 4) {
        tab->pos_view = tab->act - h + 4;
    }
}
