#include "RITCH.h"
#include "countMessages.h"


// @brief      Counts the number of each message type per file
//
// @param[in]  filename    The filename, either of a plain-text file or of a .gz-file
// @param[in]  bufferSize  The buffer size in bytes, defaults to 100MB
//
// @return     An Rcpp::DataFrame containing the message type and the count
//
// [[Rcpp::export]]
Rcpp::DataFrame getMessageCountDF(std::string filename,
                                  int64_t bufferSize,
                                  bool quiet = false) {
  
  std::vector<int64_t> count;
  

  if (!quiet) Rcpp::Rcout << "[Counting]   ";
  count = countMessages(filename, bufferSize);
  int64_t nMessages = 0;
  for (int64_t i : count) {
  	nMessages += i;
  }
  if (!quiet) Rcpp::Rcout << formatThousands(nMessages) << " messages found\n";
  if (!quiet) Rcpp::Rcout << "[Converting] to data.table\n";

  Rcpp::StringVector types(ITCH::TYPES.size());
  types = ITCH::TYPESSTRING;
  
  Rcpp::DataFrame df = Rcpp::DataFrame::create(Rcpp::Named("msg_type") = types,
                                               Rcpp::Named("count") = count);
  
  return df;
}
