#define _DEFAULT_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>

void file_saving(char *filename, char buffer[][1024], int total_lines, int line_length[]);

struct winsize terminal_size;
char buffer[100][1024];
int line_length[100];
int total_lines = 1;

char status_message[512] = "Ctrl-S = save | Ctrl-Q = quit";

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

struct cursor_info cursor;
struct editor_info editor;
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

void cursor_movement(int col, int row)
{
    char movement[32];

    int len = snprintf(movement, sizeof(movement), "\x1b[%d;%dH", row, col);

    write(STDOUT_FILENO, movement, len);
}

void draw_status_bar()
{
    char bar[1024];
    char output[2048];

    snprintf(
        bar,
        sizeof(bar),
        "%s | Row: %d Col: %d | Lines: %d",
        status_message,
        cursor.cursor_y,
        cursor.cursor_x,
        total_lines
    );

    int len = strlen(bar);

    if (len > editor.terminal_cols)
    {
        len = editor.terminal_cols;
    }

    int out_len = snprintf(
        output,
        sizeof(output),
        "\x1b[%d;1H\x1b[7m%.*s\x1b[K\x1b[m\x1b[%d;%dH",
        editor.terminal_rows,
        len,
        bar,
        cursor.cursor_y,
        cursor.cursor_x
    );

    if (out_len > 0)
    {
        if (out_len >= (int)sizeof(output))
        {
            out_len = sizeof(output) - 1;
        }

        write(STDOUT_FILENO, output, out_len);
    }
}

void redraw_screen()
{
    write(STDOUT_FILENO, "\x1b[2J", 4);
    write(STDOUT_FILENO, "\x1b[H", 3);

    int text_rows = editor.terminal_rows - 1;

    for (int i = 0; i < total_lines && i < text_rows; i++)
    {
        cursor_movement(1, i + 1);
        write(STDOUT_FILENO, "\x1b[2K", 4);
        write(STDOUT_FILENO, buffer[i], line_length[i]);
    }

    draw_status_bar();

    cursor_movement(cursor.cursor_x, cursor.cursor_y);
}

void write_text(char *c)
{
    int line = cursor.cursor_y - 1;
    int col = cursor.cursor_x - 1;

    if (line_length[line] >= 1023)
    {
        return;
    }

    for (int i = line_length[line]; i > col; i--)
    {
        buffer[line][i] = buffer[line][i - 1];
    }

    buffer[line][col] = *c;

    line_length[line]++;
    buffer[line][line_length[line]] = '\0';

    write(STDOUT_FILENO, "\x1b[1@", 4);
    write(STDOUT_FILENO, c, 1);

    cursor.cursor_x++;
    cursor_movement(cursor.cursor_x, cursor.cursor_y);
}

void text()
{
    total_lines = 1;

    for (int i = 0; i < 100; i++)
    {
        line_length[i] = 0;
        buffer[i][0] = '\0';
    }

    write(STDOUT_FILENO, "\x1b[?1049h", 8);
    write(STDOUT_FILENO, "\x1b[2J", 4);
    write(STDOUT_FILENO, "\x1b[H", 3);

    cursor.cursor_x = 1;
    cursor.cursor_y = 1;

    get_terminal_size();
    editor.terminal_rows = terminal_size.ws_row;
    editor.terminal_cols = terminal_size.ws_col;

    enable_raw_mode();

    redraw_screen();

    char c;

    while (1)
    {
        int n = read(STDIN_FILENO, &c, 1);

        get_terminal_size();
        editor.terminal_rows = terminal_size.ws_row;
        editor.terminal_cols = terminal_size.ws_col;

        if (n == 1)
        {
            if (c == 17)
            {
                write(STDOUT_FILENO, "\x1b[?1049l", 8);
                disable_raw_mode();
                break;
            }
            else if (c == '\r')
            {
                if (cursor.cursor_y < editor.terminal_rows - 1 && cursor.cursor_y < 100 && total_lines < 100)
                {
                    int current_line = cursor.cursor_y - 1;
                    int new_line = current_line + 1;
                    int col = cursor.cursor_x - 1;

                    int right_length = line_length[current_line] - col;

                    for (int i = total_lines; i > new_line; i--)
                    {
                        for (int j = 0; j <= line_length[i - 1]; j++)
                        {
                            buffer[i][j] = buffer[i - 1][j];
                        }

                        line_length[i] = line_length[i - 1];
                    }

                    for (int i = 0; i < right_length; i++)
                    {
                        buffer[new_line][i] = buffer[current_line][col + i];
                    }

                    buffer[new_line][right_length] = '\0';
                    line_length[new_line] = right_length;

                    line_length[current_line] = col;
                    buffer[current_line][col] = '\0';

                    total_lines++;

                    cursor.cursor_y++;
                    cursor.cursor_x = 1;

                    for (int i = current_line; i < total_lines && i < editor.terminal_rows - 1; i++)
                    {
                        cursor_movement(1, i + 1);
                        write(STDOUT_FILENO, "\x1b[2K", 4);
                        write(STDOUT_FILENO, buffer[i], line_length[i]);
                    }

                    cursor_movement(cursor.cursor_x, cursor.cursor_y);
                }
            } 
            else if (c == 19)  // Ctrl+S
            {
                disable_raw_mode();

                cursor_movement(1, editor.terminal_rows);
                write(STDOUT_FILENO, "\x1b[2K", 4);

                char filename[256];

                printf("Save as: ");
                fflush(stdout);

                fgets(filename, sizeof(filename), stdin);

                filename[strcspn(filename, "\n")] = '\0';

                if (strlen(filename) == 0)
                {
                    snprintf(status_message, sizeof(status_message), "Save cancelled");
                }
                else
                {
                    file_saving(filename, buffer, total_lines, line_length);
                    snprintf(status_message, sizeof(status_message), "Saved %s", filename);
                }

                enable_raw_mode();

                redraw_screen();
            } 
            else if (c == 127 || c == 8)
            {
                int line = cursor.cursor_y - 1;

                if (cursor.cursor_x > 1)
                {
                    int delete_index = cursor.cursor_x - 2;

                    for (int i = delete_index; i < line_length[line]; i++)
                    {
                        buffer[line][i] = buffer[line][i + 1];
                    }

                    line_length[line]--;

                    cursor.cursor_x--;
                    cursor_movement(cursor.cursor_x, cursor.cursor_y);

                    write(STDOUT_FILENO, "\x1b[1P", 4);
                }
                else if (cursor.cursor_x == 1 && cursor.cursor_y > 1)
                {
                    int current_line = cursor.cursor_y - 1;
                    int previous_line = cursor.cursor_y - 2;

                    int old_previous_length = line_length[previous_line];

                    if (line_length[previous_line] + line_length[current_line] < 1023)
                    {
                        for (int i = 0; i < line_length[current_line]; i++)
                        {
                            buffer[previous_line][old_previous_length + i] = buffer[current_line][i];
                        }

                        line_length[previous_line] += line_length[current_line];
                        buffer[previous_line][line_length[previous_line]] = '\0';

                        for (int i = current_line; i < total_lines - 1; i++)
                        {
                            for (int j = 0; j <= line_length[i + 1]; j++)
                            {
                                buffer[i][j] = buffer[i + 1][j];
                            }

                            line_length[i] = line_length[i + 1];
                        }

                        line_length[total_lines - 1] = 0;
                        buffer[total_lines - 1][0] = '\0';

                        total_lines--;

                        cursor.cursor_y--;
                        cursor.cursor_x = old_previous_length + 1;

                        for (int i = previous_line; i < total_lines && i < editor.terminal_rows - 1; i++)
                        {
                            cursor_movement(1, i + 1);
                            write(STDOUT_FILENO, "\x1b[2K", 4);
                            write(STDOUT_FILENO, buffer[i], line_length[i]);
                        }

                        cursor_movement(1, total_lines + 1);
                        write(STDOUT_FILENO, "\x1b[2K", 4);

                        cursor_movement(cursor.cursor_x, cursor.cursor_y);
                    }
                }
            }
            else if (c == '\t')
            {
                char space = ' ';

                for (int i = 0; i < 4; i++)
                {
                    write_text(&space);
                }
            }
            else if (c == 27)
            {
                char seq[3];

                if (read(STDIN_FILENO, &seq[0], 1) != 1)
                {
                    // ESC alone
                }
                else if (read(STDIN_FILENO, &seq[1], 1) != 1)
                {
                    // ESC alone
                }    
                else
                {
                    if (seq[0] == '[')
                    {
                        if (seq[1] == 'A')
                        {
                            // UP ARROW
                            if (cursor.cursor_y > 1)
                            {
                                cursor.cursor_y--;

                                int line = cursor.cursor_y - 1;
                                if (cursor.cursor_x > line_length[line] + 1)
                                {
                                    cursor.cursor_x = line_length[line] + 1;
                                }

                                cursor_movement(cursor.cursor_x, cursor.cursor_y);
                            }
                        }
                        else if (seq[1] == 'B')
                        {
                            // DOWN ARROW
                            if (cursor.cursor_y < total_lines && cursor.cursor_y < editor.terminal_rows - 1)
                            {
                                cursor.cursor_y++;

                                int line = cursor.cursor_y - 1;
                                if (cursor.cursor_x > line_length[line] + 1)
                                {
                                    cursor.cursor_x = line_length[line] + 1;
                                }

                                cursor_movement(cursor.cursor_x, cursor.cursor_y);
                            }
                        }
                        else if (seq[1] == 'C')
                        {
                            // RIGHT ARROW
                            int line = cursor.cursor_y - 1;

                            if (cursor.cursor_x < line_length[line] + 1)
                            {
                                cursor.cursor_x++;
                            }
                            else if (cursor.cursor_x == line_length[line] + 1 && cursor.cursor_y < total_lines && cursor.cursor_y < editor.terminal_rows - 1)
                            {
                                cursor.cursor_y++;
                                cursor.cursor_x = 1;
                            }
                            cursor_movement(cursor.cursor_x, cursor.cursor_y);
                        }
                        else if (seq[1] == 'D')
                        {
                            // LEFT ARROW
                            if (cursor.cursor_x > 1)
                            {
                                cursor.cursor_x--;
                            }
                            else if (cursor.cursor_x == 1 && cursor.cursor_y > 1)
                            {
                                cursor.cursor_y--;

                                int line = cursor.cursor_y - 1;
                                cursor.cursor_x = line_length[line] + 1;
                            }

                            cursor_movement(cursor.cursor_x, cursor.cursor_y);
                        }
                        else if (seq[1] == 'H')
                        {
                            // HOME KEY
                            cursor.cursor_x = 1;
                            cursor_movement(cursor.cursor_x, cursor.cursor_y);
                        }
                        else if (seq[1] == 'F')
                        {
                            // END KEY
                            int line = cursor.cursor_y - 1;

                            cursor.cursor_x = line_length[line] + 1;
                            cursor_movement(cursor.cursor_x, cursor.cursor_y);
                        }
                        else if (seq[1] >= '0' && seq[1] <= '9')
                        {
                            if (read(STDIN_FILENO, &seq[2], 1) != 1)
                            {
                                // unknown
                            }
                            else if (seq[1] == '1' && seq[2] == '~')
                            {
                                // HOME KEY
                                cursor.cursor_x = 1;
                                cursor_movement(cursor.cursor_x, cursor.cursor_y);
                            }
                            else if (seq[1] == '4' && seq[2] == '~')
                            {
                                // END KEY
                                int line = cursor.cursor_y - 1;

                                cursor.cursor_x = line_length[line] + 1;
                                cursor_movement(cursor.cursor_x, cursor.cursor_y);
                            }
                        }
                    }
                }
            }
            else if (c >= 32 && c <= 126)
            {
                write_text(&c);
            }
            else
            {
                // printf("IDK\r\n");
            }

            draw_status_bar();

            fflush(stdout);
        }
    }
}

int main()
{
    text();

    return 0;
}