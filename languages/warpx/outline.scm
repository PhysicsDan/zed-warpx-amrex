; Outline panel / symbol search: one entry per "# --- Section --- #" comment,
; with the parameters defined below it nested inside.

(section
  header: (section_header
    title: (title) @name)) @item

(assignment
  key: (key) @name) @item

(table_header) @name @item

(include
  (keyword) @context
  path: (_) @name) @item
