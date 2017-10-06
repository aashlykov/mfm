#include <stdio.h>

#include "mfm_input.h"
#include "mfm_general.h"
#include "mfm_simple_menu.h"

/**
 * Run simple menu
 * @param variants for selecting
 * @return
 */
int mfm_simple_menu(char** variants)
{
    //Select input directory or from bookmarks
    int act = 0;
    int pos = 0;

    char buf[8];
    int f;
    mfm_key key;
    for (;;) {
        //Screen size
        int h, w;
        mfm_scr_size(&h, &w);

        //Correct the view position
        if (act < pos) {
            pos = act;
        } else if (act > pos + h - 3) {
            pos = act - h + 3;
        }

        //Draw the current variants
        int md = 0;
        for (int i = 2; i < h; i++) {
            printf("\e[%i;1H", i);
            if (i - 2 == act) {
                printf("%s", "\e[37;42m");
            } else {
                printf("%s", "\e[37;40m");
            }
            if (!md) {
                char* cur;
                if (cur = variants[pos + i - 2]) {
                    printf("%s", cur);
                } else {
                    md = 1;
                }
            }
            printf("%s", "\e[0K");
        }

        //Get the input
        mfm_read_key(buf, &key);

        //Do the reaction to the input
        f = 0;
        if (buf[0] == 13) {
            return act;
        }
        switch (key) {
        case MFM_KEY_UP:
            act = act - 1 > 0 ? act - 1 : 0;
            break;
        case MFM_KEY_DOWN:
            act = variants[act + 1] ? act + 1 : act;
            break;
        case MFM_KEY_ESC:
            return -1;
            break;
        }
    }
}
