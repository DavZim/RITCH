#include <Rcpp.h>

// the lengths of the message types ordered based on their ASCII table positions
// To get the respective positions of a message 'msg' (e.g., 'Q') use MSG_SIZES[msg - 'A'];
const int MSG_SIZES [] = {
// A   B   C   D   E   F  G   H   I   J   K   L  M   N  O   P   Q   R   S  T
  36, 19, 36, 19, 31, 40, 0, 25, 50, 35, 28, 26, 0, 20, 0, 44, 40, 39, 12, 0, 
// U   V   W   X   Y  Z  [  \  ]  ^  _  `  a  b  c  d  e  f  g   h 
  35, 35, 12, 23, 20, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 21
};

// the names of the message types
const char MSG_NAMES [] = {
  'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 
  'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', '[', '\\', ']', '^', 
  '_', '`', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h'
};

// the number of message types in MSG_SIZES, MSG_NAMES, value is 40...
const int N_TYPES = sizeof(MSG_SIZES) / sizeof(MSG_SIZES[0]);

// The message classes (or groups)
const std::vector<std::string> MSG_CLASSES {
  "system_events",
  "stock_directory",
  "trading_status",
  "reg_sho",
  "market_participant_states",
  "mwcb",
  "ipo",
  "luld",
  "orders",
  "modifications",
  "trades",
  "noii",
  "rpii"
};

// How many classes there are
const int MSG_CLASS_SIZE = MSG_CLASSES.size();
  
// a map which links the message classes to the message types
// const std::map<std::string, std::vector<char>> CLASS_TYPE_MAP {
//   {"system_events", {'S'}},
//   {"stock_directory", {'R'}},
//   {"trading_status", {'H', 'h'}},
//   {"reg_sho", {'Y'}},
//   {"market_participant_states", {'L'}},
//   {"mwcb", {'V', 'W'}},
//   {"ipo", {'K'}},
//   {"luld", {'J'}},
//   {"orders", {'A', 'F'}},
//   {"modifications", {'E', 'C', 'X', 'D', 'U'}},
//   {"trades", {'P', 'Q', 'B'}},
//   {"noii", {'I'}},
//   {"rpii", {'N'}}
// };



inline int get_message_size(const char msg) {
  return MSG_SIZES[msg - 'A'] + 2;
}
/*
 * @brief      Formats an integer number to a std::string with thousands separator
 *
 * @param      num    The number to format
 * @param      sep    The thousands separator, default value is a comma  
 * @param      s      The return string, this is only used internally, as the function
 *                    is called recursively
 *                    
 * @return       The number as a string
 */
std::string formatThousands(int64_t num, 
                            const std::string sep = ",", 
                            std::string s = "") {
  if (num < 1000) {
    return std::to_string(num) + s;
  } else {
    std::string last_three = std::to_string(num % 1000);
    const int num_zeros = 3 - last_three.length();
    last_three = std::string(num_zeros, '0').append(last_three);
    
    const int64_t remainder = (int64_t) num / 1000;
    std::string res = sep + last_three + s;
    return formatThousands(remainder, sep, res);
  }
}
/**
 * @brief      Converts 2 bytes from a buffer in big endian to an int32_t
 * 
 * @param      buf   The buffer as a pointer to an array of chars
 *
 * @return     The converted integer
 */
int32_t get2bytes(char* buf) {
  return __builtin_bswap16(*reinterpret_cast<uint16_t*>(&buf[0]));
}

/**
 * @brief      Converts 4 bytes from a buffer in big endian to an int32_t
 * 
 * @param      buf   The buffer as a pointer to an array of chars
 *
 * @return     The converted integer
 */
int32_t get4bytes(char* buf) {
  return __builtin_bswap32(*reinterpret_cast<uint32_t*>(&buf[0]));
}

/**
 * @brief      Converts 6 bytes from a buffer in big endian to an int64_t
 * 
 * @param      buf   The buffer as a pointer to an array of chars
 *
 * @return     The converted int64_t
 */
int64_t get6bytes(char* buf) {
  return (__builtin_bswap64(*reinterpret_cast<uint64_t*>(&buf[0])) & 0xFFFFFFFFFFFF0000) >> 16;
}

/**
 * @brief      Converts 8 bytes from a buffer in big endian to an int64_t
 * 
 * @param      buf   The buffer as a pointer to an array of chars
 *
 * @return     The converted int64_t
 */
int64_t get8bytes(char* buf) {
  return __builtin_bswap64(*reinterpret_cast<uint64_t*>(&buf[0]));
}

// return N bytes of a buffer as a string
std::string getNBytes(char* buf, const int n = 8, const char empty = ' ') {
  std::string res;
  for (int i = 0; i < n; ++i) if (buf[i] != empty) res += buf[i];
  return res;
}

// replaces "" strings with NA_STRING in a Character Vector
inline void fix_NA_string(Rcpp::CharacterVector v) {
  for (auto it = v.begin(); it != v.end(); it++) if (*it == "") *it = NA_STRING;
}

// copies the 64bit values to a numeric vector and sets its class to integer64
inline Rcpp::NumericVector copy_int64_vec(std::vector<int64_t> &vec) {
  Rcpp::NumericVector res(vec.size());
  std::memcpy(&(res[0]), &(vec[0]), vec.size() * sizeof(double));
  res.attr("class") = "integer64";
  return res;
}

// one read one write class
class MessageParser{
public:
  MessageParser(std::string type) {
    std::vector<std::string> base_colnames = {
      "msg_type", "stock_locate", "tracking_number", "timestamp"
    };
    
    this->type = type;
    
    if (type == "system_events") {
      msg_types = {'S'};
      colnames = {"event_code"};
    } else if (type == "stock_directory") {
      msg_types = {'R'};
      colnames = {""};
    } else if (type == "trading_status") {
      msg_types = {'H', 'h'};
      colnames = {""};
    } else if (type == "reg_sho") {
      msg_types = {'Y'};
      colnames = {""};
    } else if (type == "market_participant_states") {
      msg_types = {'L'};
      colnames = {""};
    } else if (type == "mwcb") {
      msg_types = {'V', 'W'};
      colnames = {""};
    } else if (type == "ipo") {
      msg_types = {'K'};
      colnames = {""};
    } else if (type == "luld") {
      msg_types = {'J'};
      colnames = {""};
    } else if (type == "orders") {
      msg_types = {'A', 'F'};
      colnames = {"order_ref", "buy", "shares", "stock", "price", "mpid"};
    } else if (type == "modifications") {
      msg_types = {'E', 'C', 'X', 'D', 'U'};
      colnames = {""};
    } else if (type == "trades") {
      msg_types = {'P', 'Q', 'B'};
      colnames = {""};
    } else if (type == "noii") {
      msg_types = {'I'};
      colnames = {""};
    } else if (type == "rpii") {
      msg_types = {'N'};
      colnames = {""};
    } else if (type == "") {
      msg_types = {};
      colnames = {};
    } else {
      Rprintf("Unkown type of type '%s'\n", type.c_str());
      Rcpp::stop("Unknown message type\n");
    }
    colnames.insert(colnames.begin(), base_colnames.begin(), base_colnames.end());
    
    // Rprintf("Cols (%s): ", type.c_str());
    // for (std::string s : colnames) {
      // Rprintf("'%s' ", s.c_str());
    // }
    // Rprintf("\n");
  };
  
  void activate() {
    // Rprintf("Activate %s\n", type.c_str());
    active = true;
  }
  
  // set vectors
  void resize_vectors(int64_t n) {
    if (!active) return;
    size = n;
    // Rprintf("Resize %s to %" PRId64 "\n", type.c_str(), n);
    
    msg_type.resize(n);
    stock_locate.resize(n);
    tracking_number.resize(n);
    timestamp.resize(n);
    
    if (type == "system_events") {
      event_code.resize(n);
    } else if (type == "stock_directory") {
    } else if (type == "trading_status") {
    } else if (type == "reg_sho") {
    } else if (type == "market_participant_states") {
    } else if (type == "mwcb") {
    } else if (type == "ipo") {
    } else if (type == "luld") {
    } else if (type == "orders") {
      order_ref.resize(n);
      buy.resize(n);
      shares.resize(n);
      stock.resize(n);
      price.resize(n);
      mpid.resize(n);
    } else if (type == "modifications") {
    } else if (type == "trades") {
    } else if (type == "noii") {
    } else if (type == "rpii") {
    }
  }

  // returns true if the message was parsed
  int parse_message(char * buf) {
    const int msg_size = get_message_size(buf[0]);
    
    if (!active) return msg_size;
    // if !any(msg_types == buf[2]) return...
    bool cont = false;
    for (char type : msg_types) if (type == buf[0]) cont = true;
    if (!cont) return msg_size;
    
    // for all parse:
    // msg_type, stock_locate, tracking_number and timestamp
    msg_type[index]        = buf[0];
    stock_locate[index]    = get2bytes(&buf[1]);
    tracking_number[index] = get2bytes(&buf[3]);
    int64_t ts = get6bytes(&buf[5]);
    std::memcpy(&(timestamp[index]), &ts, sizeof(double));
    
    // parse specific values for each message
    if (type == "system_events") {
      event_code[index] = buf[11];
    } else if (type == "stock_directory") {
    } else if (type == "trading_status") {
    } else if (type == "reg_sho") {
    } else if (type == "market_participant_states") {
    } else if (type == "mwcb") {
    } else if (type == "ipo") {
    } else if (type == "luld") {
    } else if (type == "orders") {
      
      int64_t od = get8bytes(&buf[11]);
      std::memcpy(&(order_ref[index]), &od, sizeof(double));
      
      buy[index]    = buf[19] == 'B';
      shares[index] = get4bytes(&buf[20]);
      stock[index]  = getNBytes(&buf[24], 8);
      price[index]  = ((double) get4bytes(&buf[32])) / 10000.0;
      
      if (buf[0] == 'F') {
        mpid[index] = getNBytes(&buf[36], 4);
      } else {
        mpid[index] = "";
      }
      
    } else if (type == "modifications") {
    } else if (type == "trades") {
    } else if (type == "noii") {
    } else if (type == "rpii") {
    }
    
    index++;
    return msg_size;
  }
  // 
  // void prune_vectors() {
  //   int start = index, end = msg_type.size();
  //   
  //   msg_type.erase(msg_type.begin() + index, msg_type.end());
  //   stock_locate.erase(stock_locate.begin() + index, stock_locate.end());
  //   tracking_number.erase(tracking_number.begin() + index, tracking_number.end());
  //   timestamp.erase(timestamp.begin() + index, timestamp.end());
  //   
  //   if (type == "system_events") {
  //     event_code.erase(event_code.begin() + index, event_code.end());
  //   } else if (type == "stock_directory") {
  //   } else if (type == "trading_status") {
  //   } else if (type == "reg_sho") {
  //   } else if (type == "market_participant_states") {
  //   } else if (type == "mwcb") {
  //   } else if (type == "ipo") {
  //   } else if (type == "luld") {
  //   } else if (type == "orders") {
  //     
  //     int64_t od = get8bytes(&buf[11]);
  //     std::memcpy(&(order_ref[index]), &od, sizeof(double));
  //     
  //     buy[index]    = buf[19] == 'B';
  //     shares[index] = get4bytes(&buf[20]);
  //     stock[index]  = getNBytes(&buf[24], 8);
  //     price[index]  = ((double) get4bytes(&buf[32])) / 10000.0;
  //     
  //   } else if (type == "modifications") {
  //   } else if (type == "trades") {
  //   } else if (type == "noii") {
  //   } else if (type == "rpii") {
  //   }
  //   
  // }
  
  Rcpp::DataFrame get_data_frame() {
    if (!active) return Rcpp::DataFrame();
    
    // prune vector
    if (index != msg_type.size()) {
      Rprintf("Pruning found index '%" PRId64 "' msg_type_size '%" PRId64 "'!\n",
              index, msg_type.size());
      
      resize_vectors(index);
    }
    // create a dataframe
    Rcpp::List res(colnames.size());
    
    // treat 64 bit timestamp vector
    // Rcpp::NumericVector ts(timestamp.size());
    // std::memcpy(&(ts[0]), &(timestamp[0]), timestamp.size() * sizeof(double));
    // ts.attr("class") = "integer64";
    
    res[0] = msg_type;
    res[1] = stock_locate;
    res[2] = tracking_number;
    res[3] = copy_int64_vec(timestamp);
    
    if (type == "system_events") {
      res[4] = event_code;
    } else if (type == "stock_directory") {
    } else if (type == "trading_status") {
    } else if (type == "reg_sho") {
    } else if (type == "market_participant_states") {
    } else if (type == "mwcb") {
    } else if (type == "ipo") {
    } else if (type == "luld") {
    } else if (type == "orders") {
      
      Rcpp::NumericVector od(order_ref.size());
      std::memcpy(&(od[0]), &(order_ref[0]), order_ref.size() * sizeof(double));
      od.attr("class") = "integer64";
      
      res[4] = od;
      res[5] = buy;
      res[6] = shares;
      res[7] = stock;
      res[8] = price;
      res[9] = mpid;
      
      // treat missing values...
      fix_NA_string(res[9]);
      
    } else if (type == "modifications") {
    } else if (type == "trades") {
    } else if (type == "noii") {
    } else if (type == "rpii") {
    }
    
    res.names() = colnames;
    
    // treat 64 bit vectors
    
    // set List to DT
    res.attr("class") = Rcpp::StringVector::create("data.table", "data.frame");
    // need to call data.table::setalloccol() on data in R!
    
    // return DT
    return res;
  }
  
  std::vector<char> msg_types;
  bool active = false;
  
private:
  std::string type;
  int64_t size = 0, index = 0;
  
  // Rcpp Data Vectors
  std::vector<std::string> colnames;
  Rcpp::List data;
  
  std::vector<char>    msg_type;
  std::vector<int>     stock_locate;
  std::vector<int>     tracking_number;
  std::vector<int64_t> timestamp;
  
  // system events
  std::vector<char>    event_code;
  
  
  
  // orders
  std::vector<int64_t> order_ref;
  std::vector<bool> buy;
  std::vector<int> shares;
  std::vector<std::string> stock;
  std::vector<double> price;
  std::vector<std::string> mpid;
};

// counts messages in a file
std::vector<int64_t> count_messages_internal(std::string filename, int64_t max_buffer_size) {
  FILE* infile;
  infile = fopen(filename.c_str(), "rb");
  if (infile == NULL) Rcpp::stop("File Error!\n");
  
  // get size of the file
  fseek(infile, 0L, SEEK_END); 
  int64_t filesize = ftell(infile);
  fseek(infile, 0L, SEEK_SET);
  
  // create buffer
  int64_t buf_size = max_buffer_size > filesize ? filesize : max_buffer_size;
  char * buf;
  buf = (char*) malloc(buf_size);
  
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
    const int64_t offset = i - this_buffer_size;
    fseek(infile, i - this_buffer_size, SEEK_CUR);
    bytes_read += i;
  }
  
  free(buf);
  fclose(infile);
  return count;
}

// [[Rcpp::export]]
Rcpp::NumericVector count_messages_impl(std::string filename,
                                        int64_t max_buffer_size = 1e8,
                                        bool quiet = false) {
  
  std::vector<int64_t> count = count_messages_internal(filename, max_buffer_size);
  
  int64_t total_msgs = 0;
  for (int64_t v : count) total_msgs += v;
  if (!quiet) Rprintf("[Counting]   %s messages found\n", 
      formatThousands(total_msgs).c_str());
  
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


// [[Rcpp::export]]
Rcpp::List read_itch_impl(std::vector<std::string> classes,
                          std::string filename,
                          int64_t max_buffer_size = 1e8,
                          bool quiet = false) {
  
  
  std::vector<int64_t> count = count_messages_internal(filename, max_buffer_size);
  
  int64_t total_msgs = 0;
  for (int64_t v : count) total_msgs += v;
  if (!quiet) Rprintf("[Counting]   %s messages found\n", 
      formatThousands(total_msgs).c_str());
  
  std::vector<int64_t> sizes(classes.size());
  
  // initiate the MessageParsers and resize the vectors...
  
  // for each message class, hold a pointer to a message parser
  // message classes: 13

    // N_TYPES = 40, for each message in MSG_SIZES one
  std::vector<MessageParser*> msg_parsers(N_TYPES);
  std::map<std::string, MessageParser*> class_to_parsers;
  
  MessageParser empty("");
  for (int i = 0; i < N_TYPES; i++) msg_parsers[i] = &empty;
  
  // Rprintf("Loading Message Parsers\n");
  int c = 0;
  for (std::string cls : MSG_CLASSES) {
    // Rprintf("Looking at '%s'\n", cls.c_str());
    MessageParser* msgp_ptr = new MessageParser(cls);
    
    // check if this class is active?!
    for (const std::string c : classes) if (c == cls) msgp_ptr->activate();
    
    int64_t num_msg_this_type = 0;
    std::vector<char> this_msg_types = msgp_ptr->msg_types;
      // CLASS_TYPE_MAP.at(type);
    
    for (const char mt : this_msg_types) {
      // Rprintf("Message type: %c\n", mt);
      num_msg_this_type += count[mt - 'A'];
    }
    
    if (msgp_ptr->active) {
      // Rprintf("Active and resized to '%i'\n", num_msg_this_type);
      msgp_ptr->resize_vectors(num_msg_this_type);
    }
    
    class_to_parsers[cls] = msgp_ptr;
    for (const char mt : msgp_ptr->msg_types) msg_parsers[mt - 'A'] = msgp_ptr;
  }
  
  // parse the messages
  // redirect to the correct msg types only
  FILE* infile;
  infile = fopen(filename.c_str(), "rb");
  if (infile == NULL) Rcpp::stop("File Error!\n");
  
  // get size of the file
  fseek(infile, 0L, SEEK_END); 
  int64_t filesize = ftell(infile);
  fseek(infile, 0L, SEEK_SET);
  
  // create buffer
  int64_t buf_size = max_buffer_size > filesize ? filesize : max_buffer_size;
  char * buf;
  buf = (char*) malloc(buf_size);
  // Rprintf("Allocating buffer to size %" PRId64 "\n", buf_size);
  
  int64_t bytes_read = 0, this_buffer_size = 0;
  
  while (bytes_read < filesize) {
    Rcpp::checkUserInterrupt();
    
    // read in buffer buffers
    this_buffer_size = fread(buf, 1, buf_size, infile);
    int64_t i = 0;
    
    int msg_size = 0;
    do {
      // MEMORY NOT MAPPED error here! in the second round, but why?
      // Rprintf("offset %" PRId64 ":%" PRId64 " (size size %" PRId64 ") '%c'\n",
      //         bytes_read + i, bytes_read + i + get_message_size(buf[i + 2]),
      //         get_message_size(buf[i + 2]), buf[i + 2]);
      
      const char mt = buf[i + 2];
      msg_size = msg_parsers[mt - 'A']->parse_message(&buf[i + 2]);
      // Rprintf("  take ? %i\n", msg_parsers[mt - 'A']->active ? 1 : 0);
      // Rprintf("msg size %" PRId64 "\n", msg_size);
      i += msg_size;
      // Rprintf("  i %" PRId64 "\n", i);
      
      // Rprintf("i + msg_size <= this_buffer_size %" PRId64 " <= %" PRId64 "\n",
              // i + msg_size, this_buffer_size);
      // Rprintf("bytes_read + i <= filesize %" PRId64 " <= %" PRId64 "\n",
              // bytes_read + i, filesize);
      
    } while (i + msg_size <= this_buffer_size && bytes_read + i <= filesize);
    
    // offset file pointer to fit the next message into the buffer
    const int64_t offset = i - this_buffer_size;
    fseek(infile, i - this_buffer_size, SEEK_CUR);
    bytes_read += i;
  }
  
  free(buf);
  fclose(infile);
  
  // gather the data.frames into a list
  Rcpp::List res;
  for (std::string cls : classes) {
    
    Rcpp::DataFrame df = class_to_parsers[cls]->get_data_frame();
    res.push_back(df);
  }
  
  return res;
}
/***R
count_messages <- function(file, buffer_size = 1e8, quiet = FALSE) {
  t0 <- Sys.time()
  ct <- count_messages_impl(file, buffer_size, quiet)
  sel <- strsplit("SRHYLVWKJhAFECXDUPQBIN", "")[[1]]
  ct <- ct[sel]
  sel <- names(ct)
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

read_itch <- function(msg_class, file, buffer_size = 1e8, quiet = FALSE) {
  t0 <- Sys.time()
  poss_classes <- c(
    "system_events", "stock_directory", "trading_status", "reg_sho",
    "market_participant_states", "mwcb", "ipo", "luld", "orders", 
    "modifications", "trades", "noii", "rpii"
  )
  
  if (!all(msg_class %in% poss_classes)) stop("Invalid msg class detected")
  
  a <- read_itch_impl(msg_class, file, buffer_size, quiet)
  res <- lapply(a, data.table::setalloccol)[[1]]
  
  if (!quiet) 
    cat(sprintf("[Done]       in %.2f secs\n",
                difftime(Sys.time(), t0, units = "secs")))
  return(res)
}

read_system_events <- function(file, buffer_size = 1e8, quiet = FALSE) {
  read_itch("system_events", file, buffer_size, quiet)
}
read_orders <- function(file, buffer_size = 1e8, quiet = FALSE) {
  read_itch("orders", file, buffer_size, quiet)
}


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