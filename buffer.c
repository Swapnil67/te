#include<stdio.h>
#include<string.h>
#include<assert.h>
#include<stdlib.h>

#include "buffer.h"

#define LINE_INIT_CAPACITY 1024

static void line_grow(Line *line, size_t n) {
  size_t new_capacity = line->capacity;
  // printf("new_capacity: %zu\n", new_capacity);
  // printf("line->size: %zu\n", line->size);

  assert(new_capacity >= line->size);
  
  while (new_capacity - line->size < n) {
    if (new_capacity == 0) {
      new_capacity = LINE_INIT_CAPACITY;
    } else {
      new_capacity *= 2;
    }
  }

  // * only realloc if not possible to fit in current capacity
  if (new_capacity != line->capacity) {
    line->chars = realloc(line->chars, new_capacity);
    line->capacity = new_capacity;
  }
}

void line_insert_text_before(Line *line, const char *text, size_t col) {
  const size_t text_size = strlen(text);
  line_grow(line, text_size);

  // * move chunk from buffer_cursor to buffer_size 
  size_t move_chunk_size = line->size - col;
  // printf("move_chunk_size %zu \n", move_chunk_size);
  // if (free_space > (move_chunk_size + text_size)) {
  // * Buffer is full
  // }

  // * First shift the chunk by `move_chunk_size`
  memmove(line->chars + col + text_size,  // * destination address
          line->chars + col,              // * source address
          move_chunk_size);

  memcpy(line->chars + col, text, text_size);
  line->size += text_size;
}

void line_backspace(Line *line, size_t col) {
  if (line->size > 0 && col > 0) {
    // * shift whole chunk to left by 1
    memmove(line->chars + col - 1,   // * destination address
            line->chars + col,       // * source address
            line->size - col); // * chunk size

    line->size -= 1;
  }
}

void line_delete(Line *line, size_t col) {
  if (col < line->size && line->size > 0) {
    // * shift whole chunk to left by 1
    memmove(line->chars + col,     // * destination address
            line->chars + col + 1, // * source address
            line->size - col);     // * chunk size

    line->size -= 1;
  }
}