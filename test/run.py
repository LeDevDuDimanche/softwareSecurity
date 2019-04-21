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
import shutil


# `HERE` is the directory this script is located in.
HERE = pathlib.Path(__file__).resolve().parent
TEST_DIR = HERE / 'temp'
BIN_DIR = HERE.parent / 'bin'

CLIENT = 'client'
SERVER = 'server'
BINARIES = CLIENT, SERVER
SERVER_PATH = TEST_DIR / SERVER


def get_tests():
    """Yield all tests."""
    yield test_hijack_exists


def setup():
    """Perform setup for tests and return True if it succeeded."""

    try:
        TEST_DIR.mkdir(exist_ok=True)
    except OSError:
        print("Error in setup: couldn't make temporary test directory")
        return False

    # Copy the binaries into the temporary test directory because the server
    # must be in the same directory as the config file.
    for binary in BINARIES:
        try:
            shutil.copy(BIN_DIR / binary, TEST_DIR / binary)
        except OSError:
            print(f"Error in setup: couldn't copy {binary}.  (Run `make` "
                   "before testing!)")
            return False

    return True


def cleanup():
    """Clean up after tests."""

    for binary in BINARIES:
        try:
            (TEST_DIR / binary).unlink()
        except OSError:
            print(f"Error in cleanup: couldn't remove {binary}")

    try:
        TEST_DIR.rmdir()
    except OSError:
        print("Error in cleanup: couldn't delete the temporary test "
              "directory.  (Maybe it isn't empty, tests should clean up their "
              "files themselves!)")


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
