#ifndef MFM_GENERAL_H
#define MFM_GENERAL_H

#include <sys/types.h>
#include <sys/stat.h>
#include "mfm_tab.h"

/**
 * Global state of session
 */
typedef struct
{
    mfm_tab* tabs;    /**< Array of current menus */
    int len;          /**< Count of menus */
    int cur;          /**< Current active menu */
    void* f_cmd;      /**< Commands for files */
    char** bookmarks; /**< Bookmarks for fast jump */
} mfm_state;

/**
 * Substitute %f by selected files and %% by %
 * @param menu Current tab with items
 * @param orig Original string
 * @return
 */
char* mfm_substitute(mfm_tab* menu, char* orig);

/**
 * Read config file with actions for files of
 * different types
 * @param f_cmd Place to put result
 */
void mfm_read_config(void** f_cmd);

/**
 * Compare function to get kv
 * @param one
 * @param two
 * @return
 */
int mfm_get_kv(const void* one, const void* two);

/**
 * Get the screen size
 * @param h Place to put height
 * @param w Place to put width
 */
void mfm_scr_size(int* h, int* w);

/**
 * Execute command in shell
 * @param comm
 */
void mfm_command(char* comm);

/**
 * Show message in the bot of window
 * @param message to show
 * @param delay after showing
 */
void mfm_show_message(char* message, int delay);

/**
 * Append two paths with / . You shoul free result of this function
 * after using
 * @param one
 * @param two
 * @return
 */
char* mfm_append_paths(char* one, char* two);

/**
 * Traverse the directory
 * @param path  Target directory
 * @param fn    Function to be applied to every item
 * @param udata User data for function
 * @return
 */
int mfm_traverse_directory(
    char* path,
    int (*fn)(char*, struct stat*, void*),
    void* udata
);

/**
 * Is it '.' or '..' directory
 * @param path
 * @return
 */
int mfm_dots_dir(char* path);

/**
 * Draw help line
 * @param h Coords where draw it
 * @param w
 */
void mfm_help(int h, int w);

/**
 * Draw the tab numbers
 * @param st
 * @param h Coords where to draw
 * @param w
 */
void mfm_draw_numbers(mfm_state* st, int h, int w);

/**
 * Read the bookmarks
 * @return
 */
char** mfm_read_bookmarks();

/**
 * Create the empty directory
 * @param name
 * @return
 */
int mfm_mkdir(char* name);

/**
 * Delete the item (callback for traverse function)
 * @param path
 * @param st
 * @param udata
 * @return
 */
int mfm_delete_item(char* path, struct stat* st, void* udata);

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
);

/**
 * Get the text presentation of size in bytes
 * @param bytes
 * @param buf
 */
void mfm_get_size_text(long long bytes, char* buf);

/**
 * Drain all data at standard input
 */
void mfm_drain_input();

#endif /* MFM_GENERAL_H */

