#ifndef FILTERITCH_H
#define FILTERITCH_H

#include <Rcpp.h>
#include "specifications.h"
#include "helper_functions.h"

void filter_itch_impl(std::string infile, std::string outfile,
                      int64_t start, int64_t end,
                      Rcpp::CharacterVector filter_msg_type,
                      Rcpp::IntegerVector filter_stock_locate,
                      Rcpp::NumericVector min_timestamp,
                      Rcpp::NumericVector max_timestamp,
                      bool append = false,
                      int64_t max_buffer_size = 1e8,
                      bool quiet = false);

#endif // FILTERITCH_H