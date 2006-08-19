# Copyright (C) 2006  Tobi Vollebregt
# This sed script can be used to convert double precision floating point
# constants in C/C++ code to single precision floating point constants.
# Run it with: sed -r --file=double_to_single_precision.sed -i foo.cpp bar.cpp
# Known quirks:
#   * 1.0l is incorrectly replaced by 1.0fl.
#   * Any number with a dot behind it is replaced, so also numbered lists in
#     comments, copyright statements and numbers in strings (special measures
#     have been taken against conversion of number on the beginning or end of
#     a string though, because this was a common case in Spring) risk
#     conversion to single precision.
#   * Number on the beginning or end of a line aren't converted. This is no
#     problem for C/C++ though as there normally are no numbers there.

# The command is put here twice because:
#   * sed doesn't match the same regex multiple times, and
#   * this regex looks at the char before and after the number too.
# Hence the 30.0 in (16.0/30.0) wouldn't be converted properly because the '/'
# would have been included in the first match already and it can't be matched
# a second time.

s/([^A-Z_0-9"])(((([0-9]*\.[0-9]+)|([0-9]+\.[0-9]*))([E][-+]?[0-9]+)?)|([0-9]+[E][-+]?[0-9]+))([^EF0-9"])/\1\2f\9/gi
s/([^A-Z_0-9"])(((([0-9]*\.[0-9]+)|([0-9]+\.[0-9]*))([E][-+]?[0-9]+)?)|([0-9]+[E][-+]?[0-9]+))([^EF0-9"])/\1\2f\9/gi
