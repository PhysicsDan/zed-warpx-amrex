; Syntax highlighting for WarpX input files (AMReX ParmParse format).
; In Zed, when several patterns capture the same text the LAST one wins, so
; general rules come first and special cases after them.

; ------------------------------------------------------------------ comments

(comment) @comment

(line_continuation) @punctuation.special

(directive) @preproc

; "# --- GEOMETRY --- #": decoration dimmed, title highlighted
(section_header) @comment.doc

(section_header
  title: (title) @title)

; ---------------------------------------------------------------------- keys

; carbon.foil.density: prefixes you define (species, lasers, diagnostics, ...)
(namespace) @type

; the parameter itself
(property) @property

(key
  "." @punctuation.delimiter)

(table_header
  "." @punctuation.delimiter)

(table_header
  [
    "["
    "]"
  ] @punctuation.bracket)

; Built-in prefixes: warpx.cfl, amr.n_cell, geometry.prob_lo, ...
(key
  .
  (namespace) @keyword
  (#any-of? @keyword
    "ablastr" "algo" "amr" "amrex" "amrex_gmres" "boundary" "collisions"
    "diagnostics" "eb2" "external_vector_potential" "fab" "fabarray" "fluids"
    "geometry" "gmres" "hybrid_pic_model" "implicit_evolve" "insulator"
    "interpolation" "jacobian" "lasers" "lattice" "macroscopic" "my_constants"
    "newton" "particle_thermalizer" "particles" "pc_algmg" "pc_curl_curl_mlmg"
    "pc_darwin_mlmg" "pc_jacobi" "pc_petsc" "picard" "precon_mat" "psatd"
    "qed_bw" "qed_qs" "qed_schwinger" "reduced_diags" "slice" "tiny_profiler"
    "vismf" "warpx"))

(table_header
  .
  (namespace) @keyword
  (#any-of? @keyword
    "ablastr" "algo" "amr" "amrex" "amrex_gmres" "boundary" "collisions"
    "diagnostics" "eb2" "external_vector_potential" "fab" "fabarray" "fluids"
    "geometry" "gmres" "hybrid_pic_model" "implicit_evolve" "insulator"
    "interpolation" "jacobian" "lasers" "lattice" "macroscopic" "my_constants"
    "newton" "particle_thermalizer" "particles" "pc_algmg" "pc_curl_curl_mlmg"
    "pc_darwin_mlmg" "pc_jacobi" "pc_petsc" "picard" "precon_mat" "psatd"
    "qed_bw" "qed_qs" "qed_schwinger" "reduced_diags" "slice" "tiny_profiler"
    "vismf" "warpx"))

; my_constants.n0 = ... defines the constant n0
(key
  .
  (namespace) @_prefix
  .
  (property) @constant
  (#eq? @_prefix "my_constants"))

; Arguments of a parser function, declared in the key and used in the value:
;   filter_function(t,x,y,z,ux,uy,uz) = "abs(x) < x_max"
(parameter) @variable.parameter

(parameters
  [
    "("
    ")"
  ] @punctuation.bracket)

(parameters
  "," @punctuation.delimiter)

; FILE = other.inputs, UNSET = key1 key2
(keyword) @keyword

(include
  path: (path) @string.special)

(unset
  (key_name) @property)

"=" @operator

; -------------------------------------------------------------------- values

; user constants (n0, dz_cell, ...) and option names (esirkepov, Full, ...)
(identifier) @constant

(number) @number

[
  (string)
  (raw_string)
] @string

[
  "+"
  "-"
  "*"
  "/"
  "**"
  "^"
  "//"
  "%"
  "<"
  ">"
  "<="
  ">="
  "=="
  "!="
  "!"
  "&&"
  "||"
  "&"
  "|"
  "?"
] @operator

[
  ","
  ":"
  ";"
] @punctuation.delimiter

[
  "("
  ")"
  "["
  "]"
  "{"
  "}"
] @punctuation.bracket

(call
  function: (identifier) @function)

; amrex::Parser functions
(call
  function: (identifier) @function.builtin
  (#any-of? @function.builtin
    "sqrt" "exp" "log" "log10" "sin" "cos" "tan" "asin" "acos" "atan"
    "atan2" "sinh" "cosh" "tanh" "asinh" "acosh" "atanh" "abs" "fabs" "iabs"
    "floor" "ceil" "comp_ellint_1" "comp_ellint_2" "erf" "pow" "heaviside"
    "jn" "yn" "min" "max" "fmod" "if"))

; WarpX constants
((identifier) @constant.builtin
  (#any-of? @constant.builtin
    "pi" "clight" "epsilon0" "mu0" "q_e" "m_e" "m_p" "m_u" "kb" "hbar"))

((identifier) @boolean
  (#any-of? @boolean "true" "false" "True" "False" "TRUE" "FALSE"))

((identifier) @keyword.operator
  (#any-of? @keyword.operator "and" "or"))

; Values that name things defined elsewhere in the file (species, lasers,
; diagnostics, ...) get the same colour as those names in keys
(assignment
  key: (key
    (property) @property)
  value: (identifier) @type
  (#any-of? @property
    "species_names" "names" "diags_names" "reduced_diags_names"
    "collision_names" "injection_sources" "species" "particle_fields_species"
    "ionization_product_species" "ionization_species"
    "ionization_target_species" "product_species" "ele_product_species"
    "pos_product_species" "qed_breit_wheeler_ele_product_species"
    "qed_breit_wheeler_pos_product_species"
    "qed_quantum_sync_phot_product_species" "qed_virtual_photon_species_name"
    "rigid_injected_species" "photon_species" "deposit_on_main_grid"
    "gather_from_main_grid" "elements"))
