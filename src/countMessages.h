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
std::vector<unsigned long long> countMessages(std::string filename, 
                                              unsigned long long bufferSize = 1e8);

void countMessageByType(std::vector<unsigned long long>& count, unsigned char msg);

#endif // COUNTMESSAGES_H