#include<stdio.h>
#include<assert.h>
#include<errno.h>
#include<string.h>
#include<stdlib.h>
#include<stdbool.h>

#include "sv.h"
#include "editor.h"

#define LINE_INIT_CAPACITY 1024
#define EDITOR_INIT_CAPACITY 128

static void editor_create_first_new_line(Editor *editor);

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

/*
* Appends a NULL terminated text at the end of line
*/
void line_append_text(Line *line, const char *text) {
  line_append_text_sized(line, text, strlen(text));
}

/*
* Appends a sized text at the end of line
*/
void line_append_text_sized(Line *line, const char *text, size_t text_size) {
  size_t col = line->size;
  line_insert_text_sized_before(line, text, &col, text_size);
}

/**
* * Appends sized text anywhere in the line
*/
void line_insert_text_sized_before(Line *line,
                                   const char *text,
                                   size_t *col,
                                   size_t text_size) {
  // * Edge case (check col fits into the line)
  if (*col > line->size) {
    *col = line->size;
  }

  line_grow(line, text_size);

  // * move chunk from buffer_cursor to buffer_size 
  size_t move_chunk_size = line->size - *col;

  // * First shift the chunk by `move_chunk_size`
  memmove(line->chars + *col + text_size,  // * destination address
          line->chars + *col,              // * source address
          move_chunk_size);

  memcpy(line->chars + *col, text, text_size);
  line->size += text_size; // * increase the line current size
  *col += text_size;       // * increase the cursor column index
}

/** 
* * Appends NULL terminated text anywhere in the line
*/
void line_insert_text_before(Line *line,
                             const char *text,
                             size_t *col) {
  line_insert_text_sized_before(line, text, col, strlen(text));
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

static void editor_create_first_new_line(Editor *editor) {
  if (editor->cursor_row >= editor->size) {
    if (editor->size > 0) { // * go to the last row
      editor->cursor_row = editor->size - 1;
    } else { // * insert new line into editor
      editor_grow(editor, 1);
      // * zero initialize the new line 
      memset(&editor->lines[editor->size], 0, sizeof(editor->lines[0]));
      editor->size += 1;
    }
  } 
}

/*
* insert the text in the `lines` using `cursor_row`
*/
void editor_insert_text_before_cursor(Editor *editor, const char *text) {
  editor_create_first_new_line(editor);
  line_insert_text_before(&editor->lines[editor->cursor_row], text, &editor->cursor_col);
}

/*
* Backspace on particular line based on cursor_row & cursor_col
*/
void editor_backspace(Editor *editor) {
  editor_create_first_new_line(editor);
  line_backspace(&editor->lines[editor->cursor_row], &editor->cursor_col);
}

/*
* Delete on particular line based on cursor_row & cursor_col
*/
void editor_delete(Editor *editor) {
  editor_create_first_new_line(editor);
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

void editor_save_to_file(const Editor *editor, const char *file_path) {
  // * open the file
  FILE *f = fopen(file_path, "w");
  if(f == NULL) {
    fprintf(stderr, "ERROR: could not open file `%s`: %s\n", file_path, strerror(errno));
    exit(1);
  }

  for (size_t row = 0; row < editor->size; ++row) {
    fwrite(editor->lines[row].chars, 1, editor->lines[row].size, f);
    fputc('\n', f);
  }

  fclose(f);
}

void editor_load_from_file(Editor *editor , FILE *f) {
  assert(editor->lines == NULL && "You can only load files into an empty editor");

  // * Create a empty new line first
  editor_create_first_new_line(editor);

  // FILE *f = fopen(file_path, "r");
  // if (f == NULL) {
  //   fprintf(stderr, "ERROR: could not open file `%s`:%s\n", file_path, strerror(errno));
  //   exit(1);
  // }

  static char chunk[640 * 1024];

  while (!feof(f)) {
    size_t n = fread(chunk, 1, sizeof(chunk), f);
    String_View chunk_sv = {
      .data = chunk,
      .count = n
    };


    while (chunk_sv.count > 0) {
      Line *line = &editor->lines[editor->size - 1];
      String_View chunk_line = {0};
      if (sv_try_chop_by_delim(&chunk_sv, '\n', &chunk_line)) {
        line_append_text_sized(line, chunk_line.data, chunk_line.count);
        editor_insert_new_line(editor);
      } else {
        line_append_text_sized(line, chunk_sv.data, chunk_sv.count);
        chunk_sv = SV_NULL;
      }
    }

  }

  editor->cursor_row = 0;
}
