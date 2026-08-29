module.exports = grammar({
  name: 'ibex',

  extras: $ => [
    $.comment,
    /\s/,
  ],

  word: $ => $.identifier,

  rules: {
    source_file: $ => repeat($._declaration),

    comment: $ => token(seq('//', /.*/)),

    identifier: $ => /[a-zA-Z_][a-zA-Z0-9_]*/,

    _declaration: $ => choice(
      $.package_declaration,
      $.module_declaration,
      $.import_declaration,
      $.struct_declaration,
      $.enum_declaration,
      $.flag_declaration,
      $.function_declaration,
      $.variable_declaration,
      $.constant_declaration,
      $.type_alias_declaration
    ),

    package_declaration: $ => seq(
      'package',
      $.identifier,
      ';'
    ),

    module_declaration: $ => seq(
      'module',
      $.identifier,
      optional($.module_parameters),
      ';'
    ),

    module_parameters: $ => seq(
      '(',
      commaSep($.parameter_declaration),
      ')'
    ),

    import_declaration: $ => seq(
      'import',
      $._type, // can be a complex path like a::b
      optional(seq('(', commaSep($._expression), ')')),
      optional(seq('as', $.identifier)),
      ';'
    ),

    struct_declaration: $ => seq(
      optional($.attribute_list),
      'struct',
      $.identifier,
      optional(seq(':', commaSep1($.identifier))),
      '{',
      repeat($.struct_member),
      '}'
    ),

    struct_member: $ => seq(
      $.identifier,
      ':',
      $._type,
      ';'
    ),

    enum_declaration: $ => seq(
      optional($.attribute_list),
      'enum',
      $.identifier,
      optional(seq(':', $.identifier)),
      '{',
      commaSep($.enum_member),
      '}'
    ),

    enum_member: $ => seq(
      $.identifier,
      optional(seq('=', $._expression))
    ),

    flag_declaration: $ => seq(
      optional($.attribute_list),
      'flag',
      $.identifier,
      optional(seq(':', $.identifier)),
      '{',
      commaSep($.enum_member),
      '}'
    ),

    type_alias_declaration: $ => seq(
      optional($.attribute_list),
      'type',
      $.identifier,
      '=',
      $._type,
      ';'
    ),

    function_declaration: $ => seq(
      optional($.attribute_list),
      $.identifier,
      ':',
      '(',
      commaSep($.parameter_declaration),
      ')',
      '->',
      $._type,
      $.block
    ),

    parameter_declaration: $ => choice(
      seq($.identifier, ':', $._type, optional(seq('=', $._expression))),
      seq($.identifier, ':', '...')
    ),

    variable_declaration: $ => seq(
      optional($.attribute_list),
      optional(choice('var', 'const', 'static')), // var/const/static can be inferred in ibex or explicitly written
      $.identifier,
      choice(
        seq(':', $._type, optional(seq('=', $._expression))),
        seq(':=', $._expression)
      ),
      ';'
    ),

    constant_declaration: $ => seq(
      optional($.attribute_list),
      'const',
      $.identifier,
      choice(
        seq(':', $._type, '=', $._expression),
        seq('=', $._expression),
        seq(':=', $._expression)
      ),
      ';'
    ),

    attribute_list: $ => seq(
      '[[',
      commaSep1($.identifier),
      ']]'
    ),

    _type: $ => choice(
      $.primitive_type,
      $.array_type,
      $.slice_type,
      $.pointer_type,
      $.reference_type,
      $.optional_type,
      $.tuple_type,
      $.named_type,
      $.typeof_type
    ),

    primitive_type: $ => choice(
      'i8', 'i16', 'i32', 'i64',
      'u8', 'u16', 'u32', 'u64', 'byte',
      'f32', 'f64',
      'bool',
      'text',
      'void'
    ),

    array_type: $ => seq(
      '[',
      $._expression,
      ']',
      $._type
    ),

    slice_type: $ => seq(
      '[',
      ']',
      $._type
    ),

    pointer_type: $ => seq(
      '*',
      $._type
    ),

    reference_type: $ => seq(
      choice('&', '&&'),
      $._type
    ),

    optional_type: $ => seq(
      '?',
      $._type
    ),

    tuple_type: $ => seq(
      '(',
      commaSep($._type),
      ')'
    ),

    named_type: $ => prec.left(1, seq(
      optional(seq($.identifier, '::')),
      $.identifier
    )),

    typeof_type: $ => seq(
      'typeof',
      '(',
      $._expression,
      ')'
    ),

    block: $ => seq(
      '{',
      repeat($._statement),
      '}'
    ),

    _statement: $ => choice(
      $.variable_declaration,
      $.constant_declaration,
      $.expression_statement,
      $.return_statement,
      $.break_statement,
      $.continue_statement,
      $.if_statement,
      $.while_statement,
      $.for_statement,
      $.switch_statement,
      $.block
    ),

    expression_statement: $ => seq(
      $._expression,
      ';'
    ),

    return_statement: $ => seq(
      'return',
      optional($._expression),
      ';'
    ),

    break_statement: $ => seq(
      'break',
      ';'
    ),

    continue_statement: $ => seq(
      'continue',
      ';'
    ),

    if_statement: $ => seq(
      'if',
      optional('('),
      $._expression,
      optional(')'),
      $.block,
      optional(seq('else', choice($.if_statement, $.block)))
    ),

    while_statement: $ => seq(
      'while',
      optional('('),
      $._expression,
      optional(')'),
      $.block
    ),

    for_statement: $ => seq(
      'for',
      optional('('),
      choice(
        seq($.identifier, 'in', $._expression),
        seq(
          optional(choice($.variable_declaration, $.expression_statement)),
          $._expression, ';',
          optional($._expression)
        )
      ),
      optional(')'),
      $.block
    ),

    switch_statement: $ => seq(
      'switch',
      optional('('),
      $._expression,
      optional(')'),
      '{',
      repeat($.case_clause),
      optional($.default_clause),
      '}'
    ),

    case_clause: $ => seq(
      'case',
      $._expression,
      ':',
      $.block
    ),

    default_clause: $ => seq(
      'default',
      ':',
      $.block
    ),

    _expression: $ => choice(
      $.binary_expression,
      $.unary_expression,
      $.call_expression,
      $.member_expression,
      $.index_expression,
      $.slice_expression,
      $.struct_init_expression,
      $.is_expression,
      $._primary_expression
    ),

    binary_expression: $ => choice(
      ...[
        ['=', 1, 'right'],
        ['+=', 1, 'right'],
        ['-=', 1, 'right'],
        ['*=', 1, 'right'],
        ['/=', 1, 'right'],
        ['or', 2, 'left'],
        ['and', 3, 'left'],
        ['||', 2, 'left'],
        ['&&', 3, 'left'],
        ['==', 4, 'left'],
        ['!=', 4, 'left'],
        ['<', 5, 'left'],
        ['<=', 5, 'left'],
        ['>', 5, 'left'],
        ['>=', 5, 'left'],
        ['|', 6, 'left'],
        ['^', 7, 'left'],
        ['&', 8, 'left'],
        ['<<', 9, 'left'],
        ['>>', 9, 'left'],
        ['+', 10, 'left'],
        ['-', 10, 'left'],
        ['*', 11, 'left'],
        ['/', 11, 'left'],
        ['%', 11, 'left']
      ].map(([operator, precedence, associativity]) =>
        prec[associativity](precedence, seq(
          field('left', $._expression),
          field('operator', operator),
          field('right', $._expression)
        ))
      )
    ),

    unary_expression: $ => prec.left(12, choice(
      seq('-', $._expression),
      seq('!', $._expression),
      seq('not', $._expression),
      seq('~', $._expression),
      seq('*', $._expression),
      seq('@', $._expression),
      seq('ref', $._expression),
      seq('move', '(', $._expression, ')')
    )),

    is_expression: $ => prec.left(12, seq(
      $._expression,
      'is',
      $._type
    )),

    call_expression: $ => prec.left(13, seq(
      field('function', $._expression),
      '(',
      commaSep(choice(
        $._expression,
        seq($.identifier, '=', $._expression)
      )),
      ')'
    )),

    member_expression: $ => prec.left(14, seq(
      field('object', $._expression),
      choice('.', '::', '->'),
      field('property', $.identifier)
    )),

    index_expression: $ => prec.left(14, seq(
      field('object', $._expression),
      '[',
      field('index', $._expression),
      ']'
    )),

    slice_expression: $ => prec.left(14, seq(
      field('object', $._expression),
      '[',
      optional($._expression),
      ':',
      optional($._expression),
      ']'
    )),

    struct_init_expression: $ => prec.left(15, seq(
      $.named_type,
      '{',
      commaSep(choice(
        $._expression,
        seq($.identifier, choice(':', '='), $._expression)
      )),
      '}'
    )),

    _primary_expression: $ => choice(
      $.identifier,
      $.number_literal,
      $.string_literal,
      $.raw_string_literal,
      $.boolean_literal,
      $.null_literal,
      seq('(', $._expression, ')'),
      $.array_literal,
      $.tuple_literal
    ),

    number_literal: $ => /\d+(\.\d+)?([eE][+-]?\d+)?([a-zA-Z0-9_]+)?/,
    
    string_literal: $ => /"([^"\\]|\\.)*"/,
    
    raw_string_literal: $ => /`[^`]*`/,
    
    boolean_literal: $ => choice('true', 'false'),
    
    null_literal: $ => 'null',

    array_literal: $ => seq(
      '[',
      commaSep($._expression),
      ']'
    ),

    tuple_literal: $ => seq(
      '(',
      commaSep($._expression),
      ')'
    )
  }
});

function commaSep1(rule) {
  return seq(rule, repeat(seq(',', rule)));
}

function commaSep(rule) {
  return optional(commaSep1(rule));
}

