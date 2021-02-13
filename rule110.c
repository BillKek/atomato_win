#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "./atomato.h"

typedef enum {
    O = 0,
    I = 1,
} Cell;

Uint32 cell_color[2] = {
    [O] = 0x00000000,
    [I] = 0xFFAABBFF,
};

#define PATTERN(A, B, C) ((A << 2) | (B << 1) | C)

Cell patterns[1 << 3] = {
    [PATTERN(O, O, O)] = O,
    [PATTERN(O, O, I)] = I,
    [PATTERN(O, I, O)] = I,
    [PATTERN(O, I, I)] = I,
    [PATTERN(I, O, O)] = O,
    [PATTERN(I, O, I)] = I,
    [PATTERN(I, I, O)] = I,
    [PATTERN(I, I, I)] = O,
};

typedef struct {
    Cell cells[COLS];
} Row;

void render_row(Row row, int y)
{
    for (int i = 0; i < COLS; ++i) {
        atomato_fill_rect(
            i * CELL_WIDTH,
            y,
            CELL_WIDTH,
            CELL_HEIGHT,
            cell_color[row.cells[i]]);
    }
}

Row next_row(Row prev)
{
    Row next = {0};

    // N = 5
    // 01234
    // *****
    //  ^

    for (int i = 0; i < COLS; ++i) {
        const int index = PATTERN(prev.cells[mod(i - 1, COLS)],
                                  prev.cells[i],
                                  prev.cells[mod(i + 1, COLS)]);
        next.cells[i] = patterns[index];
    }

    return next;
}

Row random_row(void)
{
    Row result = {0};

    for (int i = 0; i < COLS; ++i) {
        result.cells[i] = rand() % 2;
    }

    return result;
}

typedef struct {
    Row rows[ROWS];
    int begin;
    int size;
} Board;

Board board = {0};

void board_push_row(Board *board, Row row)
{
    board->rows[mod(board->begin + board->size, ROWS)] = row;

    if (board->size < ROWS) {
        board->size += 1;
    } else {
        board->begin = mod(board->begin + 1, ROWS);
    }
}

void board_next_row(Board *board)
{
    board_push_row(board, next_row(board->rows[mod(board->begin + board->size - 1, ROWS)]));
}

void board_render(const Board *board)
{
    for (int row = 0; row < board->size; ++row) {
        render_row(board->rows[mod(board->begin + row, ROWS)], row * CELL_HEIGHT);
    }
}

int main(void)
{
    atomato_begin();

    board_push_row(&board, random_row());

    while (!atomato_time_to_quit()) {
        // Handle Inputs
        atomato_poll_events(NULL);

        // Update State
        if (atomato_is_next_gen()) {
            board_next_row(&board);
        }

        // Render State
        atomato_begin_rendering();
        {
            board_render(&board);
        }
        atomato_end_rendering();
    }

    // TODO: pause on space

    atomato_end();

    return 0;
}
