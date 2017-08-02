#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "mfm_goto.h"
#include "mfm_general.h"
#include "mfm_input.h"
#include "mfm_simple_menu.h"

//Input directory and goto
void mfm_input_goto(int h, int w);

//Goto to the something directory
void mfm_goto(char** bookmarks)
{
    //Draw the head
    printf("%s", "\e[1;1H\e[33;41m\e[2K");
    printf("%s", "Go to the directory:");

    //If there is no bookmarks - input directory
    //and go to it
    int h, w;
    mfm_scr_size(&h, &w);
    if (!bookmarks) {
        mfm_input_goto(h, w);
        return;
    }

    //Select the bookmark from menu
    int choice = mfm_simple_menu(bookmarks);
    if (choice == -1) {
        return;
    }
    int res = chdir(bookmarks[choice]);
}

//Input directory and goto
void mfm_input_goto(int h, int w)
{
    printf("\e[%i;1H\e[33;41m", h);
    char* dir = mfm_read_line(h, 1, w, NULL);
    if (dir) {
        int res = chdir(dir);
        free(dir);
    }
}
