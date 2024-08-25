# RITCH 0.1.76

* fix bug where no messages would be reported for larger files

# RITCH 0.1.26

* fix bug where gz functionality would write to user library or current directory

# RITCH 0.1.25

* fix Debian segfault when writing to user library

# RITCH 0.1.24

* fix printf warnings about wrong argument type

# RITCH 0.1.23

* fix compilation warning and limit test cases to two cores (CRAN...)

# RITCH 0.1.22

* fix CRAN release by shorten example runtimes

# RITCH 0.1.21

* fix long running tasks in read functions

# RITCH 0.1.20

* fix bug where tests would fail on some platforms where files are written and not cleaned up
* CRAN release

# RITCH 0.1.19

* fix bug in tests on some platforms
* CRAN release

# RITCH 0.1.18

* CRAN release


# RITCH 0.1.11

* update internal C++ structure, reducing code complexity, increasing read speeds, reducing size of package
* add `filter_itch(infile, outfile, ...)` to filter directly to files


# RITCH 0.1.10

* add `write_itch()` to write ITCH files
* add filters to `read_*` functions
* add read functions for all classes
