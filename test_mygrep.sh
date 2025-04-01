#!/bin/bash
echo "Test 1: Search for test in test.txt"
./mygrep test.txt test
echo "Test 2: Search for notfound"
./mygrep test.txt notfound
echo "Test 3: Search for line"
./mygrep test.txt line
