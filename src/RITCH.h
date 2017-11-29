#ifndef RITCH_H
#define RITCH_H

#include <Rcpp.h>
#include <string>
#include <vector>
#include <cstdint>
#include <limits>

// User Includes
#include "MessageTypes.h"
#include "Specifications.h"
// [[Rcpp::plugins("cpp11")]]

/**
 * ############################################################
 * getMessageLength fetches the lengths for a given message
 * loadPlain load the contents of a file (plain-text or .gz)
 *  into the MessageType or its children (see MessageTypes.h)
 * #############################################################
 */

/// Returns the message lengths for a given character
unsigned long long getMessageLength(unsigned char msgType);
int getMessagePosition(unsigned char msgType);

// loads a plain-text file into the messagetype
void loadToMessages(std::string filename, 
                    MessageType& msg,
                    unsigned long long startMsgCount = 0,
                    unsigned long long endMsgCount = std::numeric_limits<unsigned long long>::max(),
                    unsigned long long bufferSize = 1e8,
                    bool quiet = false);

#endif //RITCH_H