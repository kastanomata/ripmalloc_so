#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <libgen.h>  // For dirname()/basename()
#include <limits.h>  // For PATH_MAX
#include <string.h>
#include <dirent.h>

#include <helpers/parse.h>

#define PROJECT_FOLDER "."
#define BENCHMARK_FOLDER "./benchmarks"


int benchmark();