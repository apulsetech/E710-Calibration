#############################################################################
#                  IMPINJ CONFIDENTIAL AND PROPRIETARY                      #
#                                                                           #
# This source code is the property of Impinj, Inc. Your use of this source  #
# code in whole or in part is subject to your applicable license terms      #
# from Impinj.                                                              #
# Contact support@impinj.com for a copy of the applicable Impinj license    #
# terms.                                                                    #
#                                                                           #
# (c) Copyright 2015 - 2023 Impinj, Inc. All rights reserved.               #
#                                                                           #
#############################################################################


class InvalidEvalStatement(Exception):
    pass


def _indent(s, tab=False, levels=1):
    """
    Provide a pretty printing like functionality for parsed objects

    :param s: the string to 'prettify'
    :param tab: bool
    :param levels: int
    :return: prettified version of 's'

    """
    if tab:
        delim = '\t' * levels
    else:
        delim = '    ' * levels

    rstr = '\n'.join([delim + x for x in s.split('\n')])

    if s[-1] == '\n':
        rstr += '\n'

    return rstr


# The potential for abuse is high here, however given the requirements
# at hand, the alternative would involve writing our own simple DSL
# and I believe that the complexity in that task would far outweigh
# the benefit of the strict subset of capabilities (for safety)
def _eval_statement(st, defines={}):
    """
    This function evaluates a valid python expression using a predefined
    defines dictionary to allow variable name substitution.
    NOTE: this function disallows the use of import statements to reduce
    the risk of executing dangerous statements.

    Inputs:
      - st (string): Any valid python expression | VAR1 + VAR2
      - defines (dict): A dictionary providing context for any variables in
        st | {VAR1:3, VAR2:5}

    Returns: The result of the expression.
    """

    # Take a copy of the the defines so we don't accidentally modify
    # the source material
    if 'import' in st:
        raise InvalidEvalStatement(
            'Attempted import in eval statement: \n{}'.format(st)
        )

    # Note that we don't define any globals and instead use the
    # supplied definitions as locals only (to avoid conflict)
    return eval(st, {}, defines.copy())


def lookup_with_alternate(the_dict, the_key, alt_values, default_val):
    """
    Return a value either as a direct lookup into a dictionary, or
    use the return of that lookup as a lookup into another dictionary and
    return that value.
    :param the_dict: python dictionary
    :param the_key: string
    :param alt_values: python dictionary
    :param default_val:

    """
    if the_key not in the_dict:
        return default_val

    if the_dict[the_key] in alt_values:
        return alt_values[the_dict[the_key]]

    return the_dict[the_key]


def camel_to_snake(camel_str):
    """ Convert a CamelCaseString to a snake_case_string. """
    snake_str = ''
    prev_value = None
    for value in camel_str:
        if value.isupper():
            if snake_str != '' and prev_value != '_':
                snake_str += '_'
            snake_str += value.lower()
        else:
            snake_str += value
        prev_value = value

    return snake_str
