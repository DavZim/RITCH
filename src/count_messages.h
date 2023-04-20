#ifndef COUNTMESSAGES_H
#define COUNTMESSAGES_H

#include <Rcpp.h>
#include "specifications.h"
#include "helper_functions.h"

// internal main worker function that counts the messages
std::vector<int64_t> count_messages_internal(std::string filename,
                                             int64_t max_buffer_size);

// Entry function for returning the count data.frame
Rcpp::DataFrame count_messages_impl(std::string filename,
                                    int64_t max_buffer_size = 1e8,
                                    bool quiet = false);

#endif // COUNTMESSAGES_H