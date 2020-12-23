#include <Rcpp.h>

// based on ASCII Tables. Use msg_char - 'A' for its position...
const int MSG_SIZES [] = {
// A   B   C   D   E   F  G   H   I   J   K   L  M   N  O   P   Q   R   S  T
  36, 19, 36, 19, 31, 40, 0, 25, 50, 35, 28, 26, 0, 20, 0, 44, 40, 39, 12, 0, 
// U   V   W   X   Y  Z  [  \  ]  ^  _  `  a  b  c  d  e  f  g   h 
  35, 35, 12, 23, 20, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 21, 
};


inline int get_message_size(const char msg) {
  return MSG_SIZES[msg - 'A'];
}


// [[Rcpp::export]]
Rcpp::NumericVector count_messages_impl(std::string filename,
                                        int64_t max_buffer_size = 1e8,
                                        bool quiet = false) {
  
  FILE* infile;
  infile = fopen(filename.c_str(), "rb");
  if (infile == NULL) Rcpp::stop("File Error!\n");
  
  // get size of the file
  fseek(infile, 0L, SEEK_END); 
  int64_t filesize = ftell(infile);
  fseek(infile, 0L, SEEK_SET);
  // Rprintf("Filesize       %" PRId64 "\n", filesize);
  
  // create buffer
  int64_t buf_size = max_buffer_size > filesize ? filesize : max_buffer_size;
  char * buf;
  buf = (char*) malloc(buf_size);
  // Rprintf("Buffer of Size %" PRId64 "\n", buf_size);
  
  int64_t bytes_read = 0, this_buffer_size = 0;
  std::vector<int64_t> count(sizeof(MSG_SIZES)/sizeof(MSG_SIZES[0]));
  
  while (bytes_read < filesize) {
    Rcpp::checkUserInterrupt();
    // Rprintf(".");
    
    // read in buffer buffers
    this_buffer_size = fread(buf, 1, buf_size, infile);
    // Rprintf("- New Buffer: %7zu - %7zu, buf size %7" PRId64 " | ",
            // ftell(infile) - this_buffer_size, ftell(infile), this_buffer_size);
    
    int64_t i = 0;
    while (i < this_buffer_size) {
      
      const int msg_size = get_message_size(buf[i + 2]);
      
      if (i + msg_size < this_buffer_size) {

        count[buf[i + 2] - 'A']++;
        i += msg_size + 2;

      } else {
        // Rprintf(" -repositioning ");
        // end of buffer, reposition the filepointer...
        fseek(infile, -(this_buffer_size - i), SEEK_CUR);
        break;
      }
    }
    // Rprintf("<EOB>, parsed %" PRId64 "\n", i);
    
    bytes_read += i;
  }
  int64_t total_msgs = 0;
  for (int64_t v : count) total_msgs += v;
  
  if (!quiet) Rprintf("[Counting]   %" PRId64 " messages found\n", total_msgs);
  
  free(buf);
  fclose(infile);
  
  if (!quiet) Rprintf("[Converting] to data.table\n");
  
  Rcpp::NumericVector res(count.size());
  std::memcpy(&(res[0]), &(count[0]), count.size() * sizeof(int64_t));
  
  res.attr("class") = "integer64";
  Rcpp::CharacterVector names(count.size());
  for (int i = 0; i < count.size(); i++) {
    std::string s;
    s += 'A' + i;
    names(i) = s;
  }
  res.names() = names;
  return res;
}

/***R
count_messages <- function(file, buffer_size = 1e8, quiet = FALSE) {
  t0 <- Sys.time()
  ct <- count_messages_impl(file, buffer_size, quiet)
  sel <- strsplit("SRHYLVWKJAFECXDUPQBIN", "")[[1]]
  ct <- ct[sel]
  names(ct) <- NULL
  res <- data.table::data.table(
    msg_type = sel,
    count = ct
  )
  if (!quiet) 
    cat(sprintf("[Done]       in %.2f secs\n",
                difftime(Sys.time(), t0, units = "secs")))
  return(res)
}

*/
/*


// one read one write class
class MessageParser{
public:
  
  // set vectors
  hold all vectors but only access those of the current message...
  
  void resize_vectors(int64_t n);
  
  //
};

*/

// show_diff(c(LETTERS, letters[1:8]))
// [[Rcpp::export]]
void show_diff(Rcpp::CharacterVector x) {
  for (int i = 0; i < x.size(); i++) {
    const char cc = Rcpp::as<char>(x[i]);
    const int idx = get_message_size(cc);
    Rprintf("%c = %i\n", cc, idx);
  }
}