# Attention
Project has been moved here:

https://gitlab.com/dron2065/mfm

# Abstract
Minimal File Manager is a little console program written in C. POSIX only.
# Building
This program has no dependencies. For building just type:

    make
or, if there is no make in your system:

    cc *.c -o mfm -I. -O2
Then, you can put executable mfm to any $PATH directory.
# Usage
Run program:

    mfm [dir1 [dir2 [...]]]
All dirs will be opened in tabs (can be opened no more than ten tabs).
## Controls:
* Tab - create new tab
* Ctrl+W - close current tab (quit from program with closing last tab)
* Space or Insert - select/unselect current item
* \+ or = - select all items in tab
* \- or _ - unselect all items
* Home End - go to the start or to the end of file list
* PgUp PgDn - go up or down throw five items of file list
* Delete - delete current or selected files/directories
* Enter:
    * go to the directory;
    * open current or selected files
    * run current executable
* D - make new dir
* E - execute shell command with delay (expression %f will be replaced by current or selected file names, %%f will be replaced by %f)
* Ctrl-E - same, but without delay
* S - run shell
* R - rename file/directory
* U - update current list
* H - show/hide hidden files
* G - select directory from bookmarks and go to it
* Ctrl-G - go to any directory
* J - jump to directory item at current list
* Ctrl-J - jump to file item at current list
* C - copy all selected items at all tabs except current to the directory of current tab
* M - same way move items
* Q - quit from program

## Configuration files
* $HOME/.local/etc/mfm/ext - commands to be applied for files with a certain extension. Every line in file looks like: last symbols of file name=command to be applied . Example: .mkv=mpv %f --fs . %f will be replaced with file names, %%f - by %f . Also, it's possible to specify only last symbols, then will be used last specified command;
* $HOME/.local/etc/mfm/bookmarks - some directories. Used by G command
