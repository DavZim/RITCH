#ifndef HELPERFUNCTIONS_H
#define HELPERFUNCTIONS_H

#include <Rcpp.h>
#include "specifications.h"

// get the message size for a char
int get_message_size(const char msg);
// converts from the long form (MSG_NAMES) to the shorter used form (ACT_MST_NAMES)
std::vector<int64_t> take_needed_messages(std::vector<int64_t> &v);
// formats a number with thousands separator
std::string format_thousands(int64_t num,
                             const std::string sep = ",",
                             const std::string s = "");

// get functions
int32_t get2bytes(char* buf);
int32_t get4bytes(char* buf);
int64_t get6bytes(char* buf);
int64_t get8bytes(char* buf);
std::string getNBytes(char* buf, const int n = 8, const char empty = ' ');

// converts a numeric vector to integer64
Rcpp::NumericVector to_int64(Rcpp::NumericVector v);

// function that checks if a buffer passes a filter
bool passes_filter(char* buf, std::vector<char> &filter);
bool passes_filter(char* buf, std::vector<int> &filter);
bool passes_filter_in(char* buf, std::vector<int64_t> &lower,
                      std::vector<int64_t> &upper);

// set functions, set X bytes in a buffer
uint64_t set2bytes(char* b, int32_t val);
uint64_t set4bytes(char* b, int32_t val);
uint64_t set6bytes(char* b, int64_t val);
uint64_t set8bytes(char* b, int64_t val);
uint64_t setCharBytes(char* b, std::string x, uint64_t n);


#endif //HELPERFUNCTIONS_H