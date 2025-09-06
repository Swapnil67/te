#ifndef EDITOR_H_
#define EDITOR_H_

#include <stdlib.h>

typedef struct {
  size_t capacity;    /* current line characters capacity */
  size_t size;        /* current line characters count    */
  char *chars;        /* buffer pointer                   */
} Line;

void line_append_text(Line *line, const char *text);
void line_append_text_sized(Line *line, const char *text, size_t text_size);
void line_insert_text_before(Line *line, const char *text, size_t *col);
void line_insert_text_sized_before(Line *line, const char *text, size_t *col, size_t text_size);
void line_backspace(Line *line, size_t *col);
void line_delete(Line *line, size_t *col);

// * High level editor structure
typedef struct {
  size_t capacity;       /* current line capacity */
  size_t size;           /* current line count    */
  Line *lines;           /* line buffer           */
  size_t cursor_row;     /* cursor row index      */
  size_t cursor_col;     /* cursor col index      */
} Editor;

void editor_insert_new_line(Editor *editor);
void editor_insert_text_before_cursor(Editor *editor, const char *text);
void editor_backspace(Editor *editor);
void editor_delete(Editor *editor);
const char *editor_char_under_cursor(const Editor *editor);

void editor_save_to_file(const Editor *editor, const char *file_path);
void editor_load_from_file(Editor *editor, const char *file_path);

#endif // * EDITOR_H_

