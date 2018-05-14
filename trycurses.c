#include <curses.h>

int main(void) {
  initscr();
  timeout(-1);
  int c = getch();
  endwin();
  printf ("%d %c\n", c, c);
  return 0;
}
