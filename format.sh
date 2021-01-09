#!/bin/bash
find samal_cli samal_lib peg_parser tests -name *.hpp -o -name *.cpp |xargs clang-format -i -style=file
