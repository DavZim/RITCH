#ifndef COUNTMESSAGES_H
#define COUNTMESSAGES_H

#include "RITCH.h"

/**
 * #######################################################################################
 * countMessages load the contents of the 
 * file int a Rcpp::DataFrame
 * 
 * countMessageByType is an internal function that does the actual counting
 * #######################################################################################
 */ 
std::vector<int64_t> countMessages(std::string filename, 
                                   int64_t bufferSize = 1e8);

void countMessageByType(std::vector<int64_t>& count, unsigned char msg);

#endif // COUNTMESSAGES_H