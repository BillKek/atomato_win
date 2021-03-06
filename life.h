// # Life
//
// Here we keep the common code for all of the Cellular Atomata
// from the Game of Life Family:
// https://en.wikipedia.org/wiki/Life-like_cellular_automaton

#ifndef LIFE_H_
#define LIFE_H_

#include <errno.h>
#include "square.h"

#define STB_IMAGE_IMPLEMENTATION
#include "./stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "./stb_image_write.h"

typedef int8_t Cell;

typedef struct
{
    Cell cells[ROWS*(COLS+1)]; // tail for opt.leadout
} Board;

inline int life_board_nbors(Cell* cells, int row0, int col0, Cell cell)
{
    int result = 0;
    for (int drow = -1; drow <= 1; ++drow)
    {
        for (int dcol = -1; dcol <= 1; ++dcol)
        {
            if (drow != 0 || dcol != 0)
            {
                const int row = mod(row0 + drow, ROWS);
                const int col = mod(col0 + dcol, COLS);

                if (cells[iter_cell(col,row)] == cell)
                {
                    result += 1;
                }
            }
        }
    }
    return result;
}

void life_random_board(Board *board, int cell_state)
{
    for (int row = 0; row < ROWS; ++row)
    {
        for (int col = 0; col < COLS; ++col)
        {
            board->cells[iter_cell(col,row)] = rand() % cell_state;
        }
    }
}

typedef void (*Life_Rule)(Cell* prev, Cell* next);

typedef struct
{
    Square square;
    Board boards[2];
    int board_current;
    Life_Rule rule;
    int cells_count;
    const Color *cells_color;
} Life;

void life_copy_shape_to(Board *board,
                        int row0, int col0,
                        int rows, int cols,
                        const Cell shape[rows][cols])
{
    for (int drow = 0; drow < rows; ++drow)
    {
        for (int dcol = 0; dcol < cols; ++dcol)
        {
            const int row = mod(row0 + drow, ROWS);
            const int col = mod(col0 + dcol, COLS);
            board->cells[iter_cell(col,row)] = shape[drow][dcol];
        }
    }
}

void life_save_board_to_image(const Board *board, const char *file_path,
                              int cells_count,
                              const Color cells_color[cells_count])
{
    static uint32_t data[ROWS][COLS] = {0};

    for (int row = 0; row < ROWS; ++row)
    {
        for (int col = 0; col < COLS; ++col)
        {
            data[row][col] = cells_color[board->cells[iter_cell(col,row)]];
        }
    }

    if (!stbi_write_png(file_path, COLS, ROWS, 4, data, COLS * 4))
    {
        fprintf(stderr, "ERROR: could not save to file %s: %s",
                file_path, strerror(errno));
        exit(1);
    }
}

void life_load_board_from_image(Board *board, const char *file_path,
                                int cells_count,
                                const Color cells_color[cells_count])
{
    int w = 0, h = 0, n = 0;
    uint32_t *data = (uint32_t *) stbi_load(file_path, &w, &h, &n, 0);

    if (data == NULL)
    {
        fprintf(stderr, "ERROR: could not read file %s: %s",
                file_path, strerror(errno));
        exit(1);
    }

    assert(n == 4);
    assert(COLS >= w);
    assert(ROWS >= h);

    for (int row = 0; row < ROWS; ++row)
    {
        for (int col = 0; col < COLS; ++col)
        {
            const Color color = data[row * w + col];

            bool found = false;
            for (Cell cell = 0; !found && cell < cells_count; ++cell)
            {
                if (cells_color[cell] == color)
                {
                    board->cells[iter_cell(col,row)] = cell;
                    found = true;
                }
            }

            if (!found)
            {
                fprintf(stderr, "%s: Unknown color 0x%08X\n", file_path, color);
                exit(1);
            }
        }
    }
}

static Life context = {0}; // stack overflow (lit) when inside life_go()
void life_go(const Board *init_board,
             Life_Rule rule,
             int cells_count,
             const Color cells_color[cells_count],
             const char *save_file_path)
{

    square_begin(&context.square);
    context.board_current = 0;
    memcpy(&context.boards[context.board_current], init_board, sizeof(*init_board));
    context.rule = rule;
    context.cells_count = cells_count;
    context.cells_color = cells_color;

    while (!square_time_to_quit(&context.square))
    {
        if (context.square.cell_clicked)
        {
            int const row = context.square.cell_row;
            int const col = context.square.cell_col;
            Board *const board = &context.boards[context.board_current];
            board->cells[iter_cell(col,row)] = mod(board->cells[iter_cell(col,row)] + 1, context.cells_count);
        }

        if (context.square.core.keyboard['r'])
        {
            memset(&context.boards[context.board_current], 0, sizeof(context.boards[context.board_current]));

        }

        if (context.square.core.keyboard['s'])
        {
            life_save_board_to_image(
                &context.boards[context.board_current],
                save_file_path,
                context.cells_count,
                context.cells_color);
        }

        if (context.rule)
        {
            const size_t count = square_next_gen_count(&context.square);

            for (size_t i = 0; i < count*30; ++i)
            {
                Board *prev = &context.boards[context.board_current];
                Board *next = &context.boards[1 - context.board_current];

                context.rule(prev->cells, next->cells);

                context.board_current = 1 - context.board_current;
            }
        }

        square_begin_rendering(&context.square);
        {
            const Board *board = &context.boards[context.board_current];
            for (int row = 0; row < ROWS; ++row)
            {
                for (int col = 0; col < COLS; ++col)
                {
                    square_fill_cell(
                        &context.square,
                        row, col,
                        context.cells_color[board->cells[iter_cell(col,row)]]);
                }
            }
        }
        square_end_rendering(&context.square);
    }

    square_end(&context.square);
}

#endif // LIFE_H_
