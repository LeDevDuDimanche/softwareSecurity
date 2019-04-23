Names of .in files and .outregx files are match to each other.  For example,
date.in as input to a client will result in client output matching the
contents of date.outregx (Python verbose regular expression).

If and only if the name contains 'exit', it is expected (and checked) that the
client exits.

The test cases in this folder shouldn't contain any input that results in an
error message being printed.
