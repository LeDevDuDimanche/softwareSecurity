#!/usr/bin/env python3

"""Script to run integration tests.

Each test is a callable (e.g., a function) that can be called with no
arguments.  When run, it performs the test and returns a tuple with three
elements (name, passed, info) of type (str, bool, str).  The first value is the
name of the test.  The second is True if the test passed.  The third gives
diagnostic information if the test failed.  If the test passed, the third value
is meaningless.

The tests are allowed to assume that setup() was run before the test is
run.  So the temporary test directory will be properly set up.

The tests shouldn't break the setup or assume that a different test was run
before them.  This means it shouldn't matter in what order the tests are run.

This script requires Python 3.6 or higher."""


import pathlib
import re
import shutil


# `HERE` is the directory this script is located in.
HERE = pathlib.Path(__file__).resolve().parent
TEMP_DIR = HERE / 'temp'
CASES_DIR = HERE / 'cases'
BIN_DIR = HERE.parent / 'bin'
TEST_CONFIG = HERE / 'grass_test.conf'

IN_PATTERN = '*.in'
OUT_SUFFIX = '.outregx'

CLIENT = 'client'
SERVER = 'server'
BINARIES = CLIENT, SERVER
SERVER_PATH = TEMP_DIR / SERVER
CONFIG_PATH = TEMP_DIR / 'grass.conf'


def get_tests():
    """Yield all tests."""

    yield test_hijack_exists

    for in_path in sorted(CASES_DIR.glob(IN_PATTERN)):
        out_path = in_path.with_suffix(OUT_SUFFIX)
        name = "output-" + in_path.stem
        yield get_regex_test(name, in_path, out_path)


def setup():
    """Perform setup for tests and return True if it succeeded."""

    try:
        TEMP_DIR.mkdir(exist_ok=True)
    except OSError:
        print("Error in setup: couldn't make temporary test directory")
        return False

    # Copy the binaries into the temporary test directory because the server
    # must be in the same directory as the config file.
    for binary in BINARIES:
        try:
            shutil.copy(BIN_DIR / binary, TEMP_DIR / binary)
        except OSError:
            print(f"Error in setup: couldn't copy {binary}.  (Run `make` "
                   "before testing!)")
            return False

    try:
        shutil.copy(TEST_CONFIG, CONFIG_PATH)
    except OSError:
        print("Error in setup: couldn't copy test config")
        return False

    return True


def cleanup():
    """Clean up after tests."""

    try:
        CONFIG_PATH.unlink()
    except OSError:
        print("Error in cleanup: couldn't remove config file")

    for binary in BINARIES:
        try:
            (TEMP_DIR / binary).unlink()
        except OSError:
            print(f"Error in cleanup: couldn't remove {binary}")

    try:
        TEMP_DIR.rmdir()
    except OSError:
        print("Error in cleanup: couldn't delete the temporary test "
              "directory.  (Maybe it isn't empty, tests should clean up the "
              "files they generate!)")


def escape_decode(text_bytes):
    """Decode bytes with ASCII encoding, escaping non-printable characters."""
    text = text_bytes.decode('latin-1')
    escaped = (ascii(char)[1:-1] if char != '\n' else char for char in text)
    return ''.join(escaped)


def run_test(test):
    """Run a test and print results."""
    name, passed, info = test()
    if passed:
        print(name, "passed")
    else:
        print(name, "FAILED")
        print(info)
        print()


def test_hijack_exists():
    passed = b"Method hijack: Accepted\n" in SERVER_PATH.read_bytes()
    info = "The hijack function wasn't compiled into the server binary."
    return "hijack-exists", passed, info


def get_regex_test(name, in_path, out_path):
    """Return a test function.

    The returned test function compared uses the contents of the file at
    `in_path` as input for the client and checks the output of the client.  The
    output should match the regular expression in the file at `out_path`."""

    def test():
        try:
            in_bytes = in_path.read_bytes()
        except OSError:
            return name, False, "Couldn't read the input file."

        try:
            out_pattern = out_path.read_bytes()
        except OSError:
            return name, False, "Couldn't read the output pattern file."

        out_bytes = b'TODO\n'

        if not re.fullmatch(out_pattern, out_bytes, re.VERBOSE):
            info = ("Unexpected client output.\n"
                    "#### EXPECTED (regex) ####\n" +
                    out_pattern.decode('ascii') +
                    "#### ACTUAL ####\n" +
                    escape_decode(out_bytes) +
                    "#### END ####")
            return name, False, info

        return name, True, None

    return test


def main():
    """Run the test suite."""
    tests = list(get_tests())
    try:
        if setup():
            for test in tests:
                run_test(test)
    finally:
        cleanup()


if __name__ == '__main__':
    main()
