#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <search.h>
#include <sys/stat.h>

#include "mfm_general.h"
#include "mfm_input.h"

//Substitute %f by selected files and %% by %
char* mfm_substitute(mfm_tab* tab, char* orig)
{
    //Active element in menu
    int act = tab->act;

    //Length and string with selected files
    int f_len = 0;
    char* files;
    for (int i = 0; i < tab->len; i++) {
        if (tab->items[i].props & MFM_SEL) {
            f_len += strlen(tab->items[i].text) + 3;
        }
    }
    if (!f_len) {
        f_len = strlen(tab->items[act].text) + 3;
        files = malloc(f_len);
        files[0] = '"';
        strcpy(files + 1, tab->items[act].text);
    } else {
        files = malloc(f_len);
        int cur = 0;
        for (int i = 0; i < tab->len; i++) {
            if (tab->items[i].props & MFM_SEL) {
                files[cur++] = '"';
                strcpy(files + cur, tab->items[i].text);
                cur += strlen(tab->items[i].text);
                files[cur++] = '"';
                files[cur++] = ' ';
            }
        }
    }
    files[f_len - 1] = '\0';
    files[f_len - 2] = '"';
    f_len--;

    //Length of result string
    int r_len = 0;
    for (int i = 0; orig[i]; i++) {
        if (orig[i] == '%') {
            if (orig[++i] == '%') {
                r_len++;
            } else if (orig[i] == 'f') {
                r_len += f_len;
            } else {
                r_len += 2;
            }
        } else {
            r_len++;
        }
    }

    //Forming the result string
    char* result = malloc(r_len + 1);
    int cur = 0;
    for (int i = 0; orig[i]; i++) {
        if (orig[i] == '%') {
            if (orig[++i] == '%') {
                result[cur++] = '%';
            } else if (orig[i] == 'f') {
                strcpy(result + cur, files);
                cur += f_len;
            } else {
                result[cur++] = '%';
                result[cur++] = orig[i];
            }
        } else {
            result[cur++] = orig[i];
        }
    }
    result[r_len] = '\0';
    return result;
}

//Cut the \r\n symbols from string
void mfm_cut_1310(char* str)
{
    int l = strlen(str);
    for (int i = l - 1; i >= l - 2; i--) {
        if (str[i] == '\n' || str[i] == '\r') {
            str[i] = '\0';
        }
    }
}

//Compare two key-value pairs for search and delete
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

//Read config file with actions for files of
//different types
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

//Compare function to get kv
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

//Get the size of screen
void mfm_scr_size(int* h, int* w)
{
    printf("%s", "\e[100;500H\e[6n");
    int res = scanf("\e[%i;%iR", h, w);
}

//Execute command in shell
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

//Show message in the bot of window
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

//Append two paths with /
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

//Traverse the directory
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

//Is it '.' or '..' directory
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

//Draw help line
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

//Draw the tab numbers
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

//Read the bookmarks
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

//Delete the item
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

//Travers all selected items, except the current tab
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

//Create the empty directory
int mfm_mkdir(char* name)
{
    mkdir(name,
        S_IRUSR | S_IWUSR | S_IXUSR |
        S_IRGRP | S_IXGRP |
        S_IROTH | S_IXOTH
    );
}

//Get the text presentation of size in bytes
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
