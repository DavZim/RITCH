#' @name read_functions
#' @rdname read_functions
#'
#' @title Reads certain messages of an ITCH-file into a data.table
#' 
#' @description
#' 
#' For faster file-reads (at the tradeoff of increased memory usages), you can
#' increase the \code{buffer_size} to 1GB (1e9) or more.
#' 
#' If you read multiple different message types from the same file, you can 
#' provide the message counts as outputted from \code{\link{count_messages}} to
#' the \code{n_max} argument, this allows skipping one pass over the 
#' file per read instruction. 
#'
#' If the file is too large to be loaded into the workspace at once, you can
#' specify different \code{skip} and \code{n_max} to load only
#' a specific range of messages.
#' 
#' Note that all read functions allow both plain ITCH files as well as gzipped 
#' files. If a gzipped file is found, it will look for a plain ITCH file with
#' the same name and use that instead. If this file is not found, it will be 
#' created. Use \code{force_cleanup = TRUE} to force the deletion of the 
#' unzipped file after use.
#' 
#' @param file the path to the input file, either a gz-file or a plain ITCH file
#' @param type the type to load, can be "orders", "trades", "modifications", ... Only applies to the read_ITCH() function.
#' @param skip Number of messages to skip before starting parsing messages, 
#' note it only applies to messages of this type.
#' @param n_max Maximum number of messages to parse, default is to read all values.
#'  Can also be a data.frame of msg_types and counts, as returned by 
#'  \code{\link{count_messages}}
#' @param filter_msg_type a character vector, specifying a filter for message types.
#'  Note that this can be used to only return 'A' orders for instance.
#' @param filter_stock_locate an integer vector, specifying a filter for locate codes.
#'  The locate codes can be looked up by calling \code{\link{read_stock_directory}}.
#'  Note that some message types (e.g., system events) do not use a locate code.
#' @param min_timestamp an 64 bit integer vector (see also \code{\link[bit64]{as.integer64}})
#'  of minimum timestamp (inclusive). 
#'  Note: min and max timestamp must be supplied with the same length or left empty.
#' @param max_timestamp an 64 bit integer vector (see also \code{\link[bit64]{as.integer64}})
#'  of maxium timestamp (inclusive).
#'  Note: min and max timestamp must be supplied with the same length or left empty.
#' @param filter_stock a character vector, specifying a filter for stocks.
#'  Note that this a shorthand for the \code{filter_stock_locate} argument, as it
#'  tries to find the stock_locate based on the \code{stock_directory} argument,
#'  if this is not found, it will try to extract the stock directory from the file,
#'  else an error is thrown.
#' @param stock_directory A data.frame containing the stock-locate code relationship.
#' As outputted by \code{\link{read_stock_directory}}. 
#' Only used if \code{filter_stock} is set. To download the stock directory from
#' NASDAQs FTP server, use \code{\link{download_stock_directory}}.
#' @param buffer_size the size of the buffer in bytes, defaults to 1e8 (100 MB), 
#' if you have a large amount of RAM, 1e9 (1GB) might be faster 
#' @param quiet if TRUE, the status messages are suppressed, defaults to FALSE
#' @param add_meta if TRUE, the date and exchange information of the file are added, defaults to TRUE
#' @param force_gunzip only applies if file is a gz-file and a file with the same (gunzipped) name already exists.
#'        if set to TRUE, the existing file is overwritten. Default value is FALSE
#' @param force_cleanup only applies if file is a gz-file. If force_cleanup=TRUE, the gunzipped raw file will be deleted afterwards.
#' @param ... Additional arguments passed to \code{read_ITCH}
#' @param add_descriptions add longer descriptions to shortened variables.
#' The added information is taken from the official ITCH documentation
#' see also \code{\link{open_itch_specification}}
#' 
#' @details
#' The details of the different messages types can be found in the official
#' ITCH specification (see also \code{\link{open_itch_specification}})
#'
#' \itemize{
#'  \item{\code{read_ITCH}: Generic function called by the other read 
#'    functions.}
#' }
#' @references \url{https://www.nasdaqtrader.com/content/technicalsupport/specifications/dataproducts/NQTVITCHspecification.pdf}
#' 
#' @return a data.table containing the messages
#'
#' @examples
#' file <- system.file("extdata", "ex20101224.TEST_ITCH_50", package = "RITCH")
#' od <- read_orders(file)
#' tr <- read_trades(file)
#' md <- read_modifications(file)
#' 
#' ## Alternatively 
#' od <- read_ITCH(file, "orders")
#' 
#' str(od)
#' str(tr)
#' str(md)
#' 
#' # additional options:
#' 
#' # turn off feedback
#' od <- read_orders(file, quiet = TRUE)
#' 
#' # take only subset of messages
#' od <- read_orders(file, skip = 3, n_max = 10)
#' 
#' # a message count can be provided for slightly faster reads
#' msg_count <- count_messages(file)
#' od <- read_orders(file, n_max = msg_count)
#'   
#' # .gz files will be automatically unzipped
#' gz_file <- system.file("extdata", "ex20101224.TEST_ITCH_50.gz", package = "RITCH")
#' od <- read_orders(gz_file)
#' # force a decompress and delete the decompressed file afterwards
#' od <- read_orders(gz_file, force_gunzip = TRUE, force_cleanup = TRUE)
#' 
NULL

#' @rdname read_functions
#' @export
read_ITCH <- function(file, type, skip = 0, n_max = -1, 
                      filter_msg_type = NA_character_, filter_stock_locate = NA_integer_,
                      min_timestamp = bit64::as.integer64(NA),
                      max_timestamp = bit64::as.integer64(NA),
                      filter_stock = NA_character_, stock_directory = NA,
                      buffer_size = -1, quiet = FALSE, add_meta = TRUE,
                      force_gunzip = FALSE, force_cleanup = FALSE) {
  
  t0 <- Sys.time()
  if (!file.exists(file)) stop("File not found!")
  
  type <- tolower(type)
  msg_types <- list(
    "trades" = c("P", "Q", "B"),
    "orders" = c("A", "F"),
    "modifications" = c("E", "C", "X", "D", "U"),
    "system_events" = "S",
    "stock_directory" = "R",
    "trading_status" = c("H", "h"),
    "reg_sho" = "Y",
    "participant_states" = "L",
    "mwcb" = c("V", "W"),
    "ipo" = "K",
    "luld" = "J",
    "noii" = "I",
    "rpii" = "N"
  )
  
  imp_calls <- list(
    "trades" = getTrades_impl,
    "orders" = getOrders_impl,
    "modifications" = getModifications_impl,
    "system_events" = getSystemEvents_impl,
    "stock_directory" = getStockDirectory_impl,
    "trading_status" = getTradingStatus_impl,
    "reg_sho" = getRegSHO_impl,
    "participant_states" = getParticipantStates_impl,
    "mwcb" = getMWCB_impl,
    "ipo" = getIPO_impl,
    "luld" = getLULD_impl,
    "noii" = getNOII_impl,
    "rpii" = getRPII_impl
  )
  
  stopifnot(type %in% names(msg_types))
  
  # treat n_max
  n_max_is_dataframe <- is.data.frame(n_max)
  if (n_max_is_dataframe) {
    if (!all(c("msg_type", "count") %in% names(n_max))) 
      stop("If n_max is a data.frame/table, it must contain 'msg_type' and 'count'!")
    skip <- 0
    n_max <- as.integer(n_max[msg_type %in% msg_types[[type]], sum(count)])
  }
  # -1 because we want it 1 indexed in R (cpp is 0-indexed), +1 as we want to skip 
  start <- max(skip - 1 + 1, 0)
  end <- max(skip + n_max - 1, -1)
  if (is.numeric(n_max) && n_max != -1 && !quiet && !n_max_is_dataframe) 
    cat("NOTE: as n_max overrides counting the messages, the numbers for messages may be off.\n")
  
  # Treat filters
  # Message types
  # allow msg_types: 'AF' (multiple values are split), c('A', 'F'), c(NA, 'A') (NAs are ommited)
  filter_msg_type <- unique(filter_msg_type)
  if (any(nchar(filter_msg_type) > 1, na.rm = TRUE)) 
    filter_msg_type <- as.character(unlist(sapply(filter_msg_type, strsplit, split = "")))
  filter_msg_type <- filter_msg_type[!is.na(filter_msg_type)]
  if (!quiet && length(filter_msg_type) > 0) 
    cat(paste0("[Filter]     msg_type: '", 
               paste(filter_msg_type, collapse = "', '"),
               "'\n"))
  
  # locate code
  filter_stock_locate <- filter_stock_locate[!is.na(filter_stock_locate)]
  filter_stock_locate <- as.integer(filter_stock_locate)

  # Timestamp
  min_timestamp <- min_timestamp[!is.na(min_timestamp)]
  max_timestamp <- max_timestamp[!is.na(max_timestamp)]
  
  min_timestamp <- bit64::as.integer64(min_timestamp)
  max_timestamp <- bit64::as.integer64(max_timestamp)
  
  if (!(length(min_timestamp) == length(max_timestamp) ||
        length(min_timestamp) == 0 | length(max_timestamp) == 0))
    stop("min_timestamp and max_timestamp has to have the same length or have to be not specified!")
  
  if (!quiet && (length(min_timestamp) > 0 | length(max_timestamp) > 0)) {
    txt <- "[Filter]     timestamp: "
    if (length(max_timestamp) == 0) {
      txt <- paste0(txt, ">= ", min_timestamp)
    } else if (length(min_timestamp) == 0) {
      txt <- paste0(txt, "<= ", max_timestamp)
    } else if (length(min_timestamp) == length(max_timestamp)) {
      txt <- paste0(txt, paste(min_timestamp, max_timestamp,
                               sep = " - ", collapse = ", "))
    }
    cat(txt, "\n")
  }
  
  # Stock
  if (!(length(filter_stock) == 1 && is.na(filter_stock))) {
    if (length(stock_directory) == 1 && is.na(stock_directory)) {
      warning("filter_stock is given, but no stock_directory is specified. Trying to extract stock directory from file\n")
      stock_directory <- read_stock_directory(file, quiet = TRUE)
    }
    
    if (!all(filter_stock %chin% stock_directory$stock)) {
      stop(paste0("Not all stocks found in stock_directory, missing: '",
                  paste(filter_stock[!filter_stock %chin% stock_directory$stock], 
                        collapse = "', '"),
                  "'"))
    }
    # extend locate code by the stocks:
    filter_stock_locate <- c(filter_stock_locate,
                             stock_directory[stock %chin%filter_stock, stock_locate])
  }
  
  if (!quiet && length(filter_stock_locate) > 0) 
    cat(paste0("[Filter]     stock_locate: '", 
               paste(filter_stock_locate, collapse = "', '"),
               "'\n"))
  
  if (any(length(filter_stock_locate) > 0,
          length(filter_msg_type) > 0,
          length(min_timestamp) > 0,
          length(max_timestamp) > 0) && !quiet) 
    cat("NOTE: as filter arguments were given, the number of messages may be off\n")
  
  
  # Set the default value of the buffer size
  if (buffer_size < 0)
    buffer_size <- ifelse(grepl("\\.gz$", file), 
                          min(3 * file.size(file), 1e9), 
                          1e8)
  
  if (buffer_size < 50) stop("buffer_size has to be at least 50 bytes, otherwise the messages won't fit")
  if (buffer_size > 5e9) warning("You are trying to allocate a large array on the heap, if the function crashes, try to use a smaller buffer_size")
  
  filedate <- get_date_from_filename(file)
  
  orig_file <- file
  file <- check_and_gunzip(file, buffer_size, force_gunzip, quiet)
  
  # actually call the C++ function
  df <- imp_calls[[type]](file, start, end, 
                          filter_msg_type, filter_stock_locate,
                          min_timestamp, max_timestamp,
                          buffer_size, quiet)
  
  # if the file was gzipped and the force_cleanup=TRUE, delete unzipped file 
  if (grepl("\\.gz$", orig_file) && force_cleanup) unlink(gsub("\\.gz", "", file))
  
  if (!quiet) cat("\n[Converting] to data.table\n")
  df <- data.table::setalloccol(df)
  
  if (add_meta) {
    # add the date and exchange
    df[, date := filedate]
    df[, datetime := nanotime(as.Date(filedate)) + timestamp]
    df[, exchange := get_exchange_from_filename(file)]
  }
  
  # remove messages with empty msg_types, this can be the case if n_max was set
  # to a large value
  df <- df[msg_type != ""]
  
  a <- gc()
  
  diff_secs <- as.numeric(difftime(Sys.time(), t0, units = "secs"))
  if (!quiet) cat(sprintf("[Done]       in %.2f secs\n", diff_secs))
  
  return(df[])
}

#' @rdname read_functions
#' @export
#' @details
#' \itemize{
#'  \item{\code{read_orders}: Reads order messages. Message type \code{A} and 
#'    \code{F}}
#' }
#' @examples 
#' 
#' ## read_orders()
#' read_orders(file)
read_orders <- function(file, ...) {
  dots <- list(...)
  dots$file <- file
  dots$type <- "orders"
  do.call(read_ITCH, dots)
}

#' @rdname read_functions
#' @export
#' @details 
#' \itemize{
#'  \item{\code{read_trades}: Reads trade messages. Message type \code{P}, 
#'    \code{Q} and \code{B}}
#' }
#' @examples 
#' 
#' ## read_trades()
#' read_trades(file)
read_trades <- function(file, ...) {
  dots <- list(...)
  dots$file <- file
  dots$type <- "trades"
  do.call(read_ITCH, dots)
}

#' @rdname read_functions
#' @export
#' @details 
#' \itemize{
#'  \item{\code{read_modifications}: Reads order modification messages. Message 
#'    type \code{E}, \code{C}, \code{X}, \code{D}, and \code{U}}
#' }
#' @examples 
#' 
#' ## read_modifications()
#' read_modifications(file)
read_modifications <- function(file, ...) {
  dots <- list(...)
  dots$file <- file
  dots$type <- "modifications"
  do.call(read_ITCH, dots)
}

#' @rdname read_functions
#' @export
#' @details 
#' \itemize{
#'  \item{\code{read_system_events}: Reads system event messages. Message type 
#'    \code{S}}
#' }
#' @examples 
#' 
#' ## read_system_events()
#' read_system_events(file)
#' read_system_events(file, add_descriptions = TRUE)
read_system_events <- function(file, ..., add_descriptions = FALSE) {
  dots <- list(...)
  dots$file <- file
  dots$type <- "system_events"
  res <- do.call(read_ITCH, dots)
  
  if (add_descriptions) {
    names_ <- names(res)
    
    ei <- data.table::data.table(
      event_code = c("O", "S", "Q", "M", "E", "C"),
      event_name = c("Start of Messages", "Start of System Hours", "Start of Market Hours", "End of Market Hours", "End of System Hours", "End of Messages"),
      event_note = c(
        "Outside of time stamp messages, the start of day message is the first message sent in any trading day",
        "This message indicates that NASDAQ is open and ready to start accepting orders",
        "This message is intended to indicate that Market Hours orders are available for execution",
        "This message is intended to indicate that Market Hours orders are no longer available for execution",
        "It indicates that Nasdaq is now closed and will not accept any new orders today. It is still possible to receive Broken Trade messages and Order Delete messages after the End of Day",
        "This is always the last message sent in any trading day."
        )
    )
    res <- merge(res, ei, by = "event_code", all.x = TRUE)
    data.table::setcolorder(res, names_)
  }
  
  return(res)
}

#' @rdname read_functions
#' @export
#' @details 
#' \itemize{
#'  \item{\code{read_stock_directory}: Reads stock trading messages. Message 
#'    type \code{R}}
#' }
#' @examples 
#' 
#' ## read_stock_directory()
#' read_stock_directory(file)
#' read_stock_directory(file, add_descriptions = TRUE)
read_stock_directory <- function(file, ..., add_descriptions = FALSE) {
  dots <- list(...)
  dots$file <- file
  dots$type <- "stock_directory"
  res <- do.call(read_ITCH, dots)
  
  if (add_descriptions) {
    names_ <- names(res)
    
    mcat <- data.table::data.table(
      market_category = c("Q", "G", "S", "N", "A", "P", "Z", "V", " "),
      market_category_note = c(
        "Nasdaq Global Select Market",
        "Nasdaq Global Market",
        "Nasdaq Capital Market",
        "New York Stock Exchange",
        "NYSE MKT",
        "NYSE Arca",
        "BATS Z Exchange",
        "Investor's Exchange, LLC",
        NA_character_
      )
    )
    res <- merge(res, mcat, by = "market_category", all.x = TRUE)
    
    finstat <- data.table::data.table(
      financial_status = c("D", "E", "Q", "S", "G", "H", "J", "K", "C", "N", " "),
      financial_status_note = c(
        "Deficient", "Delinquent", "Bankrupt", "Suspended", "Deficient and Bankrupt",
        "Deficient and Delinquent", "Delinquent and Bankrupt", "Deficient, Delinquent and Bankrupt",
        "Creations and/or Redemptions Suspended for Exchange Traded Product",
        "Normal", NA_character_
      )
    )
    res <- merge(res, finstat, by = "financial_status", all.x = TRUE)
    
    luld <- data.table::data.table(
      luld_price_tier = c("1", "2", " "),
      luld_price_tier_note = c(
        "Tier 1 NMS Stock and selected ETPs",
        "Tier 2 NMS Stocks",
        NA_character_
      )
    )
    
    res <- merge(res, luld, by = "luld_price_tier", all.x = TRUE)
    data.table::setcolorder(res, names_)
  }
  
  return(res)
}

#' @rdname read_functions
#' @export
#' @details 
#' \itemize{
#'  \item{\code{read_trading_status}: Reads trading status messages. Message 
#'    type \code{H} and \code{h}}
#' }
#' @examples 
#' 
#' ## read_trading_status()
#' read_trading_status(file)
#' read_trading_status(file, add_descriptions = TRUE)
read_trading_status <- function(file, ..., add_descriptions = FALSE) {
  dots <- list(...)
  dots$file <- file
  dots$type <- "trading_status"
  res <- do.call(read_ITCH, dots)
  
  if (add_descriptions) {
    names_ <- names(res)
    
    trs <- data.table::data.table(
      trading_state = c("H", "P", "Q", "T"),
      trading_state_note = c(
        "Halted across all US equity markets / SROs",
        "Paused across all US equity markets / SROs (Nasdaq-listed securities only",
        "Quotation only period for cross-SRO halt or pause",
        "Trading on Nasdaq"
      )
    )
    res <- merge(res, trs, by = "trading_state", all.x = TRUE)
    
    mkt <- data.table::data.table(
      market_code = c("Q", "B", "X"),
      market_code_note = c("Nasdaq", "BX", "PSX")
    )
    res <- merge(res, mkt, by = "market_code", all.x = TRUE)
    
    data.table::setcolorder(res, names_)
  }
  
  return(res)
}

#' @rdname read_functions
#' @export
#' @details 
#' \itemize{
#'  \item{\code{read_reg_sho}: Reads messages regarding reg SHO. Message type 
#'    \code{Y}}
#' }
#' @examples 
#' 
#' ## read_reg_sho()
#' read_reg_sho(file)
#' read_reg_sho(file, add_descriptions = TRUE)
read_reg_sho <- function(file, ..., add_descriptions = FALSE) {
  dots <- list(...)
  dots$file <- file
  dots$type <- "reg_sho"
  res <- do.call(read_ITCH, dots)
  
  if (add_descriptions) {
    names_ <- names(res)
    ac <- data.table::data.table(
      regsho_action = c("0", "1", "2"),
      regsho_action_note = c(
        "No price test in place",
        "Reg SHO Short Sale Price Test Restriction in effect due to an intra-day price drop in security",
        "Reg SHO Short Sale Price Test Restriction remains in effect"
      )
    )
    
    res <- merge(res, ac, by = "regsho_action", all.x = TRUE)
    
    data.table::setcolorder(res, names_)
  }
  
  return(res)
}

#' @rdname read_functions
#' @export
#' @details 
#' \itemize{
#'  \item{\code{read_market_participant_states}: Reads messages regarding the 
#'    status of market participants. Message type \code{L}}
#' }
#' @examples 
#' 
#' ## read_market_participant_states()
#' read_market_participant_states(file)
#' read_market_participant_states(file, add_descriptions = TRUE)
read_market_participant_states <- function(file, ..., add_descriptions = FALSE) {
  dots <- list(...)
  dots$file <- file
  dots$type <- "participant_states"
  res <- do.call(read_ITCH, dots)
  
  if (add_descriptions) {
    names_ <- names(res)
    mmm <- data.table::data.table(
      mm_mode = c("N", "P", "S", "R", "L"),
      mm_mode_note = c("Normal", "Passive", "Syndicate", "Pre-Syndicate", "Penalty")
    )
    
    res <- merge(res, mmm, by = "mm_mode", all.x = TRUE)
    
    ps <- data.table::data.table(
      participant_state = c("A", "E", "W", "S", "D"),
      participant_state_note = c("Active", "Excused/withdrawn", "Withdrawn", 
                                 "Suspended", "Deleted")
    )
    res <- merge(res, ps, by = "participant_state", all.x = TRUE)
    
    data.table::setcolorder(res, names_)
  }
  
  return(res)
}

#' @rdname read_functions
#' @export
#' @details 
#' \itemize{
#'  \item{\code{read_mwcb}: Reads messages regarding Market-Wide-Circuit-Breakers
#'    (MWCB). Message type \code{V} and \code{W}}
#' }
#' @examples 
#' 
#' ## read_mwcb()
#' read_mwcb(file)
read_mwcb <- function(file, ...) {
  dots <- list(...)
  dots$file <- file
  dots$type <- "mwcb"
  do.call(read_ITCH, dots)
}

#' @rdname read_functions
#' @export
#' @details 
#' \itemize{
#'  \item{\code{read_ipo}: Reads messages regarding IPOs. Message type \code{K}}
#' }
#' @examples 
#' 
#' ## read_ipo()
#' read_ipo(file)
#' read_ipo(file, add_descriptions = TRUE)
read_ipo <- function(file, ..., add_descriptions = FALSE) {
  dots <- list(...)
  dots$file <- file
  dots$type <- "ipo"
  res <- do.call(read_ITCH, dots)
  
  if (add_descriptions) {
    names_ <- names(res)
    desc <- data.table::data.table(
      release_qualifier = c("A", "C"),
      release_qualifier_note = c(
        "Anticipated Quotation Release Time: This value would be used when Nasdaq Market Operations initially enters the IPO instrument for release",
        "IPO Release Canceled/Postponed: This value would be sued when Nasdaq Market Operations cancels or postpones the release of the new IPO instrument"
      )
    )
    res <- merge(res, desc, by = "release_qualifier", all.x = TRUE)
      
    data.table::setcolorder(res, names_)
  }
  
  return(res)
}

#' @rdname read_functions
#' @export
#' @details 
#' \itemize{
#'  \item{\code{read_luld}: Reads messages regarding LULDs (limit up-limit down)
#'    auction collars. Message type \code{J}}
#' }
#' @examples 
#' 
#' ## read_luld()
#' read_luld(file)
read_luld <- function(file, ...) {
  dots <- list(...)
  dots$file <- file
  dots$type <- "luld"
  do.call(read_ITCH, dots)
}

#' @rdname read_functions
#' @export
#' @details 
#' \itemize{
#'  \item{\code{read_noii}: Reads Net Order Imbalance Indicatio (NOII) messages. 
#'    Message type \code{I}}
#' }
#' @examples 
#' 
#' ## read_noii()
#' read_noii(file)
#' read_noii(file, add_descriptions = TRUE)
read_noii <- function(file, ..., add_descriptions = FALSE) {
  dots <- list(...)
  dots$file <- file
  dots$type <- "noii"
  res <- do.call(read_ITCH, dots)
  
  if (add_descriptions) {
    names_ <- names(res)
    desc <- data.table::data.table(
      imbalance_direction = c("B", "S", "N", "O"),
      imbalance_direction_note = c("Buy Imbalance", "Sell Imbalance", "No Imbalance", "Insufficient Orders to Calculate")
    )
    res <- merge(res, desc, by = "imbalance_direction", all.x = TRUE)
    
    desc <- data.table::data.table(
      cross_type = c("O", "C", "H"),
      cross_type_note = c("Nasdaq Opening Cross", "Nasdaq Closing Cross",
                          "Cross for IPO and halted/paused securities")
    )
    res <- merge(res, desc, by = "cross_type", all.x = TRUE)
    
    ll <- list(
      "L" = "Less than 1%",
      "1" = "1 to 1.99%",
      "2" = "2 to 2.99%",
      "2" = "2 to 2.99%",
      "3" = "3 to 3.99%",
      "4" = "4 to 4.99%",
      "5" = "5 to 5.99%",
      "6" = "6 to 6.99%",
      "7" = "7 to 7.99%",
      "8" = "8 to 8.99%",
      "9" = "9 to 9.99%",
      "A" = "10 to 19.99%",
      "B" = "20 to 29.99%",
      "C" = "30% or greater",
      " " = "Cannot be calculated"
    )
    
    desc <- data.table::data.table(
      variation_indicator = names(ll),
      variation_indicator_note = as.character(ll)
    )
    res <- merge(res, desc, by = "variation_indicator", all.x = TRUE)
    
    data.table::setcolorder(res, names_)
  }
  
  return(res)
}

#' @rdname read_functions
#' @export
#' @details 
#' \itemize{
#'  \item{\code{read_rpii}: Reads Retail Price Improvement Indicator (RPII) 
#'    messages. Message type \code{N}}
#' }
#' @examples 
#' 
#' ## read_rpii()
#' read_rpii(file)
#' read_rpii(file, add_descriptions = TRUE)
read_rpii <- function(file, ..., add_descriptions = FALSE) {
  dots <- list(...)
  dots$file <- file
  dots$type <- "rpii"
  res <- do.call(read_ITCH, dots)
  
  if (add_descriptions) {
    names_ <- names(res)
    desc <- data.table::data.table(
      interest_flag = c("B", "S", "A", "N"),
      interest_flag_note = c(
        "RPI orders available on the buy side",
        "RPI orders available on the sell side",
        "RPI orders available on both sides (buy and sell)",
        "No RPI orders available"
      )
    )
    res <- merge(res, desc, by = "interest_flag", all.x = TRUE)
    
    data.table::setcolorder(res, names_)
  }
  
  return(res)
}

# For backwards compatibility only...

#' @rdname read_functions
#' @export
#' @details 
#' For backwards compatability reasons, the following functions are provided as
#' well:
#' \itemize{\item{\code{get_orders}: Redirects to \code{read_orders}}}
#' @export
get_orders <- read_orders

#' @rdname read_functions
#' @export
#' @details 
#' \itemize{\item{\code{get_trades}: Redirects to \code{read_trades}}}
#' @export
get_trades <- read_trades

#' @rdname read_functions
#' @export
#' @details 
#' \itemize{\item{\code{get_modifications}: Redirects to \code{read_modifications}}}
get_modifications <- read_modifications
