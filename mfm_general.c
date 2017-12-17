#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <search.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "mfm_general.h"
#include "mfm_input.h"

char* mfm_all_selected(mfm_tab* tab);
int mfm_subs_count(char* orig, int files_len);
void mfm_subs_write(char* dest, char* orig, char* files);

/**
 * Substitute all %f by selected files and %%f by %f
 * @param tab
 * @param orig
 * @return
 */
char* mfm_substitute(mfm_tab* tab, char* orig)
{
    char* files = mfm_all_selected(tab);
    int files_len = strlen(files);

    char* res = malloc(mfm_subs_count(orig, files_len) + 1);
    mfm_subs_write(res, orig, files);

    free(files);

    return res;
}

int mfm_shell_len(char* text);
char* mfm_write_single_item(char* dest, char* item);
/**
 * Form the selected items to single string
 * @param tab
 * @return
 */
char* mfm_all_selected(mfm_tab* tab)
{
    int r_len = 0;
    
    //Count the total len
    for (int i = 0; i < tab->len; i++) {
        if (tab->items[i].props & MFM_SEL) {
            r_len += mfm_shell_len(tab->items[i].text);
        }
    }
    if (!r_len) {
        r_len += mfm_shell_len(tab->items[tab->act].text);
    }
    
    //Write the result
    char* res = malloc(r_len);
    char* cur = res;
    for (int i = 0; i < tab->len; i++) {
        if (tab->items[i].props & MFM_SEL) {
            cur = mfm_write_single_item(cur, tab->items[i].text);
        }
    }
    if (cur == res) {
        cur = mfm_write_single_item(cur, tab->items[tab->act].text);
    }
    
    res[r_len] = '\0';
    
    return res;
}

/**
 * Count len of string in the shell command
 * @param text
 * @return
 */
int mfm_shell_len(char* text)
{
    int res = 3;
    for (char* c = text; *c; c++) {
        switch (*c) {
        case '\\':
        case '"':
            res += 2;
        default:
            res++;
        }
    }
    return res;
}

/**
 * Write to the buffer single item
 * @param dest
 * @param item
 * @return Current position
 */
char* mfm_write_single_item(char* dest, char* item)
{
    *dest++ = '"';
    for (char* c = item; *c; c++) {
        switch (*c) {
        case '\\':
            *dest++ = '\\';
            *dest++ = '\\';
            break;
        case '"':
            *dest++ = '\\';
            *dest++ = '"';
            break;
        default:
            *dest++ = *c;
        }
    }
    *dest++ = '"';
    *dest++ = ' ';
    return dest;
}

/**
 * Count total len
 * @param orig
 * @param files_len
 * @return 
 */
int mfm_subs_count(char* orig, int files_len)
{
    int res = strlen(orig);
    char* cur = orig;
    while (cur = strstr(cur, "%f")) {
        if (cur == orig) {
            res = res - 2 + files_len;
        } else if (cur[-1] == '%') {
            res--;
        } else {
            res = res - 2 + files_len;
        }
        cur += 2;
    }
    return res;
}

/**
 * Write data to the format string
 * @param dest
 * @param orig
 * @param files
 */
void mfm_subs_write(char* dest, char* orig, char* files)
{
    int files_len = strlen(files);
    char* prev = orig;
    char* next;
    while (next = strstr(prev, "%f")) {
        if (next == orig) {
            strcpy(dest, files);
            dest += files_len;
        } else if (next[-1] == '%') {
            int delta = next - 1 - prev;
            memcpy(dest, prev, delta);
            dest += delta;
            *dest++ = '%';
            *dest++ = 'f';
        } else {
            int delta = next - prev;
            memcpy(dest, prev, delta);
            dest += delta;
            strcpy(dest, files);
            dest += files_len;
        }
        prev = next + 2;
    }
    strcpy(dest, prev);
}

/**
 * Cut the \r\n symbols from string
 * @param str
 */
void mfm_cut_1310(char* str)
{
    int l = strlen(str);
    for (int i = l - 1; i >= l - 2; i--) {
        if (str[i] == '\n' || str[i] == '\r') {
            str[i] = '\0';
        }
    }
}

/**
 * Compare two key-value pairs for search and delete
 * @param one
 * @param two
 * @return
 */
int mfm_kv_ins(const void* one, const void* two)
{
    unsigned char* s1 = ((unsigned char**)one)[0];
    unsigned char* s2 = ((unsigned char**)two)[0];
    int l1 = strlen(s1);
    int l2 = strlen(s2);
    int res = 0;
    while (l1 && l2 && !res) {
        l1--;
        l2--;
        res = (int)s1[l1] - (int)s2[l2];
    }
    if (res) {
        return res;
    }
    return l1 - l2;
}

/**
 * Read config file with actions for files of different types
 * @param f_cmd
 */
void mfm_read_config(void** f_cmd)
{
    //Find the file
    char* fname = mfm_append_paths(getenv("HOME"), ".local/etc/mfm/ext");
    FILE* f = fopen(fname, "r");
    free(fname);
    if (!f) {
        return;
    }

    //Read line by line
    char buf[256];
    char* last = NULL;
    for (;;) {
        //Read the line
        if (!fgets(buf, 256, f)) {
            break;
        }

        //Skip if it's not full line
        int len = strlen(buf);
        if (buf[len - 1] != '\n' && !feof(f)) {
            continue;
        }

        //Skip if line starts with '='
        if (buf[0] == '=') {
            continue;
        }

        mfm_cut_1310(buf);

        //Form key-value element
        char** kv = malloc(sizeof(char*) * 2);
        char* delim = strrchr(buf, '=');
        if (delim) {
            *delim = '\0';
            kv[1] = malloc(strlen(delim + 1) + 1);
            strcpy(kv[1], delim + 1);
            last = kv[1];
        } else if (last) {
            kv[1] = last;
        }
        kv[0] = malloc(strlen(buf) + 1);
        strcpy(kv[0], buf);

        //Set kv in tree
        char** exist = *(char***)tsearch(kv, f_cmd, mfm_kv_ins);
        if (exist != kv) {
            free(kv[0]);
            if (kv[1] != last) {
                free(kv[1]);
            }
            free(kv);
        }
    }

    //Finish work
    fclose(f);
}

/**
 * Compare function to get kv
 * @param one
 * @param two
 * @return
 */
int mfm_get_kv(const void* one, const void* two)
{
    unsigned char* s1 = (unsigned char*)one;
    unsigned char* s2 = ((unsigned char**)two)[0];
    int l1 = strlen(s1);
    int l2 = strlen(s2);

    int res = 0;

    while (l1 && l2 && !res) {
        l1--;
        l2--;
        res = (int)s1[l1] - (int)s2[l2];
    }

    if (!res && l2 <= l1) {
        return 0;
    } else if (!res && l2 > l1){
        return -1;
    }

    return res;
}

/**
 * Get the size of screen
 * @param h
 * @param w
 */
void mfm_scr_size(int* h, int* w)
{
    mfm_drain_input();
    printf("%s", "\e[1000;5000H\e[6n");
    int res = scanf("\e[%i;%iR", h, w);
}

/**
 * Execute command in shell
 * @param comm
 */
void mfm_command(char* comm)
{
    int res;
    printf("%s", "\ec");
    fflush(stdout);
    res = system("stty sane;stty echo");
    res = system(comm);
    printf("%s", "\e[1m\e[?25l");
    res = system("stty raw;stty -echo");
}

/**
 * Show message in the bot of window
 * @param message
 * @param delay
 */
void mfm_show_message(char* message, int delay)
{
    int h, w;
    mfm_scr_size(&h, &w);
    printf("\e[%i;1H\e[41;33m\e[2K%s", h, message);
    char buf[8];
    mfm_key key;
    if (delay) {
        mfm_read_key(buf, &key);
    }
    fflush(stdout);
}

/**
 * Append two paths with /
 * @param one
 * @param two
 * @return
 */
char* mfm_append_paths(char* one, char* two)
{
    char* res = malloc(
        strlen(one) +
        strlen(two) +
        2
    );
    sprintf(res, "%s/%s", one, two);
    return res;
}

/**
 * Traverse the directory
 * @param path
 * @param fn
 * @param udata
 * @return
 */
int mfm_traverse_directory(
    char* path,
    int (*fn)(char*, struct stat*, void*),
    void* udata
) {
    DIR* dir = opendir(path);
    if (dir == NULL) {
        return 1;
    }

    struct dirent* n;
    while (n = readdir(dir)) {
        char* next_item = mfm_append_paths(path, n->d_name);
        struct stat st;
        if (stat(next_item, &st) == -1) {
            free(next_item);
            continue;
        }
        if (fn(next_item, &st, udata) < 0) {
            break;
        }
        free(next_item);
    }

    closedir(dir);
    return 0;
}

/**
 * Is it '.' or '..' directory
 * @param path
 * @return
 */
int mfm_dots_dir(char* path)
{
    char* last = strrchr(path, '/');
    if (last) {
        path = last + 1;
    }
    if (!strcmp(path, ".") || !strcmp(path, "..")) {
        return 1;
    }
    return 0;
}

/**
 * Draw help line
 * @param h
 * @param w
 */
void mfm_help(int h, int w)
{
    printf("\e[%i;1H\e[44m\e[2K", h);
    printf("%s", "\e[37mMK\e[31mD\e[37mIR ");
    printf("%s", "\e[31mE\e[37mXEC ");
    printf("%s", "\e[31mS\e[37mHELL ");
    printf("%s", "\e[31mR\e[37mENAME ");
    printf("%s", "\e[31mU\e[37mPDATE ");
    printf("%s", "\e[31mH\e[37mIDDEN ");
    printf("%s", "\e[31mG\e[37mOTO ");
    printf("%s", "\e[31mC\e[37mOPY ");
    printf("%s", "\e[31mM\e[37mOVE ");
    printf("%s", "\e[31mQ\e[37mUIT ");
}

/**
 * Draw the tab numbers
 * @param st
 * @param h
 * @param w
 */
void mfm_draw_numbers(mfm_state* st, int h, int w)
{
    int i = 0;
    printf("\e[%i;%iH", h, w - 10);
    while (i < st->len) {
        if (i == st->cur) {
            printf("%s", "\e[31;43m ");
        } else {
            printf("\e[33;41m%i", i);
        }
        i++;
    }
    printf("%s", "\e[33;41m\e[0K");
}

/**
 * Read the bookmarks
 * @return
 */
char** mfm_read_bookmarks()
{
    char* fname = mfm_append_paths(getenv("HOME"), ".local/etc/mfm/bookmarks");
    FILE* f = fopen(fname, "r");
    free(fname);
    if (!f) {
        return NULL;
    }
    int cur = 0, tsz = 10;
    char** res = malloc(sizeof(char*) * tsz);
    char buf[256];
    while (!feof(f)) {
        if (!fgets(buf, 256, f)) {
            continue;
        }
        mfm_cut_1310(buf);
        int l = strlen(buf);
        if (!l) {
            continue;
        }
        res[cur] = malloc(l + 1);
        strcpy(res[cur], buf);
        cur++;
        if (cur == tsz) {
            tsz += 10;
            res = realloc(res, sizeof(char*) * tsz);
        }
    }

    res = realloc(res, sizeof(char*) * (cur + 1));
    res[cur] = NULL;

    return res;
}

/**
 * Delete the item
 * @param path
 * @param st
 * @param udata
 * @return
 */
int mfm_delete_item(char* path, struct stat* st, void* udata)
{
    if (mfm_dots_dir(path)) {
        return 0;
    }
    if (S_ISDIR(st->st_mode)) {
        mfm_traverse_directory(path, mfm_delete_item, udata);
    }
    remove(path);
    return 0;
}

/**
 * Travers all selected items, except the current tab
 * @param st
 * @param cb
 * @param udata
 */
void mfm_travers_all_selected(
    mfm_state* st,
    void (*cb)(mfm_tab*, mfm_tab_item*, void*),
    void* udata
) {
    for (int i = 0; i < st->len; i++) {
        if (st->cur == i) {
            continue;
        }
        mfm_tab* tab = st->tabs + i;
        for (int j = 0; j < tab->len; j++) {
            if (tab->items[j].props & MFM_SEL) {
                cb(tab, tab->items + j, udata);
            }
        }
    }
}

/**
 * Create the empty directory
 * @param name
 * @return
 */
int mfm_mkdir(char* name)
{
    return mkdir(name,
        S_IRUSR | S_IWUSR | S_IXUSR |
        S_IRGRP | S_IXGRP |
        S_IROTH | S_IXOTH
    );
}

/**
 * Get the text presentation of size in bytes
 * @param bytes
 * @param buf
 */
void mfm_get_size_text(long long bytes, char* buf)
{
    long long gb = 1 << 30, mb = 1 << 20, kb = 1 << 10;
    if (bytes > 1 << 30) { //GB and MB
        sprintf(buf, "%i Gb %i Mb", (int)(bytes / gb), (int)(bytes % gb / mb));
    } else if (bytes > 1 << 20) { //MB and KB
        sprintf(buf, "%i Mb %i Kb", (int)(bytes / mb), (int)(bytes % mb / kb));
    } else if (bytes > 1 << 10) { //KB and B
        sprintf(buf, "%i Kb %i b", (int)(bytes / kb), (int)(bytes % kb));
    } else { //B
        sprintf(buf, "%i b", (int)(bytes));
    }
}

/**
 * Drain all data at standard input
 */
void mfm_drain_input()
{
    char c;
    int fs = fcntl(0, F_GETFL, 0);
    fcntl(0, F_SETFL, fs | O_NONBLOCK);
    while (read(0, &c, 1) > 0);
    fcntl(0, F_SETFL, fs);
}
