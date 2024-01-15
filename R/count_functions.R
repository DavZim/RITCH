#' @name count_functions
#' @rdname count_functions
#' @title Counts the messages of an ITCH-file
#'
#' @param file the path to the input file, either a gz-file or a plain-text file
#' @param x a file or a data.table containing the message types and the counts,
#' as outputted by `count_messages`
#' @param add_meta_data if the meta-data of the messages should be added, defaults to FALSE
#' @param buffer_size the size of the buffer in bytes, defaults to 1e8 (100 MB), if you have a large amount of RAM, 1e9 (1GB) might be faster
#' @param quiet if TRUE, the status messages are supressed, defaults to FALSE
#' @param force_gunzip only applies if file is a gz-file and a file with the same (gunzipped) name already exists.
#'        if set to TRUE, the existing file is overwritten. Default value is FALSE
#' @param gz_dir a directory where the gz archive is extracted to.
#'        Only applies if file is a gz archive. Default is [tempdir()].    
#' @param force_cleanup only applies if file is a gz-file. If force_cleanup=TRUE, the gunzipped raw file will be deleted afterwards.
#' @return a data.table containing the message-type and their counts for `count_messages`
#'  or an integer value for the other functions.
#' @export
#'
#' @examples
#' file <- system.file("extdata", "ex20101224.TEST_ITCH_50", package = "RITCH")
#' count_messages(file)
#' count_messages(file, add_meta_data = TRUE, quiet = TRUE)
#'
#' # file can also be a .gz file
#' gz_file <- system.file("extdata", "ex20101224.TEST_ITCH_50.gz", package = "RITCH")
#' count_messages(gz_file, quiet = TRUE)
#'
#' # count only a specific class
#' msg_count <- count_messages(file, quiet = TRUE)
#'
#' # either count based on a given data.table outputted by count_messages
#' count_orders(msg_count)
#'
#' # or count orders from a file and not from a msg_count
#' count_orders(file)
#'
#' ### Specific class count functions are:
count_messages <- function(file, add_meta_data = FALSE, buffer_size = -1,
                           quiet = FALSE, force_gunzip = FALSE,
                           gz_dir = tempdir(), force_cleanup = TRUE) {
  t0 <- Sys.time()
  if (!file.exists(file))
    stop(sprintf("File '%s' not found!", file))

  # Set the default value of the buffer size
  buffer_size <- check_buffer_size(buffer_size, file)

  orig_file <- file
  # only needed for gz files; gz files are not deleted when the raw file already existed
  raw_file_existed <- file.exists(basename(gsub("\\.gz$", "", file)))
  file <- check_and_gunzip(file, gz_dir, buffer_size, force_gunzip, quiet)
  df <- count_messages_impl(file, buffer_size, quiet)

  df <- data.table::setalloccol(df)

  if (add_meta_data) {
    dd <- RITCH::get_msg_classes()
    df <- df[dd, on = "msg_type"]
  }

  report_end(t0, quiet, orig_file)

  if (grepl("\\.gz$", orig_file) && force_cleanup && !raw_file_existed) {
    unlink(basename(gsub("\\.gz$", "", file)))
    if (!quiet) cat(sprintf("[Cleanup]    Removing file '%s'\n", file))
  }

  return(df)
}

#' Returns the message class data for the message types
#'
#' All information is handled according to the official ITCH 5.0
#' documentation as found here:
#' <http://www.nasdaqtrader.com/content/technicalsupport/specifications/dataproducts/NQTVITCHSpecification.pdf>
#'
#' - `msg_type` the type of the message
#' - `msg_class` the group the message belongs to
#' - `msg_name` the official name of the message
#' - `doc_nr` the number of the message in the documentation
#'
#' @seealso `open_itch_specification()`
#'
#' @return a data.table with the information of the message-types
#' @export
#'
#' @examples
#' get_msg_classes()
get_msg_classes <- function() {
  data.table::data.table(
    msg_type = c("S", "R", "H", "Y", "L", "V", "W", "K", "J", "h", "A", "F", "E",
                 "C", "X", "D", "U", "P", "Q", "B", "I", "N"),
    msg_class = c("system_events", "stock_directory", "trading_status",
                  "reg_sho", "market_participant_states", "mwcb",
                  "mwcb", "ipo", "luld", "trading_status", "orders", "orders",
                  "modifications", "modifications", "modifications",
                  "modifications", "modifications", "trades", "trades", "trades",
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
#' - `count_orders`: Counts order messages. Message type `A` and `F`
#'
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
#' - `count_trades`: Counts trade messages. Message type `P`, `Q` and `B`
#'
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
#' - `count_modifications`: Counts order modification messages. Message
#'    type `E`, `C`, `X`, `D`, and `U`
#'
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
#' - `count_system_events`: Counts system event messages. Message type `S`
#'
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
#' - `count_stock_directory`: Counts stock trading messages. Message
#'    type `R`
#'
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
#' - `count_trading_status`: Counts trading status messages. Message
#'    type `H` and `h`
#'
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
#' - `count_reg_sho`: Counts messages regarding reg SHO. Message type
#'    `Y`
#'
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
#' - `count_market_participant_states`: Counts messages regarding the
#'    status of market participants. Message type `L`
#'
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
#' - `count_mwcb`: Counts messages regarding Market-Wide-Circuit-Breakers
#'    (MWCB). Message type `V` and `W`
#'
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
#' - `count_ipo`: Counts messages regarding IPOs. Message type `K`
#'
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
#' - `count_luld`: Counts messages regarding LULDs (limit up-limit down)
#'    auction collars. Message type `J`
#'
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
#' - `count_noii`: Counts Net Order Imbalance Indicatio (NOII) messages.
#'    Message type `I`
#'
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
#' - `count_rpii`: Counts Retail Price Improvement Indicator (RPII)
#'    messages. Message type `N`
#'
#' @examples
#' count_rpii(msg_count)
count_rpii <- function(x) {
  if (is.character(x)) x <- count_messages(x, quiet = TRUE)
  types <- c("N")
  count_internal(x, types)
}
