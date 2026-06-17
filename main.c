#define _DEFAULT_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>

struct winsize terminal_size;

struct editor_info
{
    int terminal_rows;
    int terminal_cols;
};

struct cursor_info
{
    int cursor_x;
    int cursor_y;
};

struct termios original_terminal;

void disable_raw_mode()
{
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &original_terminal);
}

void enable_raw_mode()
{
    tcgetattr(STDIN_FILENO, &original_terminal);
    struct termios raw = original_terminal;
    cfmakeraw(&raw);
	raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 1;
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

void get_terminal_size()
{
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &terminal_size);
}
void cursor_movement(int col, int row){
	char movement[32];

	int len = snprintf(movement, sizeof(movement), "\x1b[%d;%dH", row, col);
    write(STDOUT_FILENO, movement, len);
}
char buffer[100][1024];
int total_lines[100];
int no_of_line[100];
struct cursor_info cursor;
struct editor_info editor;
void write_text(char *c){
    buffer[cursor.cursor_y-1][cursor.cursor_x-1] = *c;
	write(STDOUT_FILENO, c, 1);
	cursor.cursor_x++;
	cursor_movement(cursor.cursor_x, cursor.cursor_y);
}

void text()
{	
	write(STDOUT_FILENO, "\x1b[2J", 4);  // clear whole screen
	write(STDOUT_FILENO, "\x1b[H", 3);   // move cursor to top-left
	get_terminal_size();
	editor.terminal_rows = terminal_size.ws_row;
	editor.terminal_cols = terminal_size.ws_col;
	cursor.cursor_x = 1;
	cursor.cursor_y = 1;
	for (int i = 0; i < 100; i++)
	{
		total_lines[i] = i+1;
	}
	
    enable_raw_mode();

    char c;

    while (1)
    {
        int n = read(STDIN_FILENO, &c, 1);

        if (n == 1)
        {

            if (c == 17)
            {
                disable_raw_mode();
                break;
            }
            else if (c == '\r')
            {
                // printf("ENTER\r\n");
            }
            else if (c == 127 || c == 8)
            {
                // printf("BACKSPACE\r\n");
                 if (cursor.cursor_x > 1)
                {
                    cursor.cursor_x--;
                    buffer[cursor.cursor_y - 1][cursor.cursor_x - 1] = '\0';
                    cursor_movement(cursor.cursor_x, cursor.cursor_y);

                    char space = ' ';
                    write(STDOUT_FILENO, &space, 1);

                    cursor_movement(cursor.cursor_x, cursor.cursor_y);
                }
            }
            else if (c == '\t')
            {
                // printf("TAB\r\n");
            }
            else if (c == 27)
            {
                char seq[2];

                if (read(STDIN_FILENO, &seq[0], 1) != 1)
                {
                    // printf("ESC\r\n");
                }
                else if (read(STDIN_FILENO, &seq[1], 1) != 1)
                {
                    // printf("ESC\r\n");
                }
                else if (seq[0] == '[' && seq[1] == 'A')
                {
                    // printf("UP ARROW\r\n");
					if (cursor.cursor_y > 1)
					{
						cursor.cursor_y--;
						cursor_movement(cursor.cursor_x,cursor.cursor_y);
					}
                }
                else if (seq[0] == '[' && seq[1] == 'B')
                {
                    // printf("DOWN ARROW\r\n");
						cursor.cursor_y++;
						cursor_movement(cursor.cursor_x,cursor.cursor_y);

                }
                else if (seq[0] == '[' && seq[1] == 'C')
                {
                    // printf("RIGHT ARROW\r\n");
					if (cursor.cursor_x < editor.terminal_cols)
					{
						cursor.cursor_x++;
						cursor_movement(cursor.cursor_x,cursor.cursor_y);
					}
					
                }
                else if (seq[0] == '[' && seq[1] == 'D')
                {
                    // printf("LEFT ARROW\r\n");
					if (cursor.cursor_x > 1)
					{
						cursor.cursor_x--;
						cursor_movement(cursor.cursor_x,cursor.cursor_y);
					}
					
                }
                else
                {
                    // printf("UNKNOWN ESCAPE KEY\r\n");
                }
            }
            else if (c >= 32 && c <= 126)
            {
                // printf("NORMAL KEY: %c\r\n", c);
				write_text(&c);
            }
            else
            {
                // printf("IDK\r\n");
            }

            fflush(stdout);
        }
    }
}

int main()
{
    text();

    return 0;
}