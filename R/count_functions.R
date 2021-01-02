#' @name count_functions
#' @rdname count_functions
#' @title Counts the messages of an ITCH-file
#'
#' @param file the path to the input file, either a gz-file or a plain-text file
#' @param x a file or a data.table containing the message types and the counts,
#' as outputted by \code{count_messages}
#' @param add_meta_data if the meta-data of the messages should be added, defaults to FALSE
#' @param buffer_size the size of the buffer in bytes, defaults to 1e8 (100 MB), if you have a large amount of RAM, 1e9 (1GB) might be faster 
#' @param quiet if TRUE, the status messages are supressed, defaults to FALSE
#' @param force_gunzip only applies if file is a gz-file and a file with the same (gunzipped) name already exists.
#'        if set to TRUE, the existing file is overwritten. Default value is FALSE
#' @param force_cleanup only applies if file is a gz-file. If force_cleanup=TRUE, the gunzipped raw file will be deleted afterwards.
#'
#' @return a data.table containing the message-type and their counts for \code{count_messages}
#'  or an integer value for the other functions.
#' @export
#'
#' @examples
#' file <- system.file("extdata", "ex20101224.TEST_ITCH_50", package = "RITCH")
#' count_messages(file)
#' count_messages(file, add_meta_data = TRUE)
#' 
#' gz_file <- system.file("extdata", "ex20101224.TEST_ITCH_50.gz", package = "RITCH")
#' count_messages(gz_file)
#' 
#' # count only a specific class
#' msg_count <- count_messages(file)
#' 
#' # either count based on a given data.table outputted by count_messages
#' count_orders(msg_count)
#' 
#' # or count messages in a file
#' count_orders(file)
#' 
#' ### Specific class count functions are:
count_messages <- function(file, add_meta_data = FALSE, buffer_size = -1,
                           quiet = FALSE, force_gunzip = FALSE, 
                           force_cleanup = FALSE) {
  t0 <- Sys.time()
  if (!file.exists(file)) stop("File not found!")
  
  # Set the default value of the buffer size
  if (buffer_size < 0)
    buffer_size <- ifelse(grepl("\\.gz$", file), 
                          min(3 * file.size(file), 1e9),
                          1e8)
  
  if (buffer_size < 50) stop("buffer_size has to be at least 50 bytes, otherwise the messages won't fit")
  if (buffer_size > 5e9) warning("You are trying to allocate a large array on the heap, if the function crashes, try to use a smaller buffer_size")
  
  orig_file <- file
  file <- check_and_gunzip(file, buffer_size, force_gunzip, quiet)
  df <- count_messages_impl(file, buffer_size, quiet)
  
  df <- data.table::setalloccol(df)
  
  if (grepl("\\.gz$", orig_file) && force_cleanup) unlink(gsub("\\.gz", "", file))
  
  if (add_meta_data) {
    dd <- RITCH::get_meta_data()
    df <- df[dd, on = "msg_type"]
  }
  
  diff_secs <- as.numeric(difftime(Sys.time(), t0, units = "secs"))
  if (!quiet) cat(sprintf("[Done]       in %.2f secs\n", diff_secs))
  
  return(df)
}

#' Returns the meta data for the messages
#' 
#' All information is handled according to the official ITCH 5.0
#' documentation as found here: 
#' \url{http://www.nasdaqtrader.com/content/technicalsupport/specifications/dataproducts/NQTVITCHSpecification.pdf}
#' 
#' \code{msg_type} the type of the message 
#' \code{msg_name} the official name of the message
#' \code{msg_group} the group the message belongs to
#' \code{doc_nr} the number of the message in the documentation
#'
#' @return a data.table with the information of the message-types
#' @export
#'
#' @examples
#' get_meta_data()
get_meta_data <- function() {
  data.table::data.table(
    msg_type = c("S", "R", "H", "Y", "L", "V", "W", "K", "J", "h", "A", "F", "E", 
                 "C", "X", "D", "U", "P", "Q", "B", "I", "N"),
    msg_class = c("system_events", "stock_directory", "trading_status",
                  "reg_sho", "market_participant_states", "mwcb", 
                  "mwcb", "ipo", "luld", "trading_status", "orders", "orders",
                  "orders", "modifications", "modifications", "modifications",
                  "modifications", "trades", "trades", "trades", "trades",
                  "noii", "rpii"),
    msg_name = c("System Event Message", "Stock Directory", 
                 "Stock Trading Action", "Reg SHO Restriction", 
                 "Market Participant Position", "MWCB Decline Level Message",
                 "MWCB Status Message", "IPO Quoting Period Update",
                 "LULD Auction Collar", "Operational Halt", "Add Order Message", 
                 "Add Order - MPID Attribution Message",
                 "Order Executed Message", 
                 "Order Executed Message With Price Message",
                 "Order Cancel Message", "Order Delete Message",
                 "Order Replace Message", "Trade Message (Non-Cross)",
                 "Cross Trade Message", "Broken Trade Message", 
                 "NOII Message",
                 "Retail Interest Message"),
    doc_nr = c("4.1", "4.2.1", "4.2.2", "4.2.3", "4.2.4", "4.2.5.1", "4.2.5.2",
               "4.2.6", "4.2.7", "4.2.8", "4.3.1", "4.3.2", "4.4.1", "4.4.2", "4.4.3",
               "4.4.4", "4.4.5", "4.5.1", "4.5.2", "4.5.3", "4.6", "4.7")
  )
}

#' Internal function to count the messages
#'
#' @param x a data.frame containing the message types and the counts
#' @param types a vector containing the types
#'
#' @keywords internal
#' @return a numeric value of number of orders in x
#'
#' @examples
#' # Only used internally
count_internal <- function(x, types) {
  if (!is.data.frame(x)) stop("x has to be a data.table")
  if (!all(c("msg_type", "count") %in% names(x))) 
    stop("x has to have the variables 'msg_type' and 'count'")
  
  as.integer(x[msg_type %in% types][, sum(count)])
}

#' @rdname count_functions
#' @export
#' @details
#' \itemize{
#'  \item{\code{count_orders}: Counts order messages. Message type \code{A} and 
#'    \code{F}}
#' }
#' @examples
#' count_orders(msg_count)
count_orders <- function(x) {
  if (is.character(x)) x <- count_messages(x, quiet = TRUE)
  types <- c("A", "F")
  count_internal(x, types)
}

#' @rdname count_functions
#' @export
#' @details 
#' \itemize{
#'  \item{\code{count_trades}: Counts trade messages. Message type \code{P}, 
#'    \code{Q} and \code{B}}
#' }
#' @examples
#' count_trades(msg_count)
count_trades <- function(x) {
  if (is.character(x)) x <- count_messages(x, quiet = TRUE)
  types <- c("P", "Q", "B")
  count_internal(x, types)
}

#' @rdname count_functions
#' @export
#' @details 
#' \itemize{
#'  \item{\code{count_modifications}: Counts order modification messages. Message 
#'    type \code{E}, \code{C}, \code{X}, \code{D}, and \code{U}}
#' }
#' @examples
#' count_modifications(msg_count)
count_modifications <- function(x) {
  if (is.character(x)) x <- count_messages(x, quiet = TRUE)
  types <- c("E", "C", "X", "D", "U")
  count_internal(x, types)
}

#' @rdname count_functions
#' @export
#' @details 
#' \itemize{
#'  \item{\code{count_system_events}: Counts system event messages. Message type 
#'    \code{S}}
#' }
#' @examples
#' count_system_events(msg_count)
count_system_events <- function(x) {
  if (is.character(x)) x <- count_messages(x, quiet = TRUE)
  types <- c("S")
  count_internal(x, types)
}

#' @rdname count_functions
#' @export
#' @details 
#' \itemize{
#'  \item{\code{count_stock_directory}: Counts stock trading messages. Message 
#'    type \code{R}}
#' }
#' @examples
#' count_stock_directory(msg_count)
count_stock_directory <- function(x) {
  if (is.character(x)) x <- count_messages(x, quiet = TRUE)
  types <- c("R")
  count_internal(x, types)
}

#' @rdname count_functions
#' @export
#' @details 
#' \itemize{
#'  \item{\code{count_trading_status}: Counts trading status messages. Message 
#'    type \code{H} and \code{h}}
#' }
#' @examples
#' count_trading_status(msg_count)
count_trading_status <- function(x) {
  if (is.character(x)) x <- count_messages(x, quiet = TRUE)
  types <- c("H", "h")
  count_internal(x, types)
}

#' @rdname count_functions
#' @export
#' @details 
#' \itemize{
#'  \item{\code{count_reg_sho}: Counts messages regarding reg SHO. Message type 
#'    \code{Y}}
#' }
#' @examples
#' count_reg_sho(msg_count)
count_reg_sho <- function(x) {
  if (is.character(x)) x <- count_messages(x, quiet = TRUE)
  types <- c("Y")
  count_internal(x, types)
}

#' @rdname count_functions
#' @export
#' @details 
#' \itemize{
#'  \item{\code{count_market_participant_states}: Counts messages regarding the 
#'    status of market participants. Message type \code{L}}
#' }
#' @examples
#' count_market_participant_states(msg_count)
count_market_participant_states <- function(x) {
  if (is.character(x)) x <- count_messages(x, quiet = TRUE)
  types <- c("L")
  count_internal(x, types)
}

#' @rdname count_functions
#' @export
#' @details 
#' \itemize{
#'  \item{\code{count_mwcb}: Counts messages regarding Market-Wide-Circuit-Breakers
#'    (MWCB). Message type \code{V} and \code{W}}
#' }
#' @examples
#' count_mwcb(msg_count)
count_mwcb <- function(x) {
  if (is.character(x)) x <- count_messages(x, quiet = TRUE)
  types <- c("V", "W")
  count_internal(x, types)
}

#' @rdname count_functions
#' @export
#' @details 
#' \itemize{
#'  \item{\code{count_ipo}: Counts messages regarding IPOs. Message type \code{K}}
#' }
#' @examples
#' count_ipo(msg_count)
count_ipo <- function(x) {
  if (is.character(x)) x <- count_messages(x, quiet = TRUE)
  types <- c("K")
  count_internal(x, types)
}

#' @rdname count_functions
#' @export
#' @details 
#' \itemize{
#'  \item{\code{count_luld}: Counts messages regarding LULDs (limit up-limit down)
#'    auction collars. Message type \code{J}}
#' }
#' @examples
#' count_luld(msg_count)
count_luld <- function(x) {
  if (is.character(x)) x <- count_messages(x, quiet = TRUE)
  types <- c("J")
  count_internal(x, types)
}

#' @rdname count_functions
#' @export
#' @details 
#' \itemize{
#'  \item{\code{count_noii}: Counts Net Order Imbalance Indicatio (NOII) messages. 
#'    Message type \code{I}}
#' }
#' @examples
#' count_noii(msg_count)
count_noii <- function(x) {
  if (is.character(x)) x <- count_messages(x, quiet = TRUE)
  types <- c("I")
  count_internal(x, types)
}

#' @rdname count_functions
#' @export
#' @details 
#' \itemize{
#'  \item{\code{count_rpii}: Counts Retail Price Improvement Indicator (RPII) 
#'    messages. Message type \code{N}}
#' }
#' @examples
#' count_rpii(msg_count)
count_rpii <- function(x) {
  if (is.character(x)) x <- count_messages(x, quiet = TRUE)
  types <- c("N")
  count_internal(x, types)
}
