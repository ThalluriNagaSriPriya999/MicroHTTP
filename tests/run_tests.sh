#!/bin/bash

echo "========================================"
echo "       MicroHTTP Automated Tests"
echo "========================================"
echo

echo "Running HTTP Parser Tests..."
echo "----------------------------------------"
./tests/test_http_parser

PARSER_RESULT=$?

echo

echo "Running Security Tests..."
echo "----------------------------------------"
./tests/test_security

SECURITY_RESULT=$?

echo
echo "========================================"
echo "           TEST SUMMARY"
echo "========================================"

if [ $PARSER_RESULT -eq 0 ]; then
    echo "HTTP Parser Tests : PASS"
else
    echo "HTTP Parser Tests : FAIL"
fi

if [ $SECURITY_RESULT -eq 0 ]; then
    echo "Security Tests    : PASS"
else
    echo "Security Tests    : FAIL"
fi

echo "========================================"

if [ $PARSER_RESULT -eq 0 ] && [ $SECURITY_RESULT -eq 0 ]; then
    echo "Overall Result    : ALL TESTS PASSED"
    exit 0
else
    echo "Overall Result    : TEST FAILURE"
    exit 1
fi
