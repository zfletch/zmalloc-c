#!/bin/bash

make test

valgrind --quiet --leak-check=yes ./test

make clean
