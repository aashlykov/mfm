#ifndef MFM_TAB
#define MFM_TAB

//Properties for menu item
typedef enum
{
    MFM_DIR = 1, //Is it a directory?
    MFM_REG = 2, //Is it ususal file?
    MFM_SEL = 4, //Is it selected?
    MFM_EXE = 8  //Executable?
} mfm_tab_item_props;

//Menu item
typedef struct
{
    mfm_tab_item_props props; //Properties
    char* text;               //Text of item
} mfm_tab_item;

//One tab
typedef struct
{
    mfm_tab_item* items;  //Array of menu items
    char* dir;            //Directory of menu
    int act;              //Current active element
    int pos_view;         //Position of viewing menu
    int len;              //Count of elements
    int hidden;           //Show hidden elements?
} mfm_tab;

//Create the menu
int mfm_init_tab(mfm_tab* tab, void** f_cmd);

//Draw the menu
void mfm_draw_tab(
    mfm_tab* tab, //Menu to drawing
    int h,        //Screen size
    int w
);

//Destroy the menu
void mfm_destroy_tab(mfm_tab* tab);

//Correct menu
void mfm_correct_tab(mfm_tab* tab, int h, int w);

#endif
