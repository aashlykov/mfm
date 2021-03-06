#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <search.h>
#include <unistd.h>
#include <search.h>
#include <sys/stat.h>

#include "mfm_general.h"
#include "mfm_tab.h"
#include "mfm_input.h"
#include "mfm_handle_input.h"

int mfm_main(mfm_state* st);

/**
 * Entry point of application
 * @param argc
 * @param argv
 * @return
 */
int main(int argc, char** argv)
{
    //Global state
    mfm_state st;

    //Read the config
    st.f_cmd = NULL;
    mfm_read_config(&(st.f_cmd));
    st.bookmarks = mfm_read_bookmarks();

    //If there is no params - init current directory
    if (argc == 1) {
        st.tabs = calloc(sizeof(mfm_tab), 1);
        mfm_init_tab(st.tabs, &(st.f_cmd));
        st.len = 1;
    } else {
        //Init tabs from command line params
        st.tabs = calloc(sizeof(mfm_tab), argc - 1);
        st.len = argc - 1;
        for (int i = 1; i < argc && i <= 10; i++) {
            int res = chdir(argv[i]);
            mfm_init_tab(st.tabs + i - 1, &(st.f_cmd));
        }
    }

    st.cur = 0;

    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stdin, NULL, _IONBF, 0);

    printf("%s", "\e[1m\e[?25l");
    int res = system("stty raw;stty -echo");
    int exit_code;
    while (!(exit_code = mfm_main(&st)));
    printf("%s", "\e[0m\e[?25h\e[2J\e[1;1H\ec");
    res = system("stty sane;stty echo");

    if (exit_code == 2) {
        printf("%s", "Screen size is lesser than 80x24\n");
    }

    return 0;
}

/**
 * Main loop
 * @param st
 * @return
 */
int mfm_main(mfm_state* st)
{
    //Get the screen size
    int h, w;
    mfm_scr_size(&h, &w);

    if (w < 80 || h < 24) {
        return 2;
    }

    //Draw current tab
    mfm_tab* tab = st->tabs + st->cur;
    int res = chdir(tab->dir);
    mfm_correct_tab(tab, h, w);
    mfm_draw_tab(tab, h, w);

    //Draw help line
    mfm_help(h, w);

    //Draw tabs numbers
    mfm_draw_numbers(st, h, w);

    //Read and handle input
    return mfm_handle_input(st);
}
