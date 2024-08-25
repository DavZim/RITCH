#include "filter_itch.h"

#ifdef __APPLE__
#  define fseeko64 fseeko
#  define ftello64 ftello
#endif

// [[Rcpp::export]]
void filter_itch_impl(std::string infile, std::string outfile,
                      int64_t start, int64_t end,
                      Rcpp::CharacterVector filter_msg_type,
                      Rcpp::IntegerVector filter_stock_locate,
                      Rcpp::NumericVector min_timestamp,
                      Rcpp::NumericVector max_timestamp,
                      bool append,
                      int64_t max_buffer_size,
                      bool quiet) {

  // treat filters
  std::vector<char> filter_msgs;
  std::vector<int>  filter_sloc;

  for (auto f : filter_msg_type) filter_msgs.push_back(Rcpp::as<char>(f));
  for (int s : filter_stock_locate) filter_sloc.push_back(s);

  const size_t ts_size = min_timestamp.size();
  std::vector<int64_t> min_ts(ts_size);
  if (ts_size > 0)
    std::memcpy(&(min_ts[0]), &(min_timestamp[0]), ts_size * sizeof(int64_t)); 

  std::vector<int64_t> max_ts(ts_size);
  if (ts_size > 0)
    std::memcpy(&(max_ts[0]), &(max_timestamp[0]), ts_size * sizeof(int64_t));
  if (max_ts.size() == 1 && max_ts[0] == -1)
    max_ts[0] = std::numeric_limits<int64_t>::max();

  // get the max_ts_value!
  int64_t max_ts_val = -1;
  for (auto t : max_ts) if (t > max_ts_val) max_ts_val = t;
  if (max_ts_val == -1) max_ts_val = std::numeric_limits<int64_t>::max();

  if (end < 0) end = std::numeric_limits<int64_t>::max();

  if (filter_msgs.size() == 0 &&
      filter_sloc.size() == 0 &&
      min_ts.size() == 0 &&
      max_ts.size() == 0 &&
      start == 0 &&
      end == -1)
    Rcpp::stop("No filters where set, aborting filter process!");

  // parse the messages
  // redirect to the correct msg types only
  FILE* ifile;
  ifile = fopen(infile.c_str(), "rb");
  if (ifile == NULL) {
    char buffer [50];
    snprintf(buffer, sizeof(buffer), "Input File Error number %i!", errno);
    Rcpp::stop(buffer);
  }

  FILE* ofile;
  std::string omode = append ? "ab" : "wb";
  ofile = fopen(outfile.c_str(), omode.c_str());
  if (ofile == NULL)  {
    char buffer [50];
    snprintf(buffer, sizeof(buffer), "Output File Error number %i!", errno);
    Rcpp::stop(buffer);
  }

  // get size of the file
  if (fseeko64(ifile, 0L, SEEK_END) != 0) {
    Rcpp::stop("Error seeking to end of file");
  }
  int64_t filesize = ftello64(ifile);
  if (filesize == -1) {
    Rcpp::stop("Error getting file size");
  }
  if (fseeko64(ifile, 0L, SEEK_SET) != 0) {
    Rcpp::stop("Error seeking back to start of file");
  }

  // create buffer
  int64_t buf_size = max_buffer_size > filesize ? filesize : max_buffer_size;
  unsigned char * ibuf;
  unsigned char * obuf;
  ibuf = (unsigned char*) malloc(buf_size);
  obuf = (unsigned char*) malloc(buf_size);
  // Rprintf("Allocating buffer to size %lld\n", buf_size);

  int64_t bytes_read = 0, this_buffer_size = 0, bytes_written = 0;
  int64_t msg_read = 0, msg_count = 0;
  std::vector<int64_t> msg_reads(MSG_CLASS_SIZE, 0);

  int64_t o = 0;
  int msg_size;
  bool max_ts_reached = false;

  while (bytes_read < filesize && !max_ts_reached) {
    Rcpp::checkUserInterrupt();

    // read in buffer buffers
    this_buffer_size = fread(ibuf, 1, buf_size, ifile);
    int64_t i = 0;
    msg_size = 0;

    do {
      // check early stop in max_timestamp
      const int64_t cur_ts = getNBytes64<6>(&ibuf[i + 2 + 5]);
      if (cur_ts > max_ts_val) {
        max_ts_reached = true;
        break;
      }

      const unsigned char mt = ibuf[i + 2];
      // Check Filter Messages
      bool parse_message = true;
      // only check the filter if previous tests are all OK
      if (parse_message)
        parse_message = passes_filter(&ibuf[i + 2], filter_msgs);
      if (parse_message)
        parse_message = passes_filter(&ibuf[i + 2 + 1], filter_sloc);
      if (parse_message)
        parse_message = passes_filter_in(&ibuf[i + 2 + 5], min_ts, max_ts);
      // use TYPE_CLASS_TRANSLATOR as we count per message class not per msg_type!
      if (parse_message) {
        // count here the msg_reads to make sure that the count is within the
        // other filters
        parse_message = msg_reads[TYPE_CLASS_TRANSLATOR[mt - 'A']] >= start &&
          msg_reads[TYPE_CLASS_TRANSLATOR[mt - 'A']] <= end;
        msg_reads[TYPE_CLASS_TRANSLATOR[mt - 'A']]++;
      }

      msg_size = get_message_size(mt);

      if (o + msg_size > buf_size) {
        // write to buffer until o
        // Rprintf("New obuf, write  %9lld bytes to ofile next msg %i\n",
        //         o, msg_size);
        fwrite(obuf, sizeof(unsigned char), o, ofile);
        // reset obuf
        std::memset(obuf, 0x00, buf_size);

        bytes_written += o;
        o = 0;
      }

      if (parse_message) {
        // Rprintf("Filter ibuf at %lld copy into obuf at %lld\n",
        // i, o);
        msg_read++;
        // Rprintf("Copying '%i' from ibuf at %lld to obuf at %lld\n",
        //         msg_size, i, o);
        std::memcpy(&(obuf[o]), &(ibuf[i]), msg_size);
        o += msg_size;
        // msg_reads[TYPE_CLASS_TRANSLATOR[mt - 'A']]++;
      }

      msg_count++;
      i += msg_size;
      // 50 = max msg_size
    } while (i + 50 <= this_buffer_size && bytes_read + i <= filesize);

    // offset file pointer to fit the next message into the buffer
    const int64_t offset = i - this_buffer_size;
    // Rprintf("Filter ibuf at %6lld offsetting by %3lld - Total bytes read %lld\n",
    //         i, offset, bytes_read + i);
    fseeko64(ifile, offset, SEEK_CUR);
    bytes_read += i;
  }

  if (o > 0) {
    // write to buffer until o
    // Rprintf("Last obuf, write %9lld bytes to ofile\n", o);
    fwrite(obuf, sizeof(unsigned char), o, ofile);
  }

  if (!quiet) {
    Rprintf("[Bytes]      scanned %lld, filtered %lld\n",
            (long long int) filesize, (long long int) bytes_written + o);
    Rprintf("[Messages]   scanned %lld, filtered %lld\n",
            (long long int) msg_count, (long long int) msg_read);
  }

  free(ibuf);
  fclose(ifile);

  free(obuf);
  fclose(ofile);
}
