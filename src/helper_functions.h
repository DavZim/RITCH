#ifndef HELPERFUNCTIONS_H
#define HELPERFUNCTIONS_H

#include <Rcpp.h>
#include "specifications.h"

// get the message size for a char
int get_message_size(const unsigned char msg);
// converts from the long form (MSG_NAMES) to the shorter used form (ACT_MST_NAMES)
std::vector<int64_t> take_needed_messages(std::vector<int64_t> &v);
// formats a number with thousands separator
std::string format_thousands(int64_t num,
                             const std::string sep = ",",
                             const std::string s = "");

// get bytes functions

// Converts n bytes from a buffer in big endian to an int32_t
template<size_t size> int32_t getNBytes32(unsigned char* buff) {
    int32_t r = 0;
    for (size_t i = 0; i < size; ++i) {
        r = (r << 8) + *buff++;
        // Rprintf("i %2i, r: %15i (0x%llx), next 6: %02x %02x %02x %02x %02x %02x\n",
        //         i, r, (long long) r, buff[0], buff[1], buff[2], buff[3], buff[4], buff[5]);
    }
    return r;
}
// Converts n bytes from a buffer in big endian to an int64_t
template<size_t size> int64_t getNBytes64(unsigned char* buff) {
    int64_t r = 0;
    for (size_t i = 0; i < size; ++i) {
        r = (r << 8) + *buff++;
        // Rprintf("i %2i, r: %15i (0x%llx), next 6: %02x %02x %02x %02x %02x %02x\n",
        //         i, r, (long long) r, buff[0], buff[1], buff[2], buff[3], buff[4], buff[5]);
    }
    return r;
}

std::string getNBytes(unsigned char* buf, const int n = 8, const unsigned char empty = ' ');

// converts a numeric vector to integer64
Rcpp::NumericVector to_int64(Rcpp::NumericVector v);

// function that checks if a buffer passes a filter
bool passes_filter(unsigned char* buf, std::vector<char> &filter);
bool passes_filter(unsigned char* buf, std::vector<int> &filter);
bool passes_filter_in(unsigned char* buf, std::vector<int64_t> &lower,
                      std::vector<int64_t> &upper);

// set functions, set X bytes in a buffer
uint64_t set2bytes(unsigned char* b, int32_t val);
uint64_t set4bytes(unsigned char* b, int32_t val);
uint64_t set6bytes(unsigned char* b, int64_t val);
uint64_t set8bytes(unsigned char* b, int64_t val);
uint64_t setCharBytes(unsigned char* b, std::string x, uint64_t n);

#endif //HELPERFUNCTIONS_H
