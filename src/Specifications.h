#ifndef SPECIFICATIONS_H
#define SPECIFICATIONS_H

#include <string>
#include <vector>

// Define NA_INT64
const int64_t NA_INT64 = 1ULL << 63;

/**
 * A Namespace containint the important (constant) specifications
 * of the different messagetypes
 */
namespace ITCH {
  // all messages in order
  const std::vector<unsigned char> TYPES = {'S','R','H','Y','L','V','W','K','J',
                                            'h','A','F','E', 'C','X','D','U','P',
                                            'Q','B','I','N'};
   // the size in bytes of each message
  namespace SIZE {
    const int64_t S = 12;
    const int64_t R = 39;
    const int64_t H = 25;
    const int64_t Y = 20;
    const int64_t L = 26;
    const int64_t V = 35;
    const int64_t W = 12;
    const int64_t K = 28;
    const int64_t J = 35;
    const int64_t h = 21;
    const int64_t A = 36;
    const int64_t F = 40;
    const int64_t E = 31;
    const int64_t C = 36;
    const int64_t X = 23;
    const int64_t D = 19;
    const int64_t U = 35;
    const int64_t P = 44;
    const int64_t Q = 40;
    const int64_t B = 19;
    const int64_t I = 50;
    const int64_t N = 20;
  }

  // the position (for example in the count) of each message
  namespace POS {
    const int S = 0;
    const int R = 1;
    const int H = 2;
    const int Y = 3;
    const int L = 4;
    const int V = 5;
    const int W = 6;
    const int K = 7;
    const int J = 8;
    const int h = 9;
    const int A = 10;
    const int F = 11;
    const int E = 12;
    const int C = 13;
    const int X = 14;
    const int D = 15;
    const int U = 16;
    const int P = 17;
    const int Q = 18;
    const int B = 19;
    const int I = 20;
    const int N = 21;
  }
  // all messages in a string, to make conversions easier
  const std::vector<std::string> TYPESSTRING = {"S","R","H","Y","L","V","W","K",
                                                "J","h","A","F","E", "C","X","D",
                                                "U","P","Q","B","I","N"};
}

#endif //SPECIFICATIONS_H
