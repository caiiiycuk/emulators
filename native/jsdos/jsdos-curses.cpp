#include <jsdos-curses.h>

WINDOW* stdscr = 0;

bool has_colors (void) {
    return false;
}

int COLOR_PAIR(int) {
    return 0;
}

int wattrset(WINDOW *, int) {
    return 0;
}

int mvwprintw(WINDOW*,int,int, const char *,...) {
    return 0;
}

int wclrtoeol (WINDOW *) {
    return 0;
}

void wbkgdset(WINDOW *,chtype) {

}

int getmaxx (const WINDOW *) {
    return 0;
}

int getmaxy (const WINDOW *) {
    return 0;
}


int waddstr(WINDOW *,const char *) {
    return 0;
}

int wprintw (WINDOW *, const char *,...) {
    return 0;
}

int wrefresh (WINDOW *) {
    return 0;
}

int endwin(void) {
    return 0;
}

int curs_set(int) {
    return 0;
}

int wclear(WINDOW *) {
    return 0;
}

int mvwaddch(WINDOW *, int, int, const chtype) {
    return 0;
}

char killchar(void) {
    return 0;
}

int mvwchgat(WINDOW *, int, int, int, attr_t, NCURSES_PAIRS_T, const void *) {
    return 0;
}

int getch (void) {
    return 0;
}

int wmove(WINDOW *,int,int) {
    return 0;
}

int waddnstr(WINDOW *,const char *,int) {
    return 0;
}

int init_pair(NCURSES_PAIRS_T,NCURSES_COLOR_T,NCURSES_COLOR_T) {
    return 0;
}

int wtouchln (WINDOW *,int,int,int) {
    return 0;
}

int scrollok (WINDOW *,bool) {
    return 0;
}

int start_color (void) {
    return 0;
}

int wscrl (WINDOW *,int) {
    return 0;
}

int keypad (WINDOW *,bool) {
    return 0;
}

int nodelay (WINDOW *,bool) {
    return 0;
}

int noecho (void) {
    return 0;
}

int cbreak (void) {
    return 0;
}

WINDOW *initscr (void) {
    return 0;
}

int refresh (void) {
    return 0;
}

WINDOW * subwin(WINDOW *, int, int, int, int) {
    return 0;
}

int werase(WINDOW *) {
    return 0;
}

int delwin(WINDOW *) {
    return 0;
}

int getbegy(const WINDOW *) {
    return 0;
}

int getbegx (const WINDOW *) {
    return 0;
}

int whline(WINDOW *, chtype, int) {
    return 0;
}

void DISP2_SetPageHandler(void) {}
void DISP2_RegisterPorts(void) {}
bool DISP2_Active(void) {}
void DISP2_Shut() {}
