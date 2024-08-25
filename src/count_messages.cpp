#include "count_messages.h"

#ifdef __APPLE__
#  define fseeko64 fseeko
#  define ftello64 ftello
#endif

// counts messages in a file
std::vector<int64_t> count_messages_internal(std::string filename,
                                             int64_t max_buffer_size) {
  FILE* infile;
  infile = fopen(filename.c_str(), "rb");
  if (infile == NULL) {
    char buffer [50];
    snprintf(buffer, sizeof(buffer), "File Error number %i!", errno);
    Rcpp::stop(buffer);
  }

  // get size of the file
  if (fseeko64(infile, 0L, SEEK_END) != 0) {
    Rcpp::stop("Error seeking to end of file");
  }
  int64_t filesize = ftello64(infile);
  if (filesize == -1) {
    Rcpp::stop("Error getting file size");
  }
  if (fseeko64(infile, 0L, SEEK_SET) != 0) {
    Rcpp::stop("Error seeking back to start of file");
  }

  // create buffer
  int64_t buf_size = max_buffer_size > filesize ? filesize : max_buffer_size;
  unsigned char * buf;
  buf = (unsigned char*) malloc(buf_size);

  int64_t bytes_read = 0, this_buffer_size = 0;
  std::vector<int64_t> count(sizeof(MSG_SIZES)/sizeof(MSG_SIZES[0]));

  while (bytes_read < filesize) {
    Rcpp::checkUserInterrupt();

    // read in buffer buffers
    this_buffer_size = fread(buf, 1, buf_size, infile);
    int64_t i = 0;

    int msg_size = 0;
    do {
      msg_size = get_message_size(buf[i + 2]);

      count[buf[i + 2] - 'A']++;
      i += msg_size;

    } while (i + msg_size <= this_buffer_size && bytes_read + i <= filesize);

    // align the file pointer to read in a full message again
    const int64_t offset = i - this_buffer_size;
    fseeko64(infile, offset, SEEK_CUR);
    bytes_read += i;
  }

  free(buf);
  fclose(infile);
  return count;
}

// [[Rcpp::export]]
Rcpp::DataFrame count_messages_impl(std::string filename,
                                    int64_t max_buffer_size,
                                    bool quiet) {

  std::vector<int64_t> ct_raw = count_messages_internal(filename, max_buffer_size);
  std::vector<int64_t> count = take_needed_messages(ct_raw);

  int64_t total_msgs = 0;
  for (int64_t v : count) total_msgs += v;

  if (!quiet) Rprintf("[Counting]   %s total messages found\n",
      format_thousands(total_msgs).c_str());

  if (!quiet) Rprintf("[Converting] to data.table\n");

  Rcpp::CharacterVector names;
  for (const unsigned char c : ACT_MSG_NAMES) names.push_back(std::string(1, c));

  Rcpp::NumericVector ct(N_ACT_MSGS);
  ct.attr("class") = "integer64";
  std::memcpy(&(ct[0]), &(count[0]), N_ACT_MSGS * sizeof(double));

  Rcpp::List res = Rcpp::List::create(
    Rcpp::Named("msg_type") = names,
    Rcpp::Named("count") = ct
  );

  res.attr("class") = Rcpp::CharacterVector::create("data.table", "data.frame");
  return res;
}
