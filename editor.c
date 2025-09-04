#include<stdio.h>
#include<string.h>
#include<assert.h>
#include<stdlib.h>

#include "editor.h"

#define LINE_INIT_CAPACITY 1024
#define EDITOR_INIT_CAPACITY 128

static void line_grow(Line *line, size_t n) {
  size_t new_capacity = line->capacity;
  // printf("new_capacity: %zu, line->size: %zu\n", new_capacity, line->size);

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

void line_insert_text_before(Line *line, const char *text, size_t *col) {

  // * Edge case (check col fits into the line)
  if (*col > line->size) {
    *col = line->size;
  }

  const size_t text_size = strlen(text);
  line_grow(line, text_size);

  // * move chunk from buffer_cursor to buffer_size 
  size_t move_chunk_size = line->size - *col;

  // * First shift the chunk by `move_chunk_size`
  memmove(line->chars + *col + text_size,  // * destination address
          line->chars + *col,              // * source address
          move_chunk_size);

  memcpy(line->chars + *col, text, text_size);
  line->size += text_size;
  *col += text_size;

}

void line_backspace(Line *line, size_t *col) {

  // * Edge case (check col fits into the line)
  if (*col > line->size) {
    *col = line->size;
  }

  if (line->size > 0 && *col > 0) {
    // * shift whole chunk to left by 1
    memmove(line->chars + *col - 1,   // * destination address
            line->chars + *col,       // * source address
            line->size - *col); // * chunk size

    line->size -= 1;
    *col -= 1;
  }
}

void line_delete(Line *line, size_t *col) {

  // * Edge case (check col fits into the line)
  if (*col > line->size) {
    *col = line->size;
  }

  if (*col < line->size && line->size > 0) {
    // * shift whole chunk to left by 1
    memmove(line->chars + *col,     // * destination address
            line->chars + *col + 1, // * source address
            line->size - *col);     // * chunk size

    line->size -= 1;
  }
}

static void editor_grow(Editor *editor, size_t n) {
  size_t new_capacity = editor->capacity;
  while (new_capacity - editor->size < n) {
    if(new_capacity == 0) {
      new_capacity = EDITOR_INIT_CAPACITY;
    } else {
      new_capacity *= 2;
    }
  }

  if (new_capacity != editor->capacity) {
    editor->lines = realloc(editor->lines, new_capacity * sizeof(editor->lines[0]));
    editor->capacity = new_capacity;
  }
}

/*
* insert a new line into lines buffer
*/
void editor_insert_new_line(Editor *editor) {
  if (editor->cursor_row > editor->size) {
    editor->cursor_row = editor->size;
  }

  // * Add one extra line in editor
  editor_grow(editor, 1);

  // * Move the lines from cursor_row
  size_t line_size = sizeof(editor->lines[0]);
  memmove(editor->lines + editor->cursor_row + 1,
          editor->lines + editor->cursor_row,
          (editor->size - (editor->cursor_row)) * line_size);

  // * fill the new line with zero
  editor->cursor_row += 1;
  editor->cursor_col = 0;
  memset(&editor->lines[editor->cursor_row], 0, line_size);
  editor->size += 1;
}

/*
* Push a new line into lines buffer
*/
void editor_push_new_line(Editor *editor) {
  editor_grow(editor, 1);
  // * zero initialize the new line 
  memset(&editor->lines[editor->size], 0, sizeof(editor->lines[0]));
  editor->size += 1;
}

/*
* insert the text in the `lines` using `cursor_row`
*/
void editor_insert_text_before_cursor(Editor *editor, const char *text) {
  // * check overflow condition
  if (editor->cursor_row >= editor->size) {
    if (editor->size > 0) { // * go to the last row
      editor->cursor_row = editor->size - 1;
    } else { // * insert new line into editor
      editor_push_new_line(editor);
    }
  }

  line_insert_text_before(&editor->lines[editor->cursor_row], text, &editor->cursor_col);
}

/*
* Backspace on particular line based on cursor_row & cursor_col
*/
void editor_backspace(Editor *editor) {
  // * check overflow condition
  if (editor->cursor_row >= editor->size) {
    if (editor->size > 0) { // * go to the last row
      editor->cursor_row = editor->size - 1;
    } else { // * insert new line into editor
      editor_push_new_line(editor);
    }
  }

  line_backspace(&editor->lines[editor->cursor_row], &editor->cursor_col);
}

/*
* Delete on particular line based on cursor_row & cursor_col
*/
void editor_delete(Editor *editor) {
  // * check overflow condition
  if (editor->cursor_row >= editor->size) {
    if (editor->size > 0) { // * go to the last row
      editor->cursor_row = editor->size - 1;
    } else { // * insert new line into editor
      editor_push_new_line(editor);
    }
  }

  line_delete(&editor->lines[editor->cursor_row], &editor->cursor_col);
}

/*
* Returns the current character under the cursor
*/
const char *editor_char_under_cursor(const Editor *editor) {
  if(editor->cursor_row < editor->size) {
    if (editor->cursor_col < editor->lines[editor->cursor_row].size) {
      return &editor->lines[editor->cursor_row].chars[editor->cursor_col];
    }
  }
  return NULL;
}
