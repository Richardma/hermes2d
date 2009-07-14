from __future__ import division

from pyparsing import (Word, Combine, Optional, alphas, alphanums, oneOf,
                delimitedList, Group, nums, Literal, OneOrMore,
                CaselessLiteral, Forward, ZeroOrMore, restOfLine,
                ParseException, Regex)

class ParseError(Exception):
    pass

# identifier
ident = Word( alphas, alphanums + '_' )

def expr_grammar():
    """
    Returns the grammar for an expression.
    """
    fnumber = Regex(r"[-+]?\d+(\.\d*)?([eE][-+]?\d+)?")

    plus  = Literal( "+" )
    minus = Literal( "-" )
    mult  = Literal( "*" )
    div   = Literal( "/" )
    lpar  = Literal( "(" )
    rpar  = Literal( ")" )
    addop  = plus | minus
    multop = mult | div
    expop = Literal( "^" ).setParseAction(lambda : "**")
    pi    = CaselessLiteral( "pi" )

    expr = Forward()
    atom = (Optional("-") + ( pi | fnumber | ident + lpar + expr + rpar | ident ) | ( lpar + expr + rpar ))

    # by defining exponentiation as "atom [ ^ factor ]..." instead of "atom [ ^ atom ]...", we get right-to-left exponents, instead of left-to-righ
    # that is, 2^3^2 = 2^(3^2), not (2^3)^2.
    factor = Forward()
    factor << atom + ZeroOrMore( ( expop + factor ) )

    term = factor + ZeroOrMore( ( multop + factor ) )
    expr << term + ZeroOrMore( ( addop + term ) )
    return Combine(expr)

# now we define the grammar for the whole mesh file:
lbrace = Literal("{").suppress()
rbrace = Literal("}").suppress()
semicolon = Literal(";").suppress()
equal = Literal("=").suppress()
comment = Literal("#") + Optional(restOfLine)

expr = expr_grammar()
list_ = Forward()
item = expr | Group(list_)
list_ << (lbrace + delimitedList(item) + rbrace)
assig = ident + equal + item + Optional(semicolon)
mesh = OneOrMore(Group(assig))
mesh.ignore(comment)


def evaluate(s, namespace):
    """
    Evaluates the string "s" in the namespace of "namespace".

    The math module is automatically included in globals, otherwise it has to
    be a valid Python syntax.
    """
    import math
    glob = {}
    glob.update(math.__dict__)
    try:
        r = eval(s, glob, namespace)
    except:
        raise ParseError("Failed to evaluate: %s" % s)
    return r

def evaluate_list(s):
    """
    Evaluates each item in the list recursively.

    Converts the list to a tuple.
    """
    if hasattr(s, "__iter__"):
        return tuple([evaluate_list(y) for y in s])
    else:
        return s

def read_hermes_format(filename):
    """
    Reads a mesh from a file in a hermes format.

    Returns nodes, elements, boundary, nurbs or raises a ParseError if the
    syntax is invalid.
    """
    m = open(filename).read()
    return read_hermes_format_str(m)

def read_hermes_format_str(m):
    """
    Reads a mesh from a string in a hermes format.

    Returns nodes, elements, boundary, nurbs or raises a ParseError if the
    syntax is invalid.
    """
    m = m.strip()
    m = m.replace("=\n", "= \\\n")
    m = m.replace("{", "[")
    m = m.replace("}", "]")
    m = m.replace("^", "**")
    import math
    namespace = {}
    try:
        exec m in math.__dict__, namespace
    except SyntaxError, e:
        raise ParseError(str(e))
    nodes = namespace.pop("vertices", None)
    elements = namespace.pop("elements", None)
    boundary = namespace.pop("boundaries", None)
    nurbs = namespace.pop("curves", None)
    if nodes is None or elements is None or boundary is None:
        raise ParseError("Either nodes, elements or boundary is missing")
    return evaluate_list(nodes), evaluate_list(elements), evaluate_list(boundary), evaluate_list(nurbs)
