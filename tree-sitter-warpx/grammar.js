/**
 * @file Tree-sitter grammar for WarpX input files (AMReX ParmParse format)
 * @license MIT
 *
 * One definition per line:   prefix.name = value value ...
 *   - `#` starts a comment (outside double quotes)
 *   - `\` at the end of a line continues the line
 *   - double-quoted strings (math expressions) may span several lines
 *   - keys may declare parser-function arguments: field_function(X,Y,t) = "..."
 *   - AMReX extras: FILE = include, UNSET = keys, [table] headers, TOML-like
 *     arrays, {...} tables, """...""" strings, #if AMREX_SPACEDIM ... lines
 *
 * The external scanner (src/scanner.c) remembers the arguments declared in a
 * key so that their uses in the value can be told apart from constants, and
 * recognises "# --- Section --- #" style comments as section headers.
 */

/// <reference types="tree-sitter-cli/dsl" />
// @ts-check

const OPERATORS = [
  '+', '-', '*', '/', '**', '^', '//', '%',
  '<', '>', '<=', '>=', '==', '!=',
  '!', '&&', '||', '&', '|', '?',
];

// Things that can appear in a value, quoted or not (nested groups excluded).
function atoms($) {
  return [
    $.number,
    $.identifier,
    alias($._param_ref, $.parameter),
    ...OPERATORS,
    ',', ':', ';',
    $._other,
  ];
}

module.exports = grammar({
  name: 'warpx',

  externals: $ => [
    $._assign, // the `=` of a definition
    $._params_open, // `(` opening the argument list of a key
    $._param_decl, // argument name in a key
    $._param_ref, // use of a declared argument in the value
    $._section_open, // `# ---` of a section header comment
    $._section_title, // its title
    $._section_close, // trailing `--- #`
    $._eof, // end of the last line when the file has no final newline
    $._error_sentinel,
  ],

  extras: $ => [
    /[ \t\f\r﻿ ]/,
    $.line_continuation,
    $.comment,
  ],

  rules: {
    source_file: $ => seq(
      repeat($._line),
      repeat($.section),
    ),

    section: $ => seq(
      field('header', $.section_header),
      $._end,
      repeat($._line),
    ),

    section_header: $ => seq(
      $._section_open,
      field('title', alias($._section_title, $.title)),
      optional($._section_close),
    ),

    _line: $ => choice(
      $._newline,
      $.directive,
      seq($._statement, $._end),
    ),

    _end: $ => choice($._newline, $._eof),

    _newline: _ => /\n/,

    _statement: $ => choice(
      $.assignment,
      $.include,
      $.unset,
      $.table_header,
      $.brace_list, // a {...} table that starts on the line after its key
    ),

    // ------------------------------------------------------------------ keys

    assignment: $ => seq(
      field('key', $.key),
      alias($._assign, '='),
      repeat(field('value', $._value)),
    ),

    key: $ => seq(
      repeat(seq(alias($._key_segment, $.namespace), '.')),
      alias($._key_segment, $.property),
      optional(field('parameters', $.parameters)),
    ),

    _key_segment: _ => /[A-Za-z_][A-Za-z0-9_\-]*/,

    parameters: $ => seq(
      alias($._params_open, '('),
      optional(seq(
        alias($._param_decl, $.parameter),
        repeat(seq(',', alias($._param_decl, $.parameter))),
      )),
      ')',
    ),

    include: $ => seq(
      alias('FILE', $.keyword),
      alias($._assign, '='),
      field('path', choice($.string, alias(/[^\s"#]+/, $.path))),
    ),

    unset: $ => seq(
      alias('UNSET', $.keyword),
      alias($._assign, '='),
      repeat(alias(/[A-Za-z_][A-Za-z0-9_.\-]*/, $.key_name)),
    ),

    table_header: $ => seq(
      '[',
      repeat(seq(alias($._key_segment, $.namespace), '.')),
      alias($._key_segment, $.namespace),
      ']',
    ),

    // #if AMREX_SPACEDIM == 2 / #elif / #else / #endif / #ifdef AMREX_USE_GPU
    // Only whole lines count (anything else after them makes them comments).
    directive: _ => token(prec(3, seq(
      '#', /[ \t]*/,
      choice(
        seq(
          choice('if', 'elif'),
          /[ \t]+\(?[ \t]*AMREX_SPACEDIM[ \t]*(>=|<=|==|!=|>|<)[ \t]*[1-3][ \t]*\)?/,
        ),
        'else',
        'endif',
        seq(choice('ifdef', 'ifndef'), /[ \t]+AMREX_USE_GPU/),
      ),
      /[ \t\r]*\n/,
    ))),

    // ---------------------------------------------------------------- values

    _value: $ => choice(
      ...atoms($),
      $.string,
      $.raw_string,
      $.call,
      $.parenthesized_expression,
      $.array,
      $.brace_list,
    ),

    // Inside (), [] and {} of an unquoted value
    _nested: $ => choice($._value, $._newline),

    call: $ => prec(1, seq(
      field('function', $.identifier),
      '(',
      repeat($._value),
      ')',
    )),

    parenthesized_expression: $ => seq('(', repeat($._value), ')'),

    array: $ => seq('[', repeat($._nested), ']'),

    brace_list: $ => seq('{', repeat($._nested), '}'),

    // "..." : usually a math expression, may span several lines
    string: $ => seq('"', repeat($._string_part), '"'),

    _string_part: $ => choice(
      ...atoms($),
      '=',
      $._string_hash,
      $._newline,
      alias($._string_call, $.call),
      alias($._string_parenthesized, $.parenthesized_expression),
      alias($._string_array, $.array),
      alias($._string_brace_list, $.brace_list),
    ),

    // Same groups as above, but they cannot open a nested string, so an
    // unbalanced `(` inside quotes stays inside the quotes.
    _string_call: $ => prec(1, seq(
      field('function', $.identifier),
      '(',
      repeat($._string_part),
      ')',
    )),
    _string_parenthesized: $ => seq('(', repeat($._string_part), ')'),
    _string_array: $ => seq('[', repeat($._string_part), ']'),
    _string_brace_list: $ => seq('{', repeat($._string_part), '}'),

    _string_hash: _ => token(prec(2, '#')),

    // """...""" : raw multi-line string
    raw_string: _ => token(seq(
      '"""',
      repeat(choice(/[^"]/, /"[^"]/, /""[^"]/)),
      '"""',
    )),

    number: _ => token(/[-+]?(\d+\.?\d*|\.\d+)([eE][-+]?\d+)?/),

    // Parser symbols may contain dots (amrex.c, a file name, ...)
    identifier: _ => /[A-Za-z_][A-Za-z0-9_.]*/,

    // Any other single character, so odd values (paths, units) still parse
    _other: _ => token(prec(-1, /[^\s"#()\[\]{}=,:;]/)),

    line_continuation: _ => token(seq('\\', /\r?\n/)),

    comment: _ => token(seq('#', /.*/)),
  },
});
