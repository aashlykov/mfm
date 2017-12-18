#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <dirent.h>
#include <string.h>
#include <sys/statvfs.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "mfm_copy.h"
#include "mfm_general.h"
#include "mfm_input.h"
#include "mfm_tab.h"

/**
 * Files/Directories to be copied
 */
typedef struct mfm_list
{
    char* src;
    char* dst;
    struct mfm_list* next;
} mfm_list;

/**
 * List of items to delete
 */
typedef struct mfm_list_to_delete
{
    mfm_tab* tab;
    mfm_tab_item* it;
    struct mfm_list_to_delete* next;
} mfm_list_to_delete;

void mfm_copy_item(mfm_tab* tab, mfm_tab_item* it, void* udata);
typedef struct
{
    mfm_list* hf;
    mfm_list* f;
    mfm_list* hd;
    mfm_list* d;
    int slen;
    char* dest;
    long long ondisk;
    long long real;
} mfm_copy_item_udata;
void mfm_mk_dirs(mfm_list* ls);
void mfm_copy_list(mfm_list* ls, long long ttl);
void mfm_destroy_list(mfm_list* ls);

/**
 * Main copy operation
 * @param st
 */
void mfm_copy(mfm_state* st)
{
    //Collect all items to copy
    mfm_copy_item_udata udata;
    memset(&udata, 0, sizeof(udata));
    udata.dest = st->tabs[st->cur].dir;
    mfm_travers_all_selected(st, mfm_copy_item, &udata);

    //Do the operations
    struct statvfs sv;
    statvfs(udata.dest, &sv);
    unsigned long long free_bytes = sv.f_bsize * sv.f_bfree;
    if (udata.ondisk > free_bytes) {
        mfm_show_message("Not enough free space", 1);
        char od[50], fr[50];
        mfm_get_size_text(udata.ondisk, od);
        mfm_get_size_text(free_bytes, fr);
        char template[][15] = {
            "Required: %s",
            "Available: %s"
        };
        char *message = malloc(strlen(template[0]) - 2 + strlen(od));
        sprintf(message, template[0], od);
        mfm_show_message(message, 1);
        free(message);
        message = malloc(strlen(template[1]) - 2 + strlen(fr));
        sprintf(message, template[1], fr);
        mfm_show_message(message, 1);
        free(message);
    } else {
        mfm_mk_dirs(udata.hd);
        mfm_copy_list(udata.hf, udata.real);
    }

    //Free the data
    mfm_destroy_list(udata.hd);
    mfm_destroy_list(udata.hf);
}

int mfm_copy_cb(char* name, struct stat* st, void* udata);

/**
 * Copy single item from menu
 * @param tab
 * @param it
 * @param udata
 */
void mfm_copy_item(mfm_tab* tab, mfm_tab_item* it, void* udata)
{
    char* full = mfm_append_paths(tab->dir, it->text);
    struct stat st;
    if (stat(full, &st) != -1) {
        ((mfm_copy_item_udata*)udata)->slen = strlen(tab->dir) + 1;
        mfm_copy_cb(full, &st, udata);
    }
    free(full);
}

/**
 * Copy items and subitems
 * @param name
 * @param st
 * @param udata
 * @return
 */
int mfm_copy_cb(char* name, struct stat* st, void* udata)
{
    if (mfm_dots_dir(name)) {
        return 0;
    }
    mfm_list* tmp = malloc(sizeof(mfm_list));
    tmp->src = malloc(strlen(name) + 1);
    strcpy(tmp->src, name);
    tmp->dst = mfm_append_paths(
        ((mfm_copy_item_udata*)udata)->dest,
        name + ((mfm_copy_item_udata*)udata)->slen
    );
    tmp->next = NULL;
    ((mfm_copy_item_udata*)udata)->ondisk += 512 * st->st_blocks;
    if (S_ISDIR(st->st_mode)) {
        if (((mfm_copy_item_udata*)udata)->hd == NULL) {
            ((mfm_copy_item_udata*)udata)->hd = tmp;
            ((mfm_copy_item_udata*)udata)->d  = tmp;
        } else {
            ((mfm_copy_item_udata*)udata)->d->next = tmp;
            ((mfm_copy_item_udata*)udata)->d =
                ((mfm_copy_item_udata*)udata)->d->next;
        }
        mfm_traverse_directory(tmp->src, mfm_copy_cb, udata);
    } else {
        if (((mfm_copy_item_udata*)udata)->hf == NULL) {
            ((mfm_copy_item_udata*)udata)->hf = tmp;
            ((mfm_copy_item_udata*)udata)->f  = tmp;
        } else {
            ((mfm_copy_item_udata*)udata)->f->next = tmp;
            ((mfm_copy_item_udata*)udata)->f =
                ((mfm_copy_item_udata*)udata)->f->next;
        }
        ((mfm_copy_item_udata*)udata)->real += st->st_size;
    }
    return 0;
}

void mfm_move_item(mfm_tab* tab, mfm_tab_item* it, void* udata);
typedef struct
{
    mfm_copy_item_udata cu;
    mfm_list_to_delete* hdel;
    mfm_list_to_delete* cdel;
} mfm_move_item_udata;
void mfm_delete_list(mfm_list_to_delete* ls);
void mfm_delete_list_destroy(mfm_list_to_delete* ls);

/**
 * Main move operation
 * @param st
 */
void mfm_move(mfm_state* st)
{
    //Collect items to copy
    mfm_move_item_udata udata;
    memset(&udata, 0, sizeof(udata));
    udata.cu.dest = st->tabs[st->cur].dir;
    mfm_travers_all_selected(st, mfm_move_item, &udata);

    //Do the operations
    struct statvfs sv;
    statvfs(udata.cu.dest, &sv);
    unsigned long long free_bytes = sv.f_bsize * sv.f_bfree;
    if (udata.cu.ondisk > free_bytes) {
        mfm_show_message("Not enough free space", 1);
        char od[50], fr[50];
        mfm_get_size_text(udata.cu.ondisk, od);
        mfm_get_size_text(free_bytes, fr);
        char template[][15] = {
            "Required: %s",
            "Available: %s"
        };
        char *message = malloc(strlen(template[0]) - 2 + strlen(od));
        sprintf(message, template[0], od);
        mfm_show_message(message, 1);
        free(message);
        message = malloc(strlen(template[1]) - 2 + strlen(fr));
        sprintf(message, template[1], fr);
        mfm_show_message(message, 1);
        free(message);
    } else {
        mfm_mk_dirs(udata.cu.hd);
        mfm_copy_list(udata.cu.hf, udata.cu.real);
        mfm_delete_list(udata.hdel);
    }

    //Update the changed tabs
    for (int i = 0; i < st->len; i++) {
        int res = chdir(st->tabs[i].dir);
        mfm_init_tab(st->tabs + i, &(st->f_cmd));
    }
    int res = chdir(st->tabs[st->cur].dir);

    //Free the data
    mfm_destroy_list(udata.cu.hd);
    mfm_destroy_list(udata.cu.hf);
    mfm_delete_list_destroy(udata.hdel);
}

/**
 * Move single item from menu
 * @param tab
 * @param it
 * @param udata
 */
void mfm_move_item(mfm_tab* tab, mfm_tab_item* it, void* udata)
{
    char* old = mfm_append_paths(tab->dir, it->text);
    char* new = mfm_append_paths(
        ((mfm_move_item_udata*)udata)->cu.dest,
        it->text
    );
    if (rename(old, new)) {
        mfm_copy_item(tab, it, &(((mfm_move_item_udata*)udata)->cu));
        mfm_list_to_delete* tmp = malloc(sizeof(mfm_list_to_delete));
        tmp->tab  = tab;
        tmp->it   = it;
        tmp->next = NULL;
        if (((mfm_move_item_udata*)udata)->hdel == NULL) {
            ((mfm_move_item_udata*)udata)->hdel = tmp;
            ((mfm_move_item_udata*)udata)->cdel = tmp;
        } else {
            ((mfm_move_item_udata*)udata)->cdel->next = tmp;
            ((mfm_move_item_udata*)udata)->cdel =
                ((mfm_move_item_udata*)udata)->cdel->next;
        }
    }
    free(old);
    free(new);
}

/**
 * Delete the items from list
 * @param ls
 */
void mfm_delete_list(mfm_list_to_delete* ls)
{
    while (ls) {
        char* full = mfm_append_paths(ls->tab->dir, ls->it->text);
        struct stat st;
        if (stat(full, &st) != -1) {
            mfm_delete_item(full, &st, NULL);
        }
        ls = ls->next;
        free(full);
    }
}

/**
 * Destroy list of items to delete
 * @param ls
 */
void mfm_delete_list_destroy(mfm_list_to_delete* ls)
{
    while (ls) {
        mfm_list_to_delete* tmp = ls;
        ls = ls->next;
        free(tmp);
    }
}

int mfm_copy_file(
    char* source,
    char* dest,
    long long* cur,
    long long ttl
);

void mfm_copy_progress(long long cr, long long ttl);

/**
 * Copy all files from list
 * @param ls
 * @param ttl
 */
void mfm_copy_list(mfm_list* ls, long long ttl)
{
    long long cur = 0;
    while (ls) {
        mfm_copy_file(
            ls->src,
            ls->dst,
            &cur,
            ttl
        );
        ls = ls->next;
    }
}

/**
 * Create all destinations directories
 * @param ls
 */
void mfm_mk_dirs(mfm_list* ls)
{
    while (ls) {
        mfm_mkdir(ls->dst);
        ls = ls->next;
    }
}

/**
 * Destroy the list
 * @param ls
 */
void mfm_destroy_list(mfm_list* ls)
{
    while (ls) {
        mfm_list* tmp = ls;
        ls = ls->next;
        free(tmp->src);
        free(tmp->dst);
        free(tmp);
    }
}

/**
 * Copy file
 * @param source
 * @param dest
 * @param cur
 * @param ttl
 * @return
 */
int mfm_copy_file(
    char* source,
    char* dest,
    long long* cur,
    long long ttl
) {
    //If dest already exists - skip file
    struct stat st;
    if (stat(dest, &st) != -1) {
        return 1;
    }

    //Open input file
    FILE* inp = fopen(source, "r");
    if (!inp) {
        return 1;
    }

    //Open output file
    FILE* otp = fopen(dest, "w");
    if (!otp) {
        fclose(inp);
        return 1;
    }

    //Main cycle of coping
    int bs = st.st_blksize;
    char* buff = malloc(bs);
    while (!feof(inp)) {
        int readed = fread(buff, 1, bs, inp);
        fwrite(buff, 1, readed, otp);
        *cur += readed;
        mfm_copy_progress(*cur, ttl);
    }
    free(buff);

    //finish work
    fclose(inp);
    fclose(otp);
    return 0;
}

/**
 * Show copy progress
 * @param cur
 * @param ttl
 */
void mfm_copy_progress(long long cur, long long ttl)
{
    static time_t tm = 0;
    if (time(NULL) - tm < 1) {
        return;
    }
    int prc = cur * 1000 / ttl;
    char mess[80];
    char cur_txt[40], ttl_txt[40];
    mfm_get_size_text(cur, cur_txt);
    mfm_get_size_text(ttl, ttl_txt);
    sprintf(mess, "Done %s  of  %s", cur_txt, ttl_txt);
    mfm_show_message(mess, 0);
    tm = time(NULL);
}
