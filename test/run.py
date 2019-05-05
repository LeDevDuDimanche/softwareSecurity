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
import subprocess
import sys
import time


# `HERE` is the directory this script is located in.
HERE = pathlib.Path(__file__).resolve().parent
TEMP_DIR = HERE / 'temp'
ERROR_FREE_DIR = HERE / 'error_free_cases'
ERROR_DIR = HERE / 'error_cases'
BIN_DIR = HERE.parent / 'bin'
TEST_CONFIG = HERE / 'grass.conf'

IN_PATTERN = '*.in'
OUT_SUFFIX = '.outregx'
ALWAYS_RUN_FILE_IO = {'cd-cwd', 'ping', 'kthxbye'}
ERROR_REGEX = re.compile(rb'^Error', re.MULTILINE)

CLIENT = 'client'
SERVER = 'server'
BINARIES = CLIENT, SERVER
CLIENT_PATH = TEMP_DIR / CLIENT
SERVER_PATH = TEMP_DIR / SERVER
CONFIG_PATH = TEMP_DIR / 'grass.conf'
REPO_DIR = TEMP_DIR / 'repo'
IP_ADDRESS = '127.0.0.1'
PORT = 2405

OUT_FILENAME = 'stdout.txt'
OUT_FILE_PATH = TEMP_DIR / OUT_FILENAME
STARTUP_WAIT = 0.045
TIMEOUT = 3.0

STDOUT_SUFFIX = '-stdout'
FILE_IO_SUFFIX = '-file-io'

RED = "\033[0;31m"
GREEN = "\033[0;32m"
END_COLOR = "\033[0m"

REPLACEMENTS = {b"samtala": b"total"}


def get_tests(include_file_io):
    """Yield all tests."""

    yield test_hijack_exists

    error_free_cases = list(ERROR_FREE_DIR.glob(IN_PATTERN))
    if not error_free_cases:
        print("Error: couldn't find any 'error-free' test cases")
    yield from get_regex_tests(error_free_cases, include_file_io)

    error_cases = list(ERROR_DIR.glob(IN_PATTERN))
    if not error_cases:
        print("Error: couldn't find any 'error' test cases")
    yield from get_regex_tests(error_cases, include_file_io,
                               may_trigger_errors=True)


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
    """Clean up after testing."""

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


def run_system(in_bytes, should_exit):
    """Run a client and server and return their respective output.

    Raise a `subprocess.SubprocessError` on error."""

    client_args = IP_ADDRESS, str(PORT)
    return run_system_processes(client_args, should_exit, in_bytes)


def run_system_file_io(in_path, should_exit):
    """Run a client and server and return the server's output.

    Raise a `subprocess.SubprocessError` on error."""

    client_args = IP_ADDRESS, str(PORT), str(in_path), OUT_FILENAME
    _, server_output = run_system_processes(client_args, should_exit)
    return server_output


def run_system_processes(client_args, should_exit, in_bytes=None):
    """Run a client and server and return their respective output bytes.

    Raise a `subprocess.SubprocessError` on error."""

    kwargs = dict(stdout=subprocess.PIPE, stderr=subprocess.DEVNULL,
                  cwd=TEMP_DIR)

    try:
        server_process = subprocess.Popen(str(SERVER_PATH), **kwargs)
    except OSError as e:
        message = f"Couldn't run the subprocesses: {e}"
        raise subprocess.SubprocessError(message) from None

    with server_process:
        try:
            time.sleep(STARTUP_WAIT)
            args = [str(CLIENT_PATH), *client_args]

            try:
                client = subprocess.run(args, **kwargs, input=in_bytes,
                                        timeout=TIMEOUT, check=True)
            except subprocess.TimeoutExpired as e:
                if should_exit:
                    message = "Client failed to exit and timed out."
                    raise subprocess.SubprocessError(message) from None
                client_output = e.stdout
            else:
                if not should_exit:
                    message = "Client exited unexpectedly."
                    raise subprocess.SubprocessError(message)
                client_output = client.stdout
        finally:
            server_process.kill()
            try:
                shutil.rmtree(REPO_DIR)
            except OSError as e:
                print("Warning regarding the cleanup of the next test: "
                      f"Couldn't remove repo: {e}")

        server_output, _ = server_process.communicate()

    return client_output, server_output


def present_output(text_bytes, title):
    """Return the bytes in a readable representation."""
    text = escape_decode(text_bytes)
    return prominent(title) + '\n' + text + prominent(f"END {title}")


def prominent(text):
    """Return a prominently displayed version of the text."""
    return f"----- {text} -----"


def escape_decode(text_bytes):
    """Decode bytes with ASCII encoding, escaping non-printable characters."""
    text = text_bytes.decode('latin-1')
    escaped = (char if char == '\n' else ascii(char)[1:-1] for char in text)
    return ''.join(escaped)


def run_test(test):
    """Run a test, print results and return True if it passed."""
    name, passed, info = test()
    if passed:
        print(f"{GREEN}{name} passed{END_COLOR}")
    else:
        print(f"{RED}{name} FAILED{END_COLOR}")
        print(info)
        print()
    return passed


def test_hijack_exists():
    name = "hijack-exists"

    try:
        server_binary = SERVER_PATH.read_bytes()
    except OSError:
        return name, False, "Couldn't read the server binary."

    passed = b"Method hijack: Accepted" in server_binary
    info = "The hijack function wasn't compiled into the server binary."
    return name, passed, info


def get_regex_tests(in_paths, include_file_io, may_trigger_errors=False):
    """Yield test functions."""

    stems = {path: path.stem for path in in_paths}
    for in_path in sorted(in_paths, key=stems.get):
        out_path = in_path.with_suffix(OUT_SUFFIX)
        stdout_name = in_path.stem + STDOUT_SUFFIX
        file_io_name = in_path.stem + FILE_IO_SUFFIX
        yield get_regex_test(stdout_name, in_path, out_path,
                             may_trigger_errors=may_trigger_errors)
        if include_file_io or in_path.stem in ALWAYS_RUN_FILE_IO:
            yield get_regex_test(file_io_name, in_path, out_path, file_io=True,
                                 may_trigger_errors=may_trigger_errors)


def get_regex_test(name, in_path, out_path, may_trigger_errors, file_io=False):
    """Return a test function.

    The returned test function compared uses the contents of the file at
    `in_path` as input for the client and checks the output of the client.  The
    output should match the regular expression in the file at `out_path`."""

    def test():
        # `should_exit` is True if the client should exit.  Here, it's always
        # True because we should exit on EOF.
        should_exit = True

        try:
            out_pattern = out_path.read_bytes()
        except OSError:
            return name, False, "Couldn't read the output pattern file."

        if file_io:
            try:
                try:
                    server_bytes = run_system_file_io(in_path, should_exit)
                except subprocess.SubprocessError as e:
                    return name, False, str(e)

                try:
                    if not OUT_FILE_PATH.exists():
                        return name, False, "Client output file doesn't exist."
                    client_bytes = OUT_FILE_PATH.read_bytes()
                except OSError:
                    return name, False, "Couldn't read the client output file."
            finally:
                try:
                    if OUT_FILE_PATH.exists():
                        OUT_FILE_PATH.unlink()
                except OSError as e:
                    print("Warning regarding the following test case: "
                          f"Couldn't remove client output file: {e}")
        else:
            try:
                in_bytes = in_path.read_bytes()
            except OSError:
                return name, False, "Couldn't read the input file."

            try:
                client_bytes, server_bytes = run_system(in_bytes, should_exit)
            except subprocess.SubprocessError as e:
                return name, False, str(e)

        if not may_trigger_errors:
            if ERROR_REGEX.search(server_bytes):
                info = ("Unexpected error message in server output.\n" +
                        present_output(server_bytes, "SERVER OUTPUT"))
                return name, False, info
            if ERROR_REGEX.search(client_bytes):
                info = ("Unexpected error message in client output.\n" +
                        present_output(client_bytes, "CLIENT OUTPUT"))
                return name, False, info

        for old, new in REPLACEMENTS.items():
            client_bytes = client_bytes.replace(old, new)

        if not re.fullmatch(out_pattern, client_bytes, re.VERBOSE):
            info = ("Unexpected client output.\n" +
                    prominent("EXPECTED (regex)") + '\n' +
                    out_pattern.decode('ascii') +
                    present_output(client_bytes, "CLIENT OUTPUT"))
            return name, False, info

        return name, True, None

    return test


def main():
    """Run the test suite."""

    start = time.perf_counter()

    include_file_io = len(sys.argv) <= 1 or sys.argv[1] != 'less'

    tests = list(get_tests(include_file_io))
    num_passed = 0
    num_failed = 0
    try:
        if setup():
            for test in tests:
                if run_test(test):
                    num_passed += 1
                else:
                    num_failed += 1
    finally:
        cleanup()

    if num_passed > 0 or num_failed > 0:
        timing = time.perf_counter() - start
        print(f"Took {timing:.2f} seconds to run {len(tests)} tests.")
        print(f"Passed {num_passed} test{'s' if num_passed != 1 else ''} and "
              f"failed {num_failed}.")


if __name__ == '__main__':
    main()
