#!/usr/bin/env python3
# pragma pylint: disable=too-many-lines
import argparse
import fnmatch
import json
import os
import subprocess
import sys
import traceback
import re
import tty
import functools
from collections import namedtuple
from copy import deepcopy
from pathlib import PurePath
from typing import Any, List, Optional, Tuple, Union
from itertools import islice

from enum import Enum, auto

import colorama # Enables the use of SGR & CUP terminal VT sequences on Windows.
from deepdiff import DeepDiff

"""
Named tuple that holds various regexes used to parse the test specification.
"""
TestRegexesTuple = namedtuple("TestRegexesTuple", [
    "sendRequest", # regex to find requests to be sent & tested
    "findQuotedTag", # regex to find tags wrapped in quotes
    "findTag", # regex to find tags
    "fileDiagnostics", # regex to find diagnostic expectations for a file
    "diagnostic" # regex to find a single diagnostic within the file expectations
])
"""
Instance of the named tuple holding the regexes
"""
TEST_REGEXES = TestRegexesTuple(
    re.compile(R'^// -> (?P<method>[\w\/]+) {'),
    re.compile(R'(?P<tag>"@\w+")'),
    re.compile(R'(?P<tag>@\w+)'),
    re.compile(R'// (?P<testname>\w+):[ ]?(?P<diagnostics>[\w @]*)'),
    re.compile(R'(?P<tag>@\w+) (?P<code>\d\d\d\d)')
)

"""
Named tuple holding regexes to find tags in the solidity code
"""
TagRegexesTuple = namedtuple("TagRegexestuple", ["simpleRange", "multilineRange"])
TAG_REGEXES = TagRegexesTuple(
    re.compile(R"(?P<range>[\^]+) (?P<tag>@\w+)"),
    re.compile(R"\^(?P<delimiter>[()]{1,2}) (?P<tag>@\w+)$")
)


def count_index(lines, start=0):
    """
    Takes an iterable of lines and adds the current byte index so it's available
    when iterating or looping.
    """
    n = start
    for elem in lines:
        yield n, elem
        n += 1 + len(elem)

def tags_only(lines, start=0):
    """
    Filter the lines for tag comments and report line number that tags refer to.
    """
    n = start
    numCommentLines = 0

    def hasTag(line):
        if line.find("// ") != -1:
            for _, regex in TAG_REGEXES._asdict().items():
                if regex.search(line[len("// "):]) is not None:
                    return True
        return False

    for line in lines:
        if hasTag(line):
            numCommentLines += 1
            yield n - numCommentLines, line
        else:
            numCommentLines = 0

        n += 1


def prepend_comments(sequence):
    """
    Prepends a comment indicator to each element
    """
    result = ""
    for line in sequence.splitlines(True):
        result = result + "// " + line
    return result


# {{{ JsonRpcProcess
class BadHeader(Exception):
    def __init__(self, msg: str):
        super().__init__("Bad header: " + msg)


class JsonRpcProcess:
    exe_path: str
    exe_args: List[str]
    process: subprocess.Popen
    trace_io: bool

    def __init__(self, exe_path: str, exe_args: List[str], trace_io: bool = True):
        self.exe_path = exe_path
        self.exe_args = exe_args
        self.trace_io = trace_io

    def __enter__(self):
        self.process = subprocess.Popen(
            [self.exe_path, *self.exe_args],
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE
        )
        return self

    def __exit__(self, exception_type, exception_value, traceback) -> None:
        self.process.kill()
        self.process.wait(timeout=2.0)

    def trace(self, topic: str, message: str) -> None:
        if self.trace_io:
            print(f"{SGR_TRACE}{topic}:{SGR_RESET} {message}")

    def receive_message(self) -> Union[None, dict]:
        # Note, we should make use of timeout to avoid infinite blocking if nothing is received.
        CONTENT_LENGTH_HEADER = "Content-Length: "
        CONTENT_TYPE_HEADER = "Content-Type: "
        if self.process.stdout is None:
            return None
        message_size = None
        while True:
            # read header
            line = self.process.stdout.readline()
            if len(line) == 0:
                # server quit
                return None
            line = line.decode("utf-8")
            if not line.endswith("\r\n"):
                raise BadHeader("missing newline")
            # remove the "\r\n"
            line = line[:-2]
            if line == '':
                break # done with the headers
            if line.startswith(CONTENT_LENGTH_HEADER):
                line = line[len(CONTENT_LENGTH_HEADER):]
                if not line.isdigit():
                    raise BadHeader("size is not int")
                message_size = int(line)
            elif line.startswith(CONTENT_TYPE_HEADER):
                # nothing todo with type for now.
                pass
            else:
                raise BadHeader("unknown header")
        if message_size is None:
            raise BadHeader("missing size")
        rpc_message = self.process.stdout.read(message_size).decode("utf-8")
        json_object = json.loads(rpc_message)
        self.trace('receive_message', json.dumps(json_object, indent=4, sort_keys=True))
        return json_object

    def send_message(self, method_name: str, params: Optional[dict]) -> None:
        if self.process.stdin is None:
            return
        message = {
            'jsonrpc': '2.0',
            'method': method_name,
            'params': params
        }
        json_string = json.dumps(obj=message)
        rpc_message = f"Content-Length: {len(json_string)}\r\n\r\n{json_string}"
        self.trace(f'send_message ({method_name})', json.dumps(message, indent=4, sort_keys=True))
        self.process.stdin.write(rpc_message.encode("utf-8"))
        self.process.stdin.flush()

    def call_method(self, method_name: str, params: Optional[dict]) -> Any:
        self.send_message(method_name, params)
        return self.receive_message()

    def send_notification(self, name: str, params: Optional[dict] = None) -> None:
        self.send_message(name, params)

# }}}

SGR_RESET = '\033[m'
SGR_TRACE = '\033[1;36m'
SGR_NOTICE = '\033[1;35m'
SGR_TEST_BEGIN = '\033[1;33m'
SGR_ASSERT_BEGIN = '\033[1;34m'
SGR_STATUS_OKAY = '\033[1;32m'
SGR_STATUS_FAIL = '\033[1;31m'

class ExpectationFailed(Exception):
    class Part(Enum):
        Diagnostics = auto()
        Methods = auto()

    def __init__(self, reason: str, part):
        self.part = part
        super().__init__(reason)

class JSONExpectationFailed(ExpectationFailed):
    def __init__(self, actual, expected, part):
        self.actual = actual
        self.expected = expected

        expected_pretty = ""

        if expected is not None:
            expected_pretty = json.dumps(expected, sort_keys=True)

        diff = DeepDiff(actual, expected)

        super().__init__(
            f"\n\tExpected {expected_pretty}" + \
            f"\n\tbut got {json.dumps(actual, sort_keys=True)}.\n\t{diff}",
            part
        )


def create_cli_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description="Solidity LSP Test suite")
    parser.set_defaults(fail_fast=False)
    parser.add_argument(
        "-f, --fail-fast",
        dest="fail_fast",
        action="store_true",
        help="Terminates the running tests on first failure."
    )
    parser.set_defaults(trace_io=False)
    parser.add_argument(
        "-T, --trace-io",
        dest="trace_io",
        action="store_true",
        help="Be more verbose by also printing assertions."
    )
    parser.set_defaults(print_assertions=False)
    parser.add_argument(
        "-v, --print-assertions",
        dest="print_assertions",
        action="store_true",
        help="Be more verbose by also printing assertions."
    )
    parser.add_argument(
        "-t, --test-pattern",
        dest="test_pattern",
        type=str,
        default="*",
        help="Filters all available tests by matching against this test pattern (using globbing)",
        nargs="?"
    )
    parser.add_argument(
        "solc_path",
        type=str,
        default="solc",
        help="Path to solc binary to test against",
        nargs="?"
    )
    parser.add_argument(
        "project_root_dir",
        type=str,
        default=f"{os.path.dirname(os.path.realpath(__file__))}/..",
        help="Path to Solidity project's root directory (must be fully qualified).",
        nargs="?"
    )
    return parser

class Counter:
    total: int = 0
    passed: int = 0
    failed: int = 0


# Returns the given marker with the end extended by 'amount'
def extendEnd(marker, amount=1):
    newMarker = deepcopy(marker)
    newMarker["end"]["character"] += amount
    return newMarker

class TestParserException(Exception):
    def __init__(self, incompleteResult, msg: str):
        self.result = incompleteResult
        super().__init__("Failed to parse test specification: " + msg)

class TestParser:
    """
    Parses test specifications.
    Usage example:

    parsed_testcases = TestParser(content).parse()

    # First diagnostics are yielded
    expected_diagnostics = next(parsed_testcases)
    ...
    # Now each request/response pair in the test definition
    for testcase in self.parsed_testcases:
        ...
    """
    RequestAndResponse = namedtuple('RequestAndResponse',
        "method, request, response, responseBegin, responseEnd",
        defaults=(None, None, None, None)
    )
    Diagnostics = namedtuple('Diagnostics', 'tests start end has_header')
    Diagnostic = namedtuple('Diagnostic', 'marker code')

    TEST_START = "// ----"

    def __init__(self, content: str):
        self.content = content
        self.lines = None
        self.current_line_tuple = None

    def parse(self):
        """
        Starts parsing the test specifications.
        Will first yield with the diagnostics expectations as type 'Diagnostics'.
        After that, it will yield once for every Request/Response pair found in
        the file, each time as type 'RequestAndResponse'.

        """
        testDefStartIdx = self.content.rfind(f"\n{self.TEST_START}\n")

        if testDefStartIdx == -1:
            # Set start/end to end of file if there is no test section
            yield self.Diagnostics({}, len(self.content), len(self.content), False)
            return

        self.lines = islice(
            count_index(self.content[testDefStartIdx+1:].splitlines(), testDefStartIdx+1),
            1,
            None
        )
        self.next_line()

        yield self.parseDiagnostics()

        while not self.at_end():
            yield self.RequestAndResponse(**self.parseRequestAndResponse())
            self.next_line()


    def parseDiagnostics(self):
        """
        Parse diagnostic expectations specified in the file.
        Returns a named tuple instance of "Diagnostics"
        """
        diagnostics = { "tests": {}, "has_header": True }

        diagnostics["start"] = self.position()

        while not self.at_end():
            fileDiagMatch = TEST_REGEXES.fileDiagnostics.match(self.current_line())
            if fileDiagMatch is None:
                break

            testDiagnostics = []

            for diagnosticMatch in TEST_REGEXES.diagnostic.finditer(fileDiagMatch.group("diagnostics")):
                testDiagnostics.append(self.Diagnostic(
                    diagnosticMatch.group("tag"),
                    int(diagnosticMatch.group("code"))
                ))

            diagnostics["tests"][fileDiagMatch.group("testname")] = testDiagnostics

            self.next_line()

        diagnostics["end"] = self.position()
        return self.Diagnostics(**diagnostics)


    def parseRequestAndResponse(self):
        RESPONSE_START = "// <- "
        REQUEST_END = "// }"
        COMMENT_PREFIX = "// "

        ret = {}
        start_character = None

        # Parse request header
        requestResult = TEST_REGEXES.sendRequest.match(self.current_line())
        if requestResult is not None:
            ret["method"] = requestResult.group("method")
            ret["request"] = "{\n"
        else:
            raise TestParserException(ret, "Method for request not found")

        self.next_line()

        # Search for request block end
        while not self.at_end():
            line = self.current_line()
            ret["request"] += line[len(COMMENT_PREFIX):] + "\n"

            self.next_line()

            if line.startswith(REQUEST_END):
                break

            # Reached end without finding request_end. Abort.
            if self.at_end():
                raise TestParserException(ret, "Request body not found")


        # Parse response header
        if self.current_line().startswith(RESPONSE_START):
            start_character = self.current_line()[len(RESPONSE_START)]
            if start_character not in ("{", "["):
                raise TestParserException(ret, "Response header malformed")
            ret["response"] = self.current_line()[len(RESPONSE_START):] + "\n"
            ret["responseBegin"] = self.position()
        else:
            raise TestParserException(ret, "Response header not found")

        self.next_line()

        end_character = "}" if start_character == "{" else "]"

        # Search for request block end
        while not self.at_end():
            ret["response"] += self.current_line()[len(COMMENT_PREFIX):] + "\n"

            if self.current_line().startswith(f"// {end_character}"):
                ret["responseEnd"] = self.position() + len(self.current_line())
                break

            self.next_line()

            # Reached end without finding block_end. Abort.
            if self.at_end():
                raise TestParserException(ret, "Response footer not found")

        return ret

    def next_line(self):
        self.current_line_tuple = next(self.lines, None)

    def current_line(self):
        return self.current_line_tuple[1]

    def position(self):
        """
        Returns current byte position
        """
        if self.current_line_tuple is None:
            return len(self.content)
        return self.current_line_tuple[0]

    def at_end(self):
        """
        Returns True if we exhausted the lines
        """
        return self.current_line_tuple is None

class FileTestRunner:
    """
    Runs all tests in a given file.
    It is required to call test_diagnostics() before calling test_methods().

    When a test fails, asks the user how to proceed.
    Offers automatic test expectation updates and rerunning of the tests.
    """

    class TestResult(Enum):
        SuccessOrIgnored = auto()
        Reparse = auto()

    def __init__(self, test_name, solc, suite):
        self.test_name = test_name
        self.suite = suite
        self.solc = solc
        self.open_tests = []
        self.content = self.suite.get_test_file_contents(self.test_name)
        self.markers = self.suite.get_file_tags(self.test_name)
        self.parsed_testcases = None
        self.expected_diagnostics = None

    def test_diagnostics(self):
        """
        Test that the expected diagnostics match the actual diagnostics
        """
        try:
            self.parsed_testcases = TestParser(self.content).parse()

            # Process diagnostics first
            self.expected_diagnostics = next(self.parsed_testcases)
            assert isinstance(self.expected_diagnostics, TestParser.Diagnostics) is True

            tests = self.expected_diagnostics.tests

            # Add our own test diagnostics if they didn't exist
            if self.test_name not in tests:
                tests[self.test_name] = []

            published_diagnostics = \
                self.suite.open_file_and_wait_for_diagnostics(self.solc, self.test_name)

            for diagnostics in published_diagnostics:
                self.open_tests.append(diagnostics["uri"].replace(self.suite.project_root_uri + "/", "")[:-len(".sol")])

            self.suite.expect_equal(
                len(published_diagnostics),
                len(tests),
                description="Amount of reports does not match!")

            for diagnostics in published_diagnostics:
                testname = diagnostics["uri"].replace(self.suite.project_root_uri + "/", "")[:-len(".sol")]

                expected_diagnostics = tests[testname]
                self.suite.expect_equal(
                    len(diagnostics["diagnostics"]),
                    len(expected_diagnostics),
                    description="Unexpected amount of diagnostics"
                )
                markers = self.suite.get_file_tags(testname)
                for actual_diagnostic in diagnostics["diagnostics"]:
                    expected_diagnostic = next((diagnostic for diagnostic in
                        expected_diagnostics if actual_diagnostic['range'] ==
                        markers[diagnostic.marker]), None)

                    if expected_diagnostic is None:
                        raise ExpectationFailed(
                            f"Unexpected diagnostic: {json.dumps(actual_diagnostic, indent=4, sort_keys=True)}",
                            ExpectationFailed.Part.Diagnostics
                        )

                    self.suite.expect_diagnostic(
                        actual_diagnostic,
                        code=expected_diagnostic.code,
                        marker=markers[expected_diagnostic.marker]
                    )

        except Exception as e:
            print(e)
            self.close_all_open_files()
            raise

    def close_all_open_files(self):
        for test in self.open_tests:
            self.solc.send_message(
                'textDocument/didClose',
                { 'textDocument': { 'uri': self.suite.get_test_file_uri(test) }}
            )
            self.suite.wait_for_diagnostics(self.solc)

        self.open_tests.clear()

    def test_methods(self) -> bool:
        """
        Test all methods. Returns False if a reparsing is required, else True
        """
        try:
            # Now handle each request/response pair in the test definition
            for testcase in self.parsed_testcases:
                try:
                    self.run_testcase(testcase)
                except JSONExpectationFailed as e:
                    result = self.user_interaction_failed_method_test(testcase, e.actual, e.expected)

                    if result == self.TestResult.Reparse:
                        return False

            return True
        except TestParserException as e:
            print(e)
            print(e.result)
            raise
        finally:
            self.close_all_open_files()

    def user_interaction_failed_method_test(self, testcase, actual, expected):
        actual_pretty = self.suite.replace_ranges_with_tags(actual)

        if expected is None:
            print("Failed to parse expected response, received:\n" + actual)
        else:
            print("Expected:\n" + \
                self.suite.replace_ranges_with_tags(expected) + \
                "\nbut got:\n" + actual_pretty
            )

        while True:
            print("(u)pdate/(r)etry/(i)gnore?")
            user_response = sys.stdin.read(1)
            if user_response == "i":
                return self.TestResult.SuccessOrIgnored

            if user_response == "u":
                actual = actual["result"]
                self.content = self.content[:testcase.responseBegin] + \
                    prepend_comments("<- " + self.suite.replace_ranges_with_tags(actual)) + \
                    self.content[testcase.responseEnd:]

                with open(self.suite.get_test_file_path(self.test_name), mode="w", encoding="utf-8", newline='') as f:
                    f.write(self.content)
                return self.TestResult.Reparse
            if user_response == "r":
                return self.TestResult.Reparse

            print("Invalid response.")


    def run_testcase(self, testcase: TestParser.RequestAndResponse):
        """
        Runs the given testcase.
        """
        requestBodyJson = self.parse_json_with_tags(testcase.request, self.markers)
        # add textDocument/uri if missing
        if 'textDocument' not in requestBodyJson:
            requestBodyJson['textDocument'] = { 'uri': self.suite.get_test_file_uri(self.test_name) }
        actualResponseJson = self.solc.call_method(testcase.method, requestBodyJson)

        # simplify response
        for result in actualResponseJson["result"]:
            result["uri"] = result["uri"].replace(self.suite.project_root_uri + "/", "")
        if "jsonrpc" in actualResponseJson:
            actualResponseJson.pop("jsonrpc")

        try:
            expectedResponseJson = self.parse_json_with_tags(testcase.response, self.markers)
        except json.decoder.JSONDecodeError:
            expectedResponseJson = None

        expectedResponseJson = { "result": expectedResponseJson }

        self.suite.expect_equal(
            actualResponseJson,
            expectedResponseJson,
            f"Request failed: \n{testcase.request}",
            ExpectationFailed.Part.Methods
        )


    def parse_json_with_tags(self, content, markersFallback):
        """
        Replaces any tags with their actual content and parsers the result as
        json to return it.
        """
        split_by_tag = TEST_REGEXES.findTag.split(content)

        # add quotes so we can parse it as json
        contentReplaced = '"'.join(split_by_tag)
        contentJson = json.loads(contentReplaced)

        def replace_tag(data, markers):

            if isinstance(data, list):
                for el in data:
                    replace_tag(el, markers)
                return data

            # Check if we need markers from a specific file
            # Needs to be done before the loop or it might be called only after
            # we found "range" or "position"
            if "uri" in data:
                markers = self.suite.get_file_tags(data["uri"][:-len(".sol")])

            for key, val in data.items():
                if key == "range":
                    for tag, tagRange in markers.items():
                        if tag == val:
                            data[key] = tagRange
                elif key == "position":
                    for tag, tagRange in markers.items():
                        if tag == val:
                            data[key] = tagRange["start"]
                elif isinstance(val, dict):
                    replace_tag(val, markers)
                elif isinstance(val, list):
                    for el in val:
                        replace_tag(el, markers)
            return data

        return replace_tag(contentJson, markersFallback)


class SolidityLSPTestSuite: # {{{
    test_counter = Counter()
    assertion_counter = Counter()
    print_assertions: bool = False
    trace_io: bool = False
    fail_fast: bool = False
    test_pattern: str

    def __init__(self):
        colorama.init()
        args = create_cli_parser().parse_args()
        self.solc_path = args.solc_path
        self.project_root_dir = os.path.realpath(args.project_root_dir) + "/test/libsolidity/lsp"
        self.project_root_uri = PurePath(self.project_root_dir).as_uri()
        self.print_assertions = args.print_assertions
        self.trace_io = args.trace_io
        self.test_pattern = args.test_pattern
        self.fail_fast = args.fail_fast

        print(f"{SGR_NOTICE}test pattern: {self.test_pattern}{SGR_RESET}")

    def main(self) -> int:
        """
        Runs all test cases.
        Returns 0 on success and the number of failing assertions (capped to 127) otherwise.
        """
        all_tests = sorted([
            str(name)[5:]
            for name in dir(SolidityLSPTestSuite)
            if callable(getattr(SolidityLSPTestSuite, name)) and name.startswith("test_")
        ])
        filtered_tests = fnmatch.filter(all_tests, self.test_pattern)
        for method_name in filtered_tests:
            test_fn = getattr(self, 'test_' + method_name)
            title: str = test_fn.__name__[5:]
            print(f"{SGR_TEST_BEGIN}Testing {title} ...{SGR_RESET}")
            try:
                with JsonRpcProcess(self.solc_path, ["--lsp"], trace_io=self.trace_io) as solc:
                    test_fn(solc)
                    self.test_counter.passed += 1
            except ExpectationFailed:
                self.test_counter.failed += 1
                print(traceback.format_exc())
                if self.fail_fast:
                    break
            except Exception as e: # pragma pylint: disable=broad-except
                self.test_counter.failed += 1
                print(f"Unhandled exception {e.__class__.__name__} caught: {e}")
                print(traceback.format_exc())
                if self.fail_fast:
                    break

        print(
            f"\n{SGR_NOTICE}Summary:{SGR_RESET}\n\n"
            f"  Test cases: {self.test_counter.passed} passed, {self.test_counter.failed} failed\n"
            f"  Assertions: {self.assertion_counter.passed} passed, {self.assertion_counter.failed} failed\n"
        )

        return min(max(self.test_counter.failed, self.assertion_counter.failed), 127)

    def setup_lsp(self, lsp: JsonRpcProcess, expose_project_root=True):
        """
        Prepares the solc LSP server by calling `initialize`,
        and `initialized` methods.
        """
        params = {
            'processId': None,
            'rootUri': self.project_root_uri,
            'trace': 'off',
            'initializationOptions': {},
            'capabilities': {
                'textDocument': {
                    'publishDiagnostics': {'relatedInformation': True}
                },
                'workspace': {
                    'applyEdit': True,
                    'configuration': True,
                    'didChangeConfiguration': {'dynamicRegistration': True},
                    'workspaceEdit': {'documentChanges': True},
                    'workspaceFolders': True
                }
            }
        }
        if not expose_project_root:
            params['rootUri'] = None
        lsp.call_method('initialize', params)
        lsp.send_notification('initialized')
        # Enable traces to receive the amount of expected diagnostics before
        # actually receiving them.
        lsp.send_message("$/setTrace", { 'value': 'messages' })

    # {{{ helpers
    def get_test_file_path(self, test_case_name):
        return f"{self.project_root_dir}/{test_case_name}.sol"

    def get_test_file_uri(self, test_case_name):
        return PurePath(self.get_test_file_path(test_case_name)).as_uri()

    def get_test_file_contents(self, test_case_name):
        """
        Reads the file contents from disc for a given test case.
        The `test_case_name` will be the basename of the file
        in the test path (test/libsolidity/lsp).
        """
        with open(self.get_test_file_path(test_case_name), mode="r", encoding="utf-8", newline='') as f:
            return f.read()

    def require_params_for_method(self, method_name: str, message: dict) -> Any:
        """
        Ensures the given RPC message does contain the
        field 'method' with the given method name,
        and then returns its passed params.
        An exception is raised on expectation failures.
        """
        assert message is not None
        if 'error' in message.keys():
            code = message['error']["code"]
            text = message['error']['message']
            raise RuntimeError(f"Error {code} received. {text}")
        if 'method' not in message.keys():
            raise RuntimeError("No method received but something else.")
        self.expect_equal(message['method'], method_name, description="Ensure expected method name")
        return message['params']

    def wait_for_diagnostics(self, solc: JsonRpcProcess) -> List[dict]:
        """
        Return all published diagnostic reports sorted by file URI.
        """
        reports = []

        num_files = solc.receive_message()["params"]["openFileCount"]

        for _ in range(0, num_files):
            message = solc.receive_message()

            assert message is not None # This can happen if the server aborts early.

            reports.append(
                self.require_params_for_method(
                    'textDocument/publishDiagnostics',
                    message,
                )
            )

        return sorted(reports, key=lambda x: x['uri'])

    def fetch_and_format_diagnostics(self, solc: JsonRpcProcess, test):
        expectations = ""

        published_diagnostics = self.open_file_and_wait_for_diagnostics(solc, test)

        for diagnostics in published_diagnostics:
            testname = diagnostics["uri"].replace(self.project_root_uri + "/", "")[:-len(".sol")]

            # Skip empty diagnostics within the same file
            if len(diagnostics["diagnostics"]) == 0 and testname == test:
                continue

            expectations += f"// {testname}:"

            for diagnostic in diagnostics["diagnostics"]:
                tag = self.find_tag_with_range(testname, diagnostic['range'])

                if tag is None:
                    raise Exception(f"No tag found for diagnostic range {diagnostic['range']}")

                expectations += f" {tag} {diagnostic['code']}"
            expectations += "\n"

        return expectations

    def update_diagnostics_in_file(
        self,
        solc: JsonRpcProcess,
        test,
        content,
        current_diagnostics: TestParser.Diagnostics
    ):
        test_header = ""

        if not current_diagnostics.has_header:
            test_header = f"{TestParser.TEST_START}\n"

        content = content[:current_diagnostics.start] + \
            test_header + \
            self.fetch_and_format_diagnostics(solc, test) + \
            content[current_diagnostics.end:]

        with open(self.get_test_file_path(test), mode="w", encoding="utf-8", newline='') as f:
            f.write(content)

        return content

    def open_file_and_wait_for_diagnostics(
        self,
        solc_process: JsonRpcProcess,
        test_case_name: str,
    ) -> List[Any]:
        """
        Opens file for given test case and waits for diagnostics to be published.
        """
        solc_process.send_message(
            'textDocument/didOpen',
            {
                'textDocument':
                {
                    'uri': self.get_test_file_uri(test_case_name),
                    'languageId': 'Solidity',
                    'version': 1,
                    'text': self.get_test_file_contents(test_case_name)
                }
            }
        )
        return self.wait_for_diagnostics(solc_process)

    def expect_equal(
        self,
        actual,
        expected,
        description="Equality",
        part=ExpectationFailed.Part.Diagnostics
    ) -> None:
        self.assertion_counter.total += 1
        prefix = f"[{self.assertion_counter.total}] {SGR_ASSERT_BEGIN}{description}: "
        diff = DeepDiff(actual, expected)
        if len(diff) == 0:
            self.assertion_counter.passed += 1
            if self.print_assertions:
                print(prefix + SGR_STATUS_OKAY + 'OK' + SGR_RESET)
            return

        # Failed assertions are always printed.
        self.assertion_counter.failed += 1
        print(prefix + SGR_STATUS_FAIL + 'FAILED' + SGR_RESET)
        raise JSONExpectationFailed(actual, expected, part)

    def expect_empty_diagnostics(self, published_diagnostics: List[dict]) -> None:
        self.expect_equal(len(published_diagnostics), 1, "one publish diagnostics notification")
        self.expect_equal(len(published_diagnostics[0]['diagnostics']), 0, "should not contain diagnostics")

    def expect_diagnostic(
        self,
        diagnostic,
        code: int,
        lineNo: int = None,
        startEndColumns: Tuple[int, int] = None,
        marker: {} = None
    ):
        self.expect_equal(
            diagnostic['code'],
            code,
            ExpectationFailed.Part.Diagnostics,
            f'diagnostic: {code}'
        )

        if marker:
            self.expect_equal(
                diagnostic['range'],
                marker,
                ExpectationFailed.Part.Diagnostics,
                "diagnostic: check range"
            )

        else:
            assert len(startEndColumns) == 2
            [startColumn, endColumn] = startEndColumns
            self.expect_equal(
                diagnostic['range'],
                {
                    'start': {'character': startColumn, 'line': lineNo},
                    'end': {'character': endColumn, 'line': lineNo}
                },
                ExpectationFailed.Part.Diagnostics,
                "diagnostic: check range"
            )


    def expect_location(
        self,
        obj: dict,
        uri: str,
        lineNo: int,
        startEndColumns: Tuple[int, int]
    ):
        """
        obj is an JSON object containing two keys:
            - 'uri': a string of the document URI
            - 'range': the location range, two child objects 'start' and 'end',
                        each having a 'line' and 'character' integer value.
        """
        [startColumn, endColumn] = startEndColumns
        self.expect_equal(obj['uri'], uri)
        self.expect_equal(obj['range']['start']['line'], lineNo)
        self.expect_equal(obj['range']['start']['character'], startColumn)
        self.expect_equal(obj['range']['end']['line'], lineNo)
        self.expect_equal(obj['range']['end']['character'], endColumn)

    def expect_goto_definition_location(
        self,
        solc: JsonRpcProcess,
        document_uri: str,
        document_position: Tuple[int, int],
        expected_uri: str,
        expected_lineNo: int,
        expected_startEndColumns: Tuple[int, int],
        description: str
    ):
        response = solc.call_method(
            'textDocument/definition',
            {
                'textDocument': {
                    'uri': document_uri,
                },
                'position': {
                    'line': document_position[0],
                    'character': document_position[1]
                }
            }
        )
        message = "Goto definition (" + description + ")"
        self.expect_equal(len(response['result']), 1, message)
        self.expect_location(response['result'][0], expected_uri, expected_lineNo, expected_startEndColumns)


    def find_tag_with_range(self, test, target_range):
        """
        Find and return the tag that represents the requested range otherwise
        return None.
        """
        markers = self.get_file_tags(test)

        for tag, tag_range in markers.items():
            if tag_range == target_range:
                return str(tag)

        return None

    def replace_ranges_with_tags(self, content):
        """
        Replace matching ranges with "@<tagname>".
        """

        def recursive_iter(obj):
            if isinstance(obj, dict):
                yield obj
                for item in obj.values():
                    yield from recursive_iter(item)
            elif any(isinstance(obj, t) for t in (list, tuple)):
                for item in obj:
                    yield from recursive_iter(item)

        for item in recursive_iter(content):
            if "uri" in item and "range" in item:
                markers = self.get_file_tags(item["uri"][:-len(".sol")])
                for tag, tagRange in markers.items():
                    if tagRange == item["range"]:
                        item["range"] = str(tag)

        # Convert JSON to string and split it at the quoted tags
        split_by_tag = TEST_REGEXES.findQuotedTag.split(json.dumps(content, indent=4, sort_keys=True))

        # remove the quotes and return result
        return "".join(map(lambda p: p[1:-1] if p.startswith('"@') else p, split_by_tag))

    def user_interaction_failed_diagnostics(
        self,
        solc: JsonRpcProcess,
        test,
        content,
        current_diagnostics: TestParser.Diagnostics
    ):
        """
        Asks the user how to proceed after an error.
        Returns True if the test/file should be ignored, otherwise False
        """
        while True:
            print("(u)pdate/(r)etry/(s)kip file?")
            user_response = sys.stdin.read(1)
            if user_response == "u":
                while True:
                    try:
                        self.update_diagnostics_in_file(solc, test, content, current_diagnostics)
                        return False
                    # pragma pylint: disable=broad-except
                    except Exception as e:
                        print(e)
                        if ret := self.user_interaction_failed_autoupdate(test):
                            return ret
            elif user_response == 's':
                return True
            elif user_response == 'r':
                return False

    def user_interaction_failed_autoupdate(self, test):
        print("(e)dit/(r)etry/(s)kip file?")
        user_response = sys.stdin.read(1)
        if user_response == "r":
            print("retrying...")
            # pragma pylint: disable=no-member
            self.get_file_tags.cache_clear()
            return False
        if user_response == "e":
            editor = os.environ.get('VISUAL', os.environ.get('EDITOR', 'vi'))
            subprocess.run(
                f'{editor} {self.get_test_file_path(test)}',
                shell=True,
                check=True
            )
            # pragma pylint: disable=no-member
            self.get_file_tags.cache_clear()
        elif user_response == "s":
            print("skipping...")

        return True

    # }}}

    # {{{ actual tests

    def test_publish_diagnostics_errors_multiline(self, solc: JsonRpcProcess) -> None:
        self.setup_lsp(solc)
        TEST_NAME = 'publish_diagnostics_3'
        published_diagnostics = self.open_file_and_wait_for_diagnostics(solc, TEST_NAME)

        self.expect_equal(len(published_diagnostics), 1, "One published_diagnostics message")
        report = published_diagnostics[0]

        self.expect_equal(report['uri'], self.get_test_file_uri(TEST_NAME), "Correct file URI")
        diagnostics = report['diagnostics']

        self.expect_equal(len(diagnostics), 1, "1 diagnostic messages")
        self.expect_equal(diagnostics[0]['code'], 3656, "diagnostic: check code")
        self.expect_equal(
            diagnostics[0]['range'],
            {
                'start': {'character': 0, 'line': 7},
                'end': {'character': 1, 'line': 10}
            },
            "diagnostic: check range"
        )

    def test_textDocument_didOpen_with_relative_import(self, solc: JsonRpcProcess) -> None:
        self.setup_lsp(solc)
        TEST_NAME = 'didOpen_with_import'
        published_diagnostics = self.open_file_and_wait_for_diagnostics(solc, TEST_NAME)

        self.expect_equal(len(published_diagnostics), 2, "Diagnostic reports for 2 files")

        # primary file:
        report = published_diagnostics[0]
        self.expect_equal(report['uri'], self.get_test_file_uri(TEST_NAME), "Correct file URI")
        self.expect_equal(len(report['diagnostics']), 0, "no diagnostics")

        # imported file (./lib.sol):
        report = published_diagnostics[1]
        self.expect_equal(report['uri'], self.get_test_file_uri('lib'), "Correct file URI")
        self.expect_equal(len(report['diagnostics']), 1, "one diagnostic")
        marker = self.get_file_tags("lib")["@diagnostics"]
        self.expect_diagnostic(report['diagnostics'][0], code=2072, marker=marker)

    @functools.lru_cache # pragma pylint: disable=lru-cache-decorating-method
    def get_file_tags(self, test_name: str, verbose=False):
        """
        Finds all tags (e.g. @tagname) in the given test and returns them as a
        dictionary having the following structure: {
            "@tagname": {
             "start": { "character": 3, "line": 2 },
             "end":   { "character": 30, "line": 2 }
            }
        }
        """
        content = self.get_test_file_contents(test_name)

        markers = {}

        for lineNum, line in tags_only(content.splitlines()):
            commentStart = line.find("//")

            for kind, regex in TAG_REGEXES._asdict().items():
                for match in regex.finditer(line[commentStart:]):
                    if kind == "simpleRange":
                        markers[match.group("tag")] = {
                            "start": {
                                "line": lineNum,
                                "character": match.start("range") + commentStart
                            },
                            "end": {
                                "line": lineNum,
                                "character": match.end("range") + commentStart
                        }}
                    elif kind == "multilineRange":
                        if match.group("delimiter") == "(":
                            markers[match.group("tag")] = \
                                { "start": { "line": lineNum, "character": 0 } }
                        elif match.group("delimiter") == ")":
                            markers[match.group("tag")]["end"] = \
                                { "line": lineNum, "character": 0 }

        if verbose:
            print(markers)
        return markers


    def test_didChange_in_A_causing_error_in_B(self, solc: JsonRpcProcess) -> None:
        # Reusing another test but now change some file that generates an error in the other.
        self.test_textDocument_didOpen_with_relative_import(solc)
        marker = self.get_file_tags("lib")["@addFunction"]
        self.open_file_and_wait_for_diagnostics(solc, 'lib')
        solc.send_message(
            'textDocument/didChange',
            {
                'textDocument':
                {
                    'uri': self.get_test_file_uri('lib')
                },
                'contentChanges':
                [
                    {
                        'range': marker,
                        'text': "" # deleting function `add`
                    }
                ]
            }
        )
        published_diagnostics = self.wait_for_diagnostics(solc)
        self.expect_equal(len(published_diagnostics), 2, "Diagnostic reports for 2 files")

        # Main file now contains a new diagnostic
        report = published_diagnostics[0]
        self.expect_equal(report['uri'], self.get_test_file_uri('didOpen_with_import'))
        diagnostics = report['diagnostics']
        marker = self.get_file_tags("didOpen_with_import")["@diagnostics"]
        self.expect_equal(len(diagnostics), 1, "now, no diagnostics")
        self.expect_diagnostic(diagnostics[0], code=9582, marker=marker)

        # The modified file retains the same diagnostics.
        report = published_diagnostics[1]
        self.expect_equal(report['uri'], self.get_test_file_uri('lib'))
        self.expect_equal(len(report['diagnostics']), 0)
        # The warning went away because the compiler aborts further processing after the error.

    def test_textDocument_didOpen_with_relative_import_without_project_url(self, solc: JsonRpcProcess) -> None:
        self.setup_lsp(solc, expose_project_root=False)
        TEST_NAME = 'didOpen_with_import'
        published_diagnostics = self.open_file_and_wait_for_diagnostics(solc, TEST_NAME)
        self.verify_didOpen_with_import_diagnostics(published_diagnostics)

    def verify_didOpen_with_import_diagnostics(
        self,
        published_diagnostics: List[Any],
        main_file_name='didOpen_with_import'
    ):
        self.expect_equal(len(published_diagnostics), 2, "Diagnostic reports for 2 files")

        # primary file:
        report = published_diagnostics[0]
        self.expect_equal(report['uri'], self.get_test_file_uri(main_file_name), "Correct file URI")
        self.expect_equal(len(report['diagnostics']), 0, "one diagnostic")

        # imported file (./lib.sol):
        report = published_diagnostics[1]
        self.expect_equal(report['uri'], self.get_test_file_uri('lib'), "Correct file URI")
        self.expect_equal(len(report['diagnostics']), 1, "one diagnostic")

        markers = self.get_file_tags('lib')
        marker = markers["@diagnostics"]
        self.expect_diagnostic(report['diagnostics'][0], code=2072, marker=marker)

    def test_generic(self, solc: JsonRpcProcess) -> None:
        self.setup_lsp(solc)

        STATIC_TESTS = ['didChange_template', 'didOpen_with_import', 'publish_diagnostics_3']

        tests = filter(
            lambda x: x not in STATIC_TESTS,
            map(lambda x: x[:-len(".sol")], os.listdir(self.project_root_dir))
        )

        for test in tests:
            try_again = True
            print(f"Running test {test}")

            while try_again:
                runner = FileTestRunner(test, solc, self)

                try:
                    runner.test_diagnostics()
                    try_again = not runner.test_methods()
                except ExpectationFailed as e:
                    print(e)

                    if e.part == e.Part.Diagnostics:
                        try_again = not self.user_interaction_failed_diagnostics(
                            solc,
                            test,
                            runner.content,
                            runner.expected_diagnostics
                        )
                    else:
                        raise

    def test_textDocument_didChange_updates_diagnostics(self, solc: JsonRpcProcess) -> None:
        self.setup_lsp(solc)
        TEST_NAME = 'publish_diagnostics_1'
        published_diagnostics = self.open_file_and_wait_for_diagnostics(solc, TEST_NAME)
        self.expect_equal(len(published_diagnostics), 1, "One published_diagnostics message")
        report = published_diagnostics[0]
        self.expect_equal(report['uri'], self.get_test_file_uri(TEST_NAME), "Correct file URI")
        diagnostics = report['diagnostics']
        self.expect_equal(len(diagnostics), 3, "3 diagnostic messages")
        markers = self.get_file_tags(TEST_NAME)
        self.expect_diagnostic(diagnostics[0], code=6321, marker=markers["@unusedReturnVariable"])
        self.expect_diagnostic(diagnostics[1], code=2072, marker=markers["@unusedVariable"])
        self.expect_diagnostic(diagnostics[2], code=2072, marker=markers["@unusedContractVariable"])

        solc.send_message(
            'textDocument/didChange',
            {
                'textDocument': {
                    'uri': self.get_test_file_uri(TEST_NAME)
                },
                'contentChanges': [
                    {
                        'range': extendEnd(markers["@unusedVariable"]),
                        'text': ""
                    }
                ]
            }
        )
        published_diagnostics = self.wait_for_diagnostics(solc)
        self.expect_equal(len(published_diagnostics), 1)
        report = published_diagnostics[0]
        self.expect_equal(report['uri'], self.get_test_file_uri(TEST_NAME), "Correct file URI")
        diagnostics = report['diagnostics']
        self.expect_equal(len(diagnostics), 2)
        self.expect_diagnostic(diagnostics[0], code=6321, marker=markers["@unusedReturnVariable"])
        self.expect_diagnostic(diagnostics[1], code=2072, marker=markers["@unusedContractVariable"])

    def test_textDocument_didChange_delete_line_and_close(self, solc: JsonRpcProcess) -> None:
        # Reuse this test to prepare and ensure it is as expected
        self.test_textDocument_didOpen_with_relative_import(solc)
        self.open_file_and_wait_for_diagnostics(solc, 'lib')

        marker = self.get_file_tags('lib')["@diagnostics"]

        # lib.sol: Fix the unused variable message by removing it.
        solc.send_message(
            'textDocument/didChange',
            {
                'textDocument':
                {
                    'uri': self.get_test_file_uri('lib')
                },
                'contentChanges': # delete the in-body statement: `uint unused;`
                [
                    {
                        'range': extendEnd(marker),
                        'text': ""
                    }
                ]
            }
        )
        published_diagnostics = self.wait_for_diagnostics(solc)
        self.expect_equal(len(published_diagnostics), 2, "published diagnostics count")
        report1 = published_diagnostics[0]
        self.expect_equal(report1['uri'], self.get_test_file_uri('didOpen_with_import'), "Correct file URI")
        self.expect_equal(len(report1['diagnostics']), 0, "no diagnostics in didOpen_with_import.sol")
        report2 = published_diagnostics[1]
        self.expect_equal(report2['uri'], self.get_test_file_uri('lib'), "Correct file URI")
        self.expect_equal(len(report2['diagnostics']), 0, "no diagnostics in lib.sol")

        # Now close the file and expect the warning to re-appear
        solc.send_message(
            'textDocument/didClose',
            { 'textDocument': { 'uri': self.get_test_file_uri('lib') }}
        )

        published_diagnostics = self.wait_for_diagnostics(solc)
        self.verify_didOpen_with_import_diagnostics(published_diagnostics)

    def test_textDocument_opening_two_new_files_edit_and_close(self, solc: JsonRpcProcess) -> None:
        """
        Open two new files A and B, let A import B, expect no error,
        then close B and now expect the error of file B not being found.
        """

        self.setup_lsp(solc)
        FILE_A_URI = 'file:///a.sol'
        solc.send_message('textDocument/didOpen', {
            'textDocument': {
                'uri': FILE_A_URI,
                'languageId': 'Solidity',
                'version': 1,
                'text': ''.join([
                    '// SPDX-License-Identifier: UNLICENSED\n',
                    'pragma solidity >=0.8.0;\n',
                ])
            }
        })
        reports = self.wait_for_diagnostics(solc)
        self.expect_equal(len(reports), 1, "one publish diagnostics notification")
        self.expect_equal(len(reports[0]['diagnostics']), 0, "should not contain diagnostics")

        FILE_B_URI = 'file:///b.sol'
        solc.send_message('textDocument/didOpen', {
            'textDocument': {
                'uri': FILE_B_URI,
                'languageId': 'Solidity',
                'version': 1,
                'text': ''.join([
                    '// SPDX-License-Identifier: UNLICENSED\n',
                    'pragma solidity >=0.8.0;\n',
                ])
            }
        })
        reports = self.wait_for_diagnostics(solc)
        self.expect_equal(len(reports), 2, "one publish diagnostics notification")
        self.expect_equal(len(reports[0]['diagnostics']), 0, "should not contain diagnostics")
        self.expect_equal(len(reports[1]['diagnostics']), 0, "should not contain diagnostics")

        solc.send_message('textDocument/didChange', {
            'textDocument': {
                'uri': FILE_A_URI
            },
            'contentChanges': [
                {
                    'range': {
                        'start': { 'line': 2, 'character': 0 },
                        'end': { 'line': 2, 'character': 0 }
                    },
                    'text': 'import "./b.sol";\n'
                }
            ]
        })
        reports = self.wait_for_diagnostics(solc)
        self.expect_equal(len(reports), 2, "one publish diagnostics notification")
        self.expect_equal(len(reports[0]['diagnostics']), 0, "should not contain diagnostics")
        self.expect_equal(len(reports[1]['diagnostics']), 0, "should not contain diagnostics")

        solc.send_message(
            'textDocument/didClose',
            { 'textDocument': { 'uri': FILE_B_URI }}
        )
        # We only get one diagnostics message since the diagnostics for b.sol was empty.
        reports = self.wait_for_diagnostics(solc)
        self.expect_equal(len(reports), 1, "one publish diagnostics notification")
        self.expect_diagnostic(reports[0]['diagnostics'][0], 6275, 2, (0, 17)) # a.sol: File B not found
        self.expect_equal(reports[0]['uri'], FILE_A_URI, "Correct uri")

    def test_textDocument_closing_virtual_file_removes_imported_real_file(self, solc: JsonRpcProcess) -> None:
        """
        We open a virtual file that imports a real file with a warning.
        Once we close the virtual file, the warning is removed from the diagnostics,
        since the real file is not considered part of the project anymore.
        """

        self.setup_lsp(solc)
        FILE_A_URI = f'{self.project_root_uri}/a.sol'
        solc.send_message('textDocument/didOpen', {
            'textDocument': {
                'uri': FILE_A_URI,
                'languageId': 'Solidity',
                'version': 1,
                'text':
                    '// SPDX-License-Identifier: UNLICENSED\n'
                    'pragma solidity >=0.8.0;\n'
                    'import "./lib.sol";\n'
            }
        })
        reports = self.wait_for_diagnostics(solc)
        self.expect_equal(len(reports), 2, '')
        self.expect_equal(len(reports[0]['diagnostics']), 0, "should not contain diagnostics")

        marker = self.get_file_tags("lib")["@diagnostics"]

        # unused variable in lib.sol
        self.expect_diagnostic(reports[1]['diagnostics'][0], code=2072, marker=marker)

        # Now close the file and expect the warning for lib.sol to be removed
        solc.send_message(
            'textDocument/didClose',
            { 'textDocument': { 'uri': FILE_A_URI }}
        )
        reports = self.wait_for_diagnostics(solc)
        self.expect_equal(len(reports), 1, '')
        self.expect_equal(reports[0]['uri'], f'{self.project_root_uri}/lib.sol', "")
        self.expect_equal(len(reports[0]['diagnostics']), 0, "should not contain diagnostics")

    def test_textDocument_didChange_at_eol(self, solc: JsonRpcProcess) -> None:
        """
        Append at one line and insert a new one below.
        """
        self.setup_lsp(solc)
        FILE_NAME = 'didChange_template'
        FILE_URI = self.get_test_file_uri(FILE_NAME)
        solc.send_message('textDocument/didOpen', {
            'textDocument': {
                'uri': FILE_URI,
                'languageId': 'Solidity',
                'version': 1,
                'text': self.get_test_file_contents(FILE_NAME)
            }
        })
        published_diagnostics = self.wait_for_diagnostics(solc)
        self.expect_equal(len(published_diagnostics), 1, "one publish diagnostics notification")
        self.expect_equal(len(published_diagnostics[0]['diagnostics']), 0, "no diagnostics")
        solc.send_message('textDocument/didChange', {
            'textDocument': {
                'uri': FILE_URI
            },
            'contentChanges': [
                {
                    'range': {
                        'start': { 'line': 6, 'character': 0 },
                        'end': { 'line': 6, 'character': 0 }
                    },
                    'text': " f"
                }
            ]
        })
        published_diagnostics = self.wait_for_diagnostics(solc)
        self.expect_equal(len(published_diagnostics), 1, "one publish diagnostics notification")
        report2 = published_diagnostics[0]
        self.expect_equal(report2['uri'], FILE_URI, "Correct file URI")
        self.expect_equal(len(report2['diagnostics']), 1, "one diagnostic")
        self.expect_diagnostic(report2['diagnostics'][0], 7858, 6, (1, 2))

        solc.send_message('textDocument/didChange', {
            'textDocument': { 'uri': FILE_URI },
            'contentChanges': [
                {
                    'range': {
                        'start': { 'line': 6, 'character': 2 },
                        'end': { 'line': 6, 'character': 2 }
                    },
                    'text': 'unction f() public {}'
                }
            ]
        })
        published_diagnostics = self.wait_for_diagnostics(solc)
        self.expect_equal(len(published_diagnostics), 1, "one publish diagnostics notification")
        report3 = published_diagnostics[0]
        self.expect_equal(report3['uri'], FILE_URI, "Correct file URI")
        self.expect_equal(len(report3['diagnostics']), 1, "one diagnostic")
        self.expect_diagnostic(report3['diagnostics'][0], 4126, 6, (1, 23))

    def test_textDocument_didChange_empty_file(self, solc: JsonRpcProcess) -> None:
        """
        Starts with an empty file and changes it to look like
        the didOpen_with_import test case. Then we can use
        the same verification calls to ensure it worked as expected.
        """
        # This FILE_NAME must be alphabetically before lib.sol to not over-complify
        # the test logic in verify_didOpen_with_import_diagnostics.
        FILE_NAME = 'a_new_file'
        FILE_URI = self.get_test_file_uri(FILE_NAME)
        self.setup_lsp(solc)
        solc.send_message('textDocument/didOpen', {
            'textDocument': {
                'uri': FILE_URI,
                'languageId': 'Solidity',
                'version': 1,
                'text': ''
            }
        })
        reports = self.wait_for_diagnostics(solc)
        self.expect_equal(len(reports), 1)
        report = reports[0]
        published_diagnostics = report['diagnostics']
        self.expect_equal(len(published_diagnostics), 2)
        self.expect_diagnostic(published_diagnostics[0], code=1878, lineNo=0, startEndColumns=(0, 0))
        self.expect_diagnostic(published_diagnostics[1], code=3420, lineNo=0, startEndColumns=(0, 0))
        solc.send_message('textDocument/didChange', {
            'textDocument': {
                'uri': self.get_test_file_uri('a_new_file')
            },
            'contentChanges': [
                {
                    'range': {
                        'start': { 'line': 0, 'character': 0 },
                        'end': { 'line': 0, 'character': 0 }
                    },
                    'text': self.get_test_file_contents('didOpen_with_import')
                }
            ]
        })
        published_diagnostics = self.wait_for_diagnostics(solc)
        self.verify_didOpen_with_import_diagnostics(published_diagnostics, 'a_new_file')

    def test_textDocument_didChange_multi_line(self, solc: JsonRpcProcess) -> None:
        """
        Starts with an empty file and changes it to multiple times, changing
        content across lines.
        """
        self.setup_lsp(solc)
        FILE_NAME = 'didChange_template'
        FILE_URI = self.get_test_file_uri(FILE_NAME)
        solc.send_message('textDocument/didOpen', {
            'textDocument': {
                'uri': FILE_URI,
                'languageId': 'Solidity',
                'version': 1,
                'text': self.get_test_file_contents(FILE_NAME)
            }
        })
        published_diagnostics = self.wait_for_diagnostics(solc)
        self.expect_equal(len(published_diagnostics), 1, "one publish diagnostics notification")
        self.expect_equal(len(published_diagnostics[0]['diagnostics']), 0, "no diagnostics")
        solc.send_message('textDocument/didChange', {
            'textDocument': { 'uri': FILE_URI },
            'contentChanges': [
                {
                    'range': {
                        'start': { 'line': 3, 'character': 3 },
                        'end': { 'line': 4, 'character': 1 }
                    },
                    'text': "tract D {\n\n  uint x\n = -1; \n "
                }
            ]
        })
        published_diagnostics = self.wait_for_diagnostics(solc)
        self.expect_equal(len(published_diagnostics), 1, "one publish diagnostics notification")
        report2 = published_diagnostics[0]
        self.expect_equal(report2['uri'], FILE_URI, "Correct file URI")
        self.expect_equal(len(report2['diagnostics']), 1, "one diagnostic")
        self.expect_diagnostic(report2['diagnostics'][0], 7407, 6, (3, 5))

        # Now we are changing the part "x\n = -" of "uint x\n = -1;"
        solc.send_message('textDocument/didChange', {
            'textDocument': { 'uri': FILE_URI },
            'contentChanges': [
                {
                    'range': {
                        'start': { 'line': 5, 'character': 7 },
                        'end': { 'line': 6, 'character': 4 }
                    },
                    'text': "y\n = [\nuint(1),\n3,4]+"
                }
            ]
        })
        published_diagnostics = self.wait_for_diagnostics(solc)
        self.expect_equal(len(published_diagnostics), 1, "one publish diagnostics notification")
        report3 = published_diagnostics[0]
        self.expect_equal(report3['uri'], FILE_URI, "Correct file URI")
        self.expect_equal(len(report3['diagnostics']), 2, "two diagnostics")
        diagnostic = report3['diagnostics'][0]
        self.expect_equal(diagnostic['code'], 2271, 'diagnostic: 2271')
        # check multi-line error code
        self.expect_equal(
            diagnostic['range'],
            {
                'end': {'character': 6, 'line': 8},
                'start': {'character': 3, 'line': 6}
            },
            "diagnostic: check range"
        )
        diagnostic = report3['diagnostics'][1]
        self.expect_equal(diagnostic['code'], 7407, 'diagnostic: 7407')
        # check multi-line error code
        self.expect_equal(
            diagnostic['range'],
            {
                'end': {'character': 6, 'line': 8},
                'start': {'character': 3, 'line': 6}
            },
            "diagnostic: check range"
        )

    # }}}
    # }}}

if __name__ == "__main__":
    # Turn off user input buffering so we get the input immediately,
    # not only after a line break
    tty.setcbreak(sys.stdin.fileno())
    suite = SolidityLSPTestSuite()
    exit_code = suite.main()
    sys.exit(exit_code)
