#ifndef SPECIFICATIONS_H
#define SPECIFICATIONS_H

#include <string>
#include <vector>

/**
 * A Namespace containint the important (constant) specifications
 * of the different messagetypes
 */
namespace ITCH {
  // all messages in order
  const std::vector<unsigned char> TYPES = {'S','R','H','Y','L','V','W','K','J',
                                            'A','F','E', 'C','X','D','U','P','Q',
                                            'B','I','N'};
   // the size in bytes of each message
  namespace SIZE {
    const unsigned long long S = 12;
    const unsigned long long R = 39;
    const unsigned long long H = 25;
    const unsigned long long Y = 20;
    const unsigned long long L = 26;
    const unsigned long long V = 35;
    const unsigned long long W = 12;
    const unsigned long long K = 28;
    const unsigned long long J = 35;
    const unsigned long long A = 36;
    const unsigned long long F = 40;
    const unsigned long long E = 31;
    const unsigned long long C = 36;
    const unsigned long long X = 23;
    const unsigned long long D = 19;
    const unsigned long long U = 35;
    const unsigned long long P = 44;
    const unsigned long long Q = 40;
    const unsigned long long B = 19;
    const unsigned long long I = 50;
    const unsigned long long N = 20;
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
    const int A = 9;
    const int F = 10;
    const int E = 11;
    const int C = 12;
    const int X = 13;
    const int D = 14;
    const int U = 15;
    const int P = 16;
    const int Q = 17;
    const int B = 18;
    const int I = 19;
    const int N = 20;
  }
  // all messages in a string, to make conversions easier
  const std::vector<std::string> TYPESSTRING = {"S","R","H","Y","L","V","W","K","J",
                                                "A","F","E", "C","X","D","U","P","Q",
                                                "B","I","N"};
}

#endif //SPECIFICATIONS_H
