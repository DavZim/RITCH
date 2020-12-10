#' @name read_functions
#' @rdname read_functions
#'
#' @title Reads certain messages of an ITCH-file into a data.table
#' 
#' @description  
#' The read functions consist of \code{read_trades}, \code{read_orders}, and \code{read_modifications}.
#' 
#' TODO: Document here the different functions, which messages they read and what they imply...
#' 
#' If the file is too large to be loaded into the workspace at once,
#' you can specify different start_msg_count/end_msg_counts to load only some messages.
#' 
#' @details 
#' The specifications of the different messages types can be obtained from the official
#' ITCH specification (see also \code{\link{open_itch_specification}})
#' 
#' @param file the path to the input file, either a gz-file or a plain-text file
#' @param type the type to load, can be "orders", "trades", "modifications", ... Only applies to the read_ITCH() function.
#' @param buffer_size the size of the buffer in bytes, defaults to 1e8 (100 MB), 
#' if you have a large amount of RAM, 1e9 (1GB) might be faster 
#' @param start_msg_count the start count of the messages, defaults to 0, or a data.frame of msg_types and counts, as returned by \code{\link{count_messages}}
#' @param end_msg_count the end count of the messages, defaults to all messages
#' @param quiet if TRUE, the status messages are suppressed, defaults to FALSE
#' @param force_gunzip only applies if file is a gz-file and a file with the same (gunzipped) name already exists.
#'        if set to TRUE, the existing file is overwritten. Default value is FALSE
#' @param force_cleanup only applies if file is a gz-file. If force_cleanup=TRUE, the gunzipped raw file will be deleted afterwards.
#' @param ... Additional arguments passed to \code{read_ITCH}
#' @param add_descriptions add longer descriptions to shortened variables.
#' The added information is taken from the official ITCH documentation
#' see also \code{\link{open_itch_specification}}
#'  
#' @references \url{https://www.nasdaqtrader.com/content/technicalsupport/specifications/dataproducts/NQTVITCHspecification.pdf}
#' 
#' @return a data.table containing the messages
#'
#' @examples
#' file <- "20191230.BX_ITCH_50"
#' od <- get_orders(file)
#' tr <- get_trades(file)
#' md <- get_modifications(file)
#' 
#' ## Alternatively 
#' od <- read_ITCH(file, "orders")
#' 
#' str(od)
#' str(tr)
#' str(md)
#' 
#' \dontrun{
#'   raw_file <- "20170130.PSX_ITCH_50"
#'   get_orders(raw_file)
#'   # turn off the feedback from the reading process
#'   get_orders(raw_file, quiet = TRUE)
#' 
#'   # load only the message 20, 21, 22 (index starts at 1)
#'   get_orders(raw_file, startMsgCount = 20, endMsgCount = 22)
#' }
#' 
#' \dontrun{
#'   # .gz files will be automatically unzipped
#'   gz_file <- "20170130.PSX_ITCH_50.gz"
#'   get_orders(gz_file)
#'   get_orders(gz_file, quiet = TRUE)
#'   
#'   # a message count can be provided for slightly faster reads
#'   msg_count <- count_messages(raw_file)
#'   get_orders(raw_file, msg_count)
#' }
#' 
NULL

#' @rdname read_functions
#' @export
read_ITCH <- function(file, type, start_msg_count = 0, end_msg_count = -1, 
                      buffer_size = -1, quiet = FALSE,
                      force_gunzip = FALSE, force_cleanup = FALSE) {
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
  
  t0 <- Sys.time()
  if (!file.exists(file)) stop("File not found!")
  
  # Set the default value of the buffer size
  if (buffer_size < 0)
    buffer_size <- ifelse(grepl("\\.gz$", file), 
                          min(3 * file.size(file), 1e9), 
                          1e8)
  
  if (buffer_size < 50) stop("buffer_size has to be at least 50 bytes, otherwise the messages won't fit")
  if (buffer_size > 5e9) warning("You are trying to allocate a large array on the heap, if the function crashes, try to use a smaller buffer_size")
  
  filedate <- get_date_from_filename(file)
  
  if (is.data.frame(start_msg_count)) {
    if (!all(c("msg_type", "count") %in% names(start_msg_count))) 
      stop("If start_msg_count is a data.frame/table, it must contain 'msg_type' and 'count'!")
    dd <- start_msg_count
    start_msg_count <- 1
    end_msg_count <- as.integer(dd[msg_type %in% msg_types, sum(count)])
  }
  
  orig_file <- file
  file <- check_and_gunzip(file, buffer_size, force_gunzip, quiet)
  
  # -1 because we want it 1 indexed (cpp is 0-indexed) 
  # and max(0, xxx) b.c. the variable is unsigned!
  start <- max(start_msg_count - 1, 0)
  end <- max(end_msg_count - 1, -1)
  df <- imp_calls[[type]](file, start, end, buffer_size, quiet)
  
  # if the file was gzipped and the force_cleanup=TRUE, delete unzipped file 
  if (grepl("\\.gz$", orig_file) && force_cleanup) unlink(gsub("\\.gz", "", file))
  
  if (!quiet) cat("\n[Converting] to data.table\n")
  df <- data.table::setalloccol(df)
  
  # add the date and exchange
  df[, date := filedate]
  df[, datetime := nanotime(as.Date(filedate)) + timestamp]
  df[, exchange := get_exchange_from_filename(file)]
  
  a <- gc()
  
  diff_secs <- as.numeric(difftime(Sys.time(), t0, units = "secs"))
  if (!quiet) cat(sprintf("[Done]       in %.2f secs\n", diff_secs))
  
  return(df[])
}

#' @rdname read_functions
#' @export
#' @details
#' \itemize{
#'  \item{\code{read_orders()}}{ Order messages refer to message type 'A' and 'F'}
#' }
#' @examples 
#' 
#' ## read_orders()
#' file <- "20191230.BX_ITCH_50"
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
#'  \item{\code{read_trades()}}{ Trade messages refer to message type 'P', 'Q', and 'B'}
#' }
#' @examples 
#' 
#' ## read_trades()
#' file <- "20191230.BX_ITCH_50"
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
#'  \item{\code{read_modifications()}}{ Modification messages refer to message type 'E', 'C', 'X', 'D', and 'U'}
#' }
#' @examples 
#' 
#' ## read_modifications()
#' file <- "20191230.BX_ITCH_50"
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
#'  \item{\code{read_system_events()}}{ System event messages refer to message type 'S'}
#' }
#' @examples 
#' 
#' ## read_system_events()
#' file <- "20191230.BX_ITCH_50"
#' read_system_events(file)
#' read_system_events(file, add_descriptions)
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
#'  \item{\code{read_stock_directory()}}{ Stock directory messages refer to message type 'R'}
#' }
#' @examples 
#' 
#' ## read_stock_directory()
#' file <- "20191230.BX_ITCH_50"
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
#'  \item{\code{read_trading_status()}}{ Trading Status messages refer to message type 'H' and 'h' (operational reason)}
#' }
#' @examples 
#' 
#' ## read_trading_status()
#' file <- "20191230.BX_ITCH_50"
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
#'  \item{\code{read_reg_sho()}}{ Reg SHO Status messages refer to message type 'Y'}
#' }
#' @examples 
#' 
#' ## read_reg_sho()
#' file <- "20191230.BX_ITCH_50"
#' read_reg_sho(file)
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
#'  \item{\code{read_market_participant_states()}}{ Market participants status messages refer to message type 'L'}
#' }
#' @examples 
#' 
#' ## read_market_participant_states()
#' file <- "20191230.BX_ITCH_50"
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
#'  \item{\code{read_mwcb()}}{ MWCB (Market Wide Circuit Breaker) messages refer to message types 'V' and 'W'}
#' }
#' @examples 
#' 
#' ## read_mwcb()
#' file <- "20191230.BX_ITCH_50"
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
#'  \item{\code{read_ipo()}}{ IPO messages refer to message type 'K'}
#' }
#' @examples 
#' 
#' ## read_ipo()
#' file <- "20191230.BX_ITCH_50"
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
#'  \item{\code{read_luld()}}{ LULD messages refer to message type 'J'}
#' }
#' @examples 
#' 
#' ## read_luld()
#' file <- "20191230.BX_ITCH_50"
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
#'  \item{\code{read_noii()}}{ NOII messages refer to message type 'I'}
#' }
#' @examples 
#' 
#' ## read_noii()
#' file <- "20191230.BX_ITCH_50"
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
#'  \item{\code{read_rpii()}}{ RPII messages refer to message type 'N'}
#' }
#' @examples 
#' 
#' ## read_rpii()
#' file <- "20191230.BX_ITCH_50"
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
#' @export
get_modifications <- read_modifications
#' @export
get_trades <- read_trades
#' @export
get_orders <- read_orders