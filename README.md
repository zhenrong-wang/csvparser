# CSVParser: Parsing CSV Files and Get Lines with A Specified Keyword

# 1. Background

This code is inspired by Prof. Lemire. The Csharp version has been implemented and benchmarked. Check the [link](https://github.com/lemire/CsharpCVSBench) for his repository.

# 2. Brief Intro

**Program Name**: CSV Parser

**Purpose**: Given a CSV data file, Parse it and get the lines with specified keyword, e.g. ",Harvard".

**License**: MIT

# 3. How-To: Build, Run, and Use

## 3.1 Build

### 3.1.1 Prerequisites

You need a C compiler to build. 

- For GNU/Linux Distro or other *nix users, the [GNU Compiler Collections](https://gcc.gnu.org/), known as gcc, is recommended.
- For macOS users, [clang](https://clang.llvm.org/) is easy to install and use (brew is not needed to install clang on macOS).
- Currently this code doesn't work on Windows because it used Linux mmap() API. 

### 3.1.2 Build Guide

1. Use git to clone this code: `git clone https://github.com/zhenrong-wang/csvparser`
2. Build command example: `gcc csv_parser.c -Ofast -Wall -march=native -o parser`

## 3.2 Run

Just run `./parser`, it will parse the example CSV file under `./data/` and print out results.

# 4 Bugs and Communications

The code might be buggy, please feel free to report.
