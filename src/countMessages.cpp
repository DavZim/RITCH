#include "countMessages.h"

/*
 * @brief      Counts the number of messages from a plain-text file
 *
 * @param[in]  filename    The filename to the plain-text file
 * @param[in]  bufferSize  The buffer size in bytes, defaults to 100MB
 *
 * @return     A vector containing the number of messages per type
 */
std::vector<int64_t> countMessages(std::string filename, 
                                   int64_t bufferSize) {
  
  // Open the file
  FILE* infile;
  infile = fopen(filename.c_str(), "rb");
  if (infile == NULL) {
    Rcpp::stop("File Error!\n");
  }
  
  std::vector<int64_t> count(ITCH::TYPES.size(), 0);
  
  unsigned char* bufferPtr;
  int64_t bufferCharSize = sizeof(char) * bufferSize;
  bufferPtr = (unsigned char*) malloc(bufferCharSize);
  
  int64_t thisBufferSize = 0;
  int64_t curFilePtr;
  
  // fill the buffer
  while ((thisBufferSize = fread(bufferPtr, 1, bufferCharSize, infile)) > 0) {
    Rcpp::checkUserInterrupt();
    
    // find the byte number, where file-pointer currently points to 
    curFilePtr = ftell(infile);
    
    // use the current buffer to read in the messages
    int64_t inBufferIdx = 2;
    int64_t thisMsgLength;
    
    // loop through the buffer by the index inBufferIdx
    while (1) {
      // if there is no partial message, this will be triggered
      if (inBufferIdx >= thisBufferSize) break;
      
      thisMsgLength = getMessageLength(bufferPtr[inBufferIdx]);
      // if there is a partial message, go to the next buffer (gz-file pointer will be reset to match this!)
      if (inBufferIdx > thisBufferSize - thisMsgLength) break;
      
      // count the messages
      countMessageByType(count, bufferPtr[inBufferIdx]);
      
      // increase the index in the buffer
      inBufferIdx += thisMsgLength; 
      // two empty strings after each message...
      inBufferIdx += 2;
    }
    
    // if the message doesn't fit, a new buffer will not solve the issue, as there is too little information for the given msg-length
    if (inBufferIdx == 0) break;
    
    // reset the current fileptr if needed
    // clear the remaining bits, the last partial message might be cut in two, thus reset the gz-file pointer
    // remaining bits of the last message: curFilePtr - thisBufferSize + inBufferIdx - 2
    if (thisBufferSize != inBufferIdx) {
      fseek(infile, curFilePtr - thisBufferSize + inBufferIdx - 2, SEEK_SET);
    } else {
      // if the message perfectly ends, still set it back by 2, otherwise 2 bytes are skipped!
      fseek(infile, curFilePtr - 2, SEEK_SET);
    }
  }
  
  free(bufferPtr);
  fclose(infile);
  
  return count;
}

/**
 * @brief      Counts the number of message by a given type (char) into a given vector
 *
 * @param      count  The vector which holds the counts for each message type
 * @param[in]  msg    The message-type given by a character
 */
void countMessageByType(std::vector<int64_t>& count, unsigned char msg) {
  switch(msg) {
    case 'S': 
      count[ITCH::POS::S]++;
      break;
    case 'R':
      count[ITCH::POS::R]++;
      break;
    case 'H':
      count[ITCH::POS::H]++;
      break;
    case 'Y':
      count[ITCH::POS::Y]++;
      break;
    case 'L':
      count[ITCH::POS::L]++;
      break;
    case 'V':
      count[ITCH::POS::V]++;
      break;
    case 'W':
      count[ITCH::POS::W]++;
      break;
    case 'K':
      count[ITCH::POS::K]++;
      break;
    case 'J':
      count[ITCH::POS::J]++;
      break;
    case 'h':
      count[ITCH::POS::h]++;
      break;
    case 'A':
      count[ITCH::POS::A]++;
      break;
    case 'F':
      count[ITCH::POS::F]++;
      break;
    case 'E':
      count[ITCH::POS::E]++;
      break;
    case 'C':
      count[ITCH::POS::C]++;
      break;
    case 'X':
      count[ITCH::POS::X]++;
      break;
    case 'D':
      count[ITCH::POS::D]++;
      break;
    case 'U':
      count[ITCH::POS::U]++;
      break;
    case 'P':
      count[ITCH::POS::P]++;
      break;
    case 'Q':
      count[ITCH::POS::Q]++;
      break;
    case 'B':
      count[ITCH::POS::B]++;
      break;
    case 'I':
      count[ITCH::POS::I]++;
      break;
    case 'N':
      count[ITCH::POS::N]++;
      break;
    default:
      Rcpp::Rcout << "Unknown Case!\n";
    break;
  }
}
