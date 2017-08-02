#ifndef MFM_GENERAL
#define MFM_GENERAL

#include <sys/types.h>
#include <sys/stat.h>
#include "mfm_tab.h"

//Global state of session
typedef struct
{
    mfm_tab** tabs; //Null terminated array of current
                    //menus
    int cur;        //Current active menu
    void* f_cmd;   //Commands for files
    char** bookmarks; //
} mfm_state;

//Substitute %f by selected files and %% by %
char* mfm_substitute(mfm_tab* menu, char* orig);

//Read config file with actions for files of
//different types
void mfm_read_config(void** f_cmd);

//Compare function to get kv
int mfm_get_kv(const void* one, const void* two);

//Get the screen size
void mfm_scr_size(int* h, int* w);

//Execute command in shell
void mfm_command(char* comm);

//Show message in the bot of window
void mfm_show_message(char* message, int delay);

//Append two paths with /
char* mfm_append_paths(char* one, char* two);

//Traverse the directory
int mfm_traverse_directory(
    char* path,
    int (*fn)(char*, struct stat*, void*),
    void* udata
);

//Is it '.' or '..' directory
int mfm_dots_dir(char* path);

//Draw help line
void mfm_help(int h, int w);

//Draw the tab numbers
void mfm_draw_numbers(mfm_state* st, int h, int w);

//Read the bookmarks
char** mfm_read_bookmarks();

//Create the empty directory
int mfm_mkdir(char* name);

//Delete the item
int mfm_delete_item(char* path, struct stat* st, void* udata);

//Travers all selected items, except the current tab
void mfm_travers_all_selected(
    mfm_state* st,
    void (*cb)(mfm_tab*, mfm_tab_item*, void*),
    void* udata
);

//Get the text presentation of size in bytes
void mfm_get_size_text(long long bytes, char* buf);

#endif
