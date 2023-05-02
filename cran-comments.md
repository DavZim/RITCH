
Fixed memory leaks in C++ Code (valgrind reports no "definitely or indirectly lost" memory leaks) therefore the segfault on Debian should be solved as well.
Additionally, no errors are found using wch/r-debug RDsan and RDcsan.
