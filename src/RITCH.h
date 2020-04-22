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
int64_t getMessageLength(unsigned char msgType);
int getMessagePosition(unsigned char msgType);

// loads a plain-text file into the messagetype
void loadToMessages(std::string filename, 
                    MessageType& msg,
                    int64_t startMsgCount = 0,
                    int64_t endMsgCount = std::numeric_limits<int64_t>::max(),
                    int64_t bufferSize = 1e8,
                    bool quiet = false);

// formats a number with thousands seperators
std::string formatThousands(int64_t num, 
                            const std::string sep = ",", 
                            std::string s = "");
#endif //RITCH_H