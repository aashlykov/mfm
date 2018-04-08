#ifndef MFM_INPUT_H
#define MFM_INPUT_H

/**
 * Special keys
 */
typedef enum
{
    MFM_KEY_NULL   = -1, /**< For cases when key was not recognized */
    MFM_KEY_ESC    =  0,
    MFM_KEY_F1     =  1,
    MFM_KEY_F2     =  2,
    MFM_KEY_F3     =  3,
    MFM_KEY_F4     =  4,
    MFM_KEY_F5     =  5,
    MFM_KEY_F6     =  6,
    MFM_KEY_F7     =  7,
    MFM_KEY_F8     =  8,
    MFM_KEY_F9     =  9,
    MFM_KEY_F10    = 10,
    MFM_KEY_F11    = 11,
    MFM_KEY_F12    = 12,
    MFM_KEY_UP     = 13,
    MFM_KEY_DOWN   = 14,
    MFM_KEY_LEFT   = 15,
    MFM_KEY_RIGHT  = 16,
    MFM_KEY_INSERT = 17,
    MFM_KEY_DELETE = 18,
    MFM_KEY_HOME   = 19,
    MFM_KEY_END    = 20,
    MFM_KEY_PGUP   = 21,
    MFM_KEY_PGDN   = 22
} mfm_key;

/**
 * Read key
 * @param buf Place to put raw key code
 * @param key Place to put apecial key flag
 */
void mfm_read_key(char* buf, mfm_key* key);

/**
 * Read the line
 * @param y Position on screen
 * @param x
 * @param w Available length
 * @param exist Current string to edit - can be NULL
 * @return String from user or NULL if user cancelled the input by ESCAPE key. MUST call free() on result after using
 */
char* mfm_read_line(
    int y,
    int x,
    int w,
    unsigned char* exist
);

#endif /* MFM_INPUT_H */
