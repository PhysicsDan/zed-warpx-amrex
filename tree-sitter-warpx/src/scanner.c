// External scanner for tree-sitter-warpx.
//
// 1. Parser-function arguments. A key such as
//        hist.filter_function(t,x,y,z,ux,uy,uz) = "abs(x) < x_max"
//    declares the arguments t,x,...; inside its value `x` is an argument while
//    `x_max` is a constant. The scanner records the names declared in the key
//    and emits _param_ref for identifiers of the value that match one of them.
//    The list is cleared at the `=` of any key without an argument list.
//
// 2. Section headers: a comment at the start of a line made of `#`, at least
//    three decoration characters (- = # * ~ _ +) and some text, e.g.
//        # --- GEOMETRY --- #      #----PARTICLE DIAGNOSTIC----#
//        ####### PLASMA #######
//    is split into _section_open, _section_title and _section_close.
//
// 3. _eof: zero-width end of the last line when the file has no final newline.
//
// No libc functions are used, so the parser builds unchanged to WebAssembly.

#include "tree_sitter/alloc.h"
#include "tree_sitter/parser.h"

#include <stdbool.h>
#include <stdint.h>

enum TokenType {
  ASSIGN,
  PARAMS_OPEN,
  PARAM_DECL,
  PARAM_REF,
  SECTION_OPEN,
  SECTION_TITLE,
  SECTION_CLOSE,
  EOF_TOKEN,
  ERROR_SENTINEL,
};

#define MAX_PARAMS 16
#define MAX_PARAM_LENGTH 31

typedef struct {
  uint8_t count;                 // number of recorded argument names
  bool has_params;               // the key being parsed has an argument list
  uint8_t length[MAX_PARAMS];
  char name[MAX_PARAMS][MAX_PARAM_LENGTH];
} Scanner;

static inline bool is_blank(int32_t c) {
  return c == ' ' || c == '\t' || c == '\r' || c == '\f' || c == 0xFEFF || c == 0xA0;
}

static inline bool is_ident_start(int32_t c) {
  return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

static inline bool is_ident_char(int32_t c) {
  return is_ident_start(c) || (c >= '0' && c <= '9');
}

static inline bool is_decoration(int32_t c) {
  return c == '-' || c == '=' || c == '#' || c == '*' || c == '~' || c == '_' || c == '+';
}

static inline bool is_title_char(int32_t c) {
  return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c >= 0x80;
}

static inline bool at_line_end(TSLexer *lexer) {
  return lexer->eof(lexer) || lexer->lookahead == '\n' || lexer->lookahead == '\r';
}

void *tree_sitter_warpx_external_scanner_create(void) {
  return ts_calloc(1, sizeof(Scanner));
}

void tree_sitter_warpx_external_scanner_destroy(void *payload) {
  ts_free(payload);
}

unsigned tree_sitter_warpx_external_scanner_serialize(void *payload, char *buffer) {
  Scanner *scanner = (Scanner *)payload;
  unsigned size = 0;
  buffer[size++] = (char)scanner->count;
  buffer[size++] = (char)scanner->has_params;
  for (unsigned i = 0; i < scanner->count; i++) {
    buffer[size++] = (char)scanner->length[i];
    for (unsigned j = 0; j < scanner->length[i]; j++) {
      buffer[size++] = scanner->name[i][j];
    }
  }
  return size; // at most 2 + 16 * 32 = 514 bytes
}

void tree_sitter_warpx_external_scanner_deserialize(void *payload, const char *buffer, unsigned length) {
  Scanner *scanner = (Scanner *)payload;
  scanner->count = 0;
  scanner->has_params = false;
  if (length < 2) return;
  unsigned size = 0;
  unsigned count = (uint8_t)buffer[size++];
  scanner->has_params = buffer[size++] != 0;
  for (unsigned i = 0; i < count && i < MAX_PARAMS && size < length; i++) {
    unsigned name_length = (uint8_t)buffer[size++];
    if (name_length > MAX_PARAM_LENGTH || size + name_length > length) break;
    for (unsigned j = 0; j < name_length; j++) {
      scanner->name[i][j] = buffer[size++];
    }
    scanner->length[i] = (uint8_t)name_length;
    scanner->count = (uint8_t)(i + 1);
  }
}

bool tree_sitter_warpx_external_scanner_scan(void *payload, TSLexer *lexer, const bool *valid_symbols) {
  Scanner *scanner = (Scanner *)payload;

  // During error recovery every token is marked valid: leave it to the
  // internal lexer.
  if (valid_symbols[ERROR_SENTINEL]) return false;

  while (is_blank(lexer->lookahead)) lexer->advance(lexer, true);

  // Title of a section header: up to the last character that is neither
  // decoration nor blank.
  if (valid_symbols[SECTION_TITLE] && !at_line_end(lexer)) {
    bool found = false;
    while (!at_line_end(lexer)) {
      int32_t c = lexer->lookahead;
      lexer->advance(lexer, false);
      if (!is_decoration(c) && !is_blank(c)) {
        lexer->mark_end(lexer);
        found = true;
      }
    }
    if (!found) return false;
    lexer->result_symbol = SECTION_TITLE;
    return true;
  }

  // Trailing decoration of a section header.
  if (valid_symbols[SECTION_CLOSE] && !at_line_end(lexer)) {
    while (!at_line_end(lexer)) {
      int32_t c = lexer->lookahead;
      lexer->advance(lexer, false);
      if (!is_blank(c)) lexer->mark_end(lexer);
    }
    lexer->result_symbol = SECTION_CLOSE;
    return true;
  }

  if (valid_symbols[EOF_TOKEN] && lexer->eof(lexer)) {
    lexer->mark_end(lexer);
    lexer->result_symbol = EOF_TOKEN;
    return true;
  }

  int32_t c = lexer->lookahead;

  if (valid_symbols[ASSIGN] && c == '=') {
    lexer->advance(lexer, false);
    lexer->mark_end(lexer);
    if (!scanner->has_params) scanner->count = 0;
    scanner->has_params = false;
    lexer->result_symbol = ASSIGN;
    return true;
  }

  if (valid_symbols[PARAMS_OPEN] && c == '(') {
    lexer->advance(lexer, false);
    lexer->mark_end(lexer);
    scanner->count = 0;
    scanner->has_params = true;
    lexer->result_symbol = PARAMS_OPEN;
    return true;
  }

  bool declaring = valid_symbols[PARAM_DECL];
  if ((declaring || (valid_symbols[PARAM_REF] && scanner->count > 0)) && is_ident_start(c)) {
    char text[MAX_PARAM_LENGTH];
    unsigned length = 0;
    // Identifiers of a value may contain dots, as in the grammar.
    while (is_ident_char(lexer->lookahead) || (!declaring && lexer->lookahead == '.')) {
      if (length < MAX_PARAM_LENGTH) text[length] = (char)lexer->lookahead;
      length++;
      lexer->advance(lexer, false);
    }
    lexer->mark_end(lexer);

    if (declaring) {
      if (length <= MAX_PARAM_LENGTH && scanner->count < MAX_PARAMS) {
        unsigned i = scanner->count;
        for (unsigned j = 0; j < length; j++) scanner->name[i][j] = text[j];
        scanner->length[i] = (uint8_t)length;
        scanner->count++;
      }
      lexer->result_symbol = PARAM_DECL;
      return true;
    }

    if (length > MAX_PARAM_LENGTH) return false;
    for (unsigned i = 0; i < scanner->count; i++) {
      if (scanner->length[i] != length) continue;
      bool same = true;
      for (unsigned j = 0; j < length && same; j++) same = scanner->name[i][j] == text[j];
      if (same) {
        lexer->result_symbol = PARAM_REF;
        return true;
      }
    }
    return false; // an ordinary identifier, lexed by the grammar
  }

  // Section header: `#`, blanks, >= 3 decoration characters, then a title.
  if (valid_symbols[SECTION_OPEN] && c == '#') {
    lexer->advance(lexer, false);
    while (lexer->lookahead == ' ' || lexer->lookahead == '\t') lexer->advance(lexer, false);
    unsigned run = 0;
    while (is_decoration(lexer->lookahead)) {
      run++;
      lexer->advance(lexer, false);
    }
    if (run < 3) return false;
    while (is_decoration(lexer->lookahead) || lexer->lookahead == ' ' || lexer->lookahead == '\t') {
      lexer->advance(lexer, false);
    }
    lexer->mark_end(lexer);
    while (!at_line_end(lexer)) {
      if (is_title_char(lexer->lookahead)) {
        lexer->result_symbol = SECTION_OPEN;
        return true;
      }
      lexer->advance(lexer, false);
    }
    return false; // only decoration: an ordinary comment
  }

  return false;
}
