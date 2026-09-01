#ifndef JSDOS_CURSES_H_
#define JSDOS_CURSES_H_

#undef	ERR
#define ERR     (-1)

#define TRUE    1
#define FALSE   0

#define KEY_END		0550		/* end key */
#define KEY_LEFT	0404		/* left-arrow key */
#define KEY_RIGHT	0405		/* right-arrow key */
#define KEY_F(n)	(1400+(n))
#define KEY_BTAB	0541		/* back-tab key */
#define KEY_BACKSPACE	0407		/* backspace key */
#define KEY_HOME	0406		/* home key */
#define KEY_DC		0512		/* delete-character key */
#define KEY_UP		0403		/* up-arrow key */
#define KEY_DOWN	0402		/* down-arrow key */
#define KEY_NPAGE	0522		/* next-page key */
#define KEY_RESIZE	0632		/* Terminal resize event */
#define KEY_IC		0513		/* insert-character key */
#define KEY_PPAGE	0523		/* previous-page key */

#define COLOR_BLACK	0
#define COLOR_RED	1
#define COLOR_GREEN	2
#define COLOR_YELLOW	3
#define COLOR_BLUE	4
#define COLOR_MAGENTA	5
#define COLOR_CYAN	6
#define COLOR_WHITE	7

#define	NCURSES_PAIRS_T short
#define	NCURSES_COLOR_T short

/* Перемещает курсор в stdscr и выводит строку. */
#define mvaddstr(y,x,str)		mvwaddstr(stdscr,(y),(x),(str))
/* Перемещает курсор в окне и выводит всю строку. */
#define mvwaddstr(win,y,x,str)		(wmove((win),(y),(x)) == ERR ? ERR : waddnstr((win),(str),-1))
/* Помечает все строки окна как измененные, чтобы их перерисовали. */
#define touchwin(win)		wtouchln((win), 0, getmaxy(win), 1)
/* Записывает высоту и ширину окна в переданные переменные. */
#define getmaxyx(win,y,x)	(y = getmaxy(win), x = getmaxx(win))
/* Записывает координаты верхнего левого угла окна в переданные переменные. */
#define getbegyx(win,y,x)	(y = getbegy(win), x = getbegx(win))
/* Прокручивает окно на одну строку вверх. */
#define scroll(win)		wscrl(win,1)
/* Устанавливает текущие атрибуты вывода для stdscr. */
#define attrset(at)		wattrset(stdscr,(at))
/* Рисует горизонтальную линию в stdscr с указанной позиции. */
#define mvhline(y,x,c,n)		mvwhline(stdscr,(y),(x),(c),(n))
/* Перемещает курсор в окне и рисует горизонтальную линию. */
#define mvwhline(win,y,x,c,n)		(wmove((win),(y),(x)) == ERR ? ERR : whline((win),(c),(n)))

#define ACS_S1		0x23BA

typedef int WINDOW;
typedef unsigned chtype;
typedef	chtype	attr_t;		/* ...must be at least as wide as chtype */

extern WINDOW* stdscr;

/* Возвращает true, если терминальный слой поддерживает цвета. */
bool has_colors (void);

/* Возвращает атрибут цветовой пары для использования в attrset/wattrset. */
int COLOR_PAIR(int);

/* Устанавливает текущие атрибуты вывода для указанного окна. */
int wattrset(WINDOW *, int);

/* Перемещает курсор в окне и печатает форматированную строку. */
int mvwprintw(WINDOW*,int,int, const char *,...);

/* Очищает строку от текущей позиции курсора до правого края окна. */
int wclrtoeol (WINDOW *);

/* Задает фоновый символ и атрибуты для последующего вывода в окно. */
void wbkgdset(WINDOW *,chtype);

/* Возвращает ширину окна в символах. */
int getmaxx (const WINDOW *);

/* Возвращает высоту окна в символах. */
int getmaxy (const WINDOW *);

/* Добавляет строку в окно, начиная с текущей позиции курсора. */
int waddstr(WINDOW *,const char *);

/* Печатает форматированную строку в окно с текущей позиции курсора. */
int wprintw (WINDOW *, const char *,...);

/* Сбрасывает накопленные изменения окна на экран. */
int wrefresh (WINDOW *);

/* Завершает curses-режим и освобождает связанные с ним ресурсы. */
int endwin(void);

/* Показывает, скрывает или меняет видимость курсора. */
int curs_set(int);

/* Очищает содержимое окна и помечает его для перерисовки. */
int wclear(WINDOW *);

/* Добавляет символ с атрибутами в указанную позицию окна. */
int mvwaddch(WINDOW *, int, int, const chtype);

/* Возвращает символ, который терминал считает символом удаления строки. */
char killchar(void);

/* Меняет атрибуты участка строки в указанной позиции окна. */
int mvwchgat(WINDOW *, int, int, int, attr_t, NCURSES_PAIRS_T, const void *);

/* Читает очередную клавишу из стандартного окна. */
int getch (void);

/* Перемещает курсор внутри окна. */
int wmove(WINDOW *,int,int);

/* Добавляет в окно не больше указанного числа символов из строки. */
int waddnstr(WINDOW *,const char *,int);

/* Инициализирует цветовую пару: номер пары, цвет текста и цвет фона. */
int init_pair(NCURSES_PAIRS_T,NCURSES_COLOR_T,NCURSES_COLOR_T);

/* Помечает диапазон строк окна как измененный или неизмененный. */
int wtouchln (WINDOW *,int,int,int);

/* Включает или выключает прокрутку окна при выводе за нижнюю границу. */
int scrollok (WINDOW *,bool);

/* Инициализирует поддержку цветов. */
int start_color (void);

/* Прокручивает содержимое окна на указанное число строк. */
int wscrl (WINDOW *,int);

/* Включает или выключает распознавание специальных клавиш для окна. */
int keypad (WINDOW *,bool);

/* Включает или выключает неблокирующее чтение клавиш для окна. */
int nodelay (WINDOW *,bool);

/* Отключает автоматическое эхо вводимых символов на экран. */
int noecho (void);

/* Переводит терминал в cbreak-режим: ввод доступен посимвольно. */
int cbreak (void);

/* Инициализирует curses-режим и возвращает стандартное окно. */
WINDOW *initscr (void);

/* Обновляет стандартное окно на экране. */
int refresh (void);

/* Создает дочернее окно с заданным размером и позицией. */
WINDOW * subwin(WINDOW *, int, int, int, int);

/* Стирает содержимое окна без обязательного немедленного обновления экрана. */
int werase(WINDOW *);

/* Удаляет окно и освобождает связанные с ним ресурсы. */
int delwin(WINDOW *);

/* Возвращает Y-координату верхнего левого угла окна. */
int getbegy(const WINDOW *);

/* Возвращает X-координату верхнего левого угла окна. */
int getbegx (const WINDOW *);

/* Рисует горизонтальную линию из указанного символа в текущей позиции окна. */
int whline(WINDOW *, chtype, int);

#endif
