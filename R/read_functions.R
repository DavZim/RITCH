
#' @name read_functions
#' @rdname read_functions
#' @title Reads certain messages of an ITCH-file into a data.table
#'
#' @description
#'
#' For faster file-reads (at the tradeoff of increased memory usages), you can
#' increase the `buffer_size` to 1GB (1e9) or more.
#'
#' If you access the same file multiple times, you can provide the message
#' counts as outputted from [count_messages()] to the `n_max`
#' argument, this allows skipping one pass over the file per read instruction.
#'
#' If you need to read in multiple message classes, you can specify multiple
#' message classes to `read_itch`, which results in only a single file pass.
#'
#' If the file is too large to be loaded into the workspace at once, you can
#' specify different `skip` and `n_max` to load only
#' a specific range of messages.
#' Alternatively, you can filter certain messages to another file using
#' [filter_itch()], which is substantially faster than parsing a file
#' and filtering it.
#'
#' Note that all read functions allow both plain ITCH files as well as gzipped
#' files.
#' If a gzipped file is found, it will look for a plain ITCH file with
#' the same name and use that instead.
#' If this file is not found, it will be created by unzipping the archive.
#' Note that the unzipped file is NOT deleted by default (the file will be
#' created in the current working directory).
#' It might result in increased disk usage but reduces future read times for
#' that specific file.
#' To force RITCH to delete "temporary" files after uncompressing, use
#' `force_cleanup = TRUE` (only deletes the files if they were extracted
#' before, does not remove the archive itself).
#'
#' @param file the path to the input file, either a gz-archive or a plain ITCH file
#' @param filter_msg_class a vector of classes to load, can be "orders", "trades",
#'   "modifications", ... see also [get_msg_classes()].
#'   Default value is to take all message classes.
#' @param skip Number of messages to skip before starting parsing messages,
#' note the skip parameter applies to the specific message class, i.e., it would
#' skip the messages for each type (e.g., skip the first 10 messages for each class).
#' @param n_max Maximum number of messages to parse, default is to read all values.
#'  Can also be a data.frame of msg_types and counts, as returned by
#'  [count_messages()].
#'  Note the n_max parameter applies to the specific message class not the whole
#'  file.
#' @param filter_msg_type a character vector, specifying a filter for message types.
#'  Note that this can be used to only return 'A' orders for instance.
#' @param filter_stock_locate an integer vector, specifying a filter for locate codes.
#'  The locate codes can be looked up by calling [read_stock_directory()]
#'  or by downloading from NASDAQ by using [download_stock_directory()].
#'  Note that some message types (e.g., system events, MWCB, and IPO) do not use
#'  a locate code.
#' @param min_timestamp an 64 bit integer vector (see also [bit64::as.integer64()])
#'  of minimum timestamp (inclusive).
#'  Note: min and max timestamp must be supplied with the same length or left empty.
#' @param max_timestamp an 64 bit integer vector (see also [bit64::as.integer64()])
#'  of maxium timestamp (inclusive).
#'  Note: min and max timestamp must be supplied with the same length or left empty.
#' @param filter_stock a character vector, specifying a filter for stocks.
#'  Note that this a shorthand for the `filter_stock_locate` argument, as it
#'  tries to find the stock_locate based on the `stock_directory` argument,
#'  if this is not found, it will try to extract the stock directory from the file,
#'  else an error is thrown.
#' @param stock_directory A data.frame containing the stock-locate code relationship.
#' As outputted by [read_stock_directory()].
#' Only used if `filter_stock` is set. To download the stock directory from
#' NASDAQs server, use [download_stock_directory()].
#' @param buffer_size the size of the buffer in bytes, defaults to 1e8 (100 MB),
#' if you have a large amount of RAM, 1e9 (1GB) might be faster
#' @param quiet if TRUE, the status messages are suppressed, defaults to FALSE
#' @param add_meta if TRUE, the date and exchange information of the file are added, defaults to TRUE
#' @param force_gunzip only applies if the input file is a gz-archive and a file with the same (gunzipped) name already exists.
#'        if set to TRUE, the existing file is overwritten. Default value is FALSE
#' @param gz_dir a directory where the gz archive is extracted to.
#'        Only applies if file is a gz archive. Default is [tempdir()].        
#' @param force_cleanup only applies if the input file is a gz-archive.
#'   If force_cleanup=TRUE, the gunzipped raw file will be deleted afterwards.
#'   Only applies when the gunzipped raw file did not exist before.
#' @param ... Additional arguments passed to `read_itch`
#' @param add_descriptions add longer descriptions to shortened variables.
#' The added information is taken from the official ITCH documentation
#' see also [open_itch_specification()]
#'
#' @details
#' The details of the different messages types can be found in the official
#' ITCH specification (see also [open_itch_specification()])
#'
#' @references <https://www.nasdaqtrader.com/content/technicalsupport/specifications/dataproducts/NQTVITCHspecification.pdf>
#'
#' @return a data.table containing the messages
#'
#' @examples
#' \dontshow{
#' data.table::setDTthreads(2)
#' }
#' file <- system.file("extdata", "ex20101224.TEST_ITCH_50", package = "RITCH")
#' od <- read_orders(file, quiet = FALSE) # note quiet = FALSE is the default
#' tr <- read_trades(file, quiet = TRUE)
#'
#' ## Alternatively
#' od <- read_itch(file, "orders", quiet = TRUE)
#'
#' ll <- read_itch(file, c("orders", "trades"), quiet = TRUE)
#'
#' od
#' tr
#' str(ll, max.level = 1)
#'
#' ## additional options:
#'
#' # take only subset of messages
#' od <- read_orders(file, skip = 3, n_max = 10)
#'
#' # a message count can be provided for slightly faster reads
#' msg_count <- count_messages(file, quiet = TRUE)
#' od <- read_orders(file, n_max = msg_count)
#'
#' ## .gz archive functionality
#' # .gz archives will be automatically unzipped
#' gz_file <- system.file("extdata", "ex20101224.TEST_ITCH_50.gz", package = "RITCH")
#' od <- read_orders(gz_file)
#' # force a decompress and delete the decompressed file afterwards
#' od <- read_orders(gz_file, force_gunzip = TRUE, force_cleanup = TRUE)
NULL

#' @rdname read_functions
#' @export
#' @details
#' - `read_itch`: Reads a message class message, can also read multiple
#'  classes in one file-pass.
#'
#' @examples
#'
#' ## read_itch()
#' otm <- read_itch(file, c("orders", "trades"), quiet = TRUE)
#' str(otm, max.level = 1)
read_itch <- function(file, filter_msg_class = NA,
                      skip = 0, n_max = -1,
                      filter_msg_type = NA_character_,
                      filter_stock_locate = NA_integer_,
                      min_timestamp = bit64::as.integer64(NA),
                      max_timestamp = bit64::as.integer64(NA),
                      filter_stock = NA_character_, stock_directory = NA,
                      buffer_size = -1, quiet = FALSE, add_meta = TRUE,
                      force_gunzip = FALSE, gz_dir = tempdir(), force_cleanup = TRUE) {
  t0 <- Sys.time()
  if (!file.exists(file))
    stop(sprintf("File '%s' not found!", file))

  msg_classes <- list(
    "system_events" = "S",
    "stock_directory" = "R",
    "trading_status" = c("H", "h"),
    "reg_sho" = "Y",
    "market_participant_states" = "L",
    "mwcb" = c("V", "W"),
    "ipo" = "K",
    "luld" = "J",
    "orders" = c("A", "F"),
    "modifications" = c("E", "C", "X", "D", "U"),
    "trades" = c("P", "Q", "B"),
    "noii" = "I",
    "rpii" = "N"
  )
  if (length(filter_msg_class) == 1 && is.na(filter_msg_class))
    filter_msg_class <- names(msg_classes)

  if (!all(filter_msg_class %in% names(msg_classes)))
    stop("Invalid filter_msg_class detected")

  # treat n_max
  n_max_is_dataframe <- is.data.frame(n_max)
  if (n_max_is_dataframe) {
    if (!all(c("msg_type", "count") %in% names(n_max)))
      stop("If n_max is a data.frame/table, it must contain 'msg_type' and 'count'!")
    skip <- 0
    n_max <- as.integer(n_max[msg_type %in% msg_classes[[filter_msg_class]],
                              sum(count)])
  }
  # +1 as we want to skip
  start <- max(skip, 0)
  end <- max(skip + n_max - 1, -1)
  if (end < start) end <- -1

  if (is.numeric(n_max) && n_max != -1 && !quiet && !n_max_is_dataframe)
    cat("[Note]       n_max overrides counting the messages. Number of messages may be off\n")

  if (!quiet && (start != 0 | end >= 0))
    cat(sprintf("[Filter]     skip: %i n_max: %i (%i - %i)\n",
                skip, n_max, start + 1, end + 1))

  # Treat filters
  # Message types
  filter_msg_type <- check_msg_types(filter_msg_type, quiet)

  # locate code
  filter_stock_locate <- filter_stock_locate[!is.na(filter_stock_locate)]
  filter_stock_locate <- as.integer(filter_stock_locate)

  # Timestamp
  t <- check_timestamps(min_timestamp, max_timestamp, quiet)
  min_timestamp <- t$min
  max_timestamp <- t$max

  # Stock
  filter_stock_locate <- check_stock_filters(filter_stock, stock_directory,
                                             filter_stock_locate, file)

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
  buffer_size <- check_buffer_size(buffer_size, file)

  filedate <- get_date_from_filename(file)

  orig_file <- file
  # only needed for gz files; gz files are not deleted when the raw file already existed
  raw_file_existed <- file.exists(gsub("\\.gz$", "", file))
  file <- check_and_gunzip(file, gz_dir, buffer_size, force_gunzip, quiet)

  res_raw <- read_itch_impl(filter_msg_class, file, start, end,
                            filter_msg_type, filter_stock_locate,
                            min_timestamp, max_timestamp,
                            buffer_size, quiet)

  if (!quiet) cat("[Converting] to data.table\n")

  res <- lapply(res_raw, data.table::setalloccol)

  if (add_meta) {
    # add the date and exchange
    res <- lapply(res, function(df) {
      dtime <- nanotime::nanotime(NULL)
      if (nrow(df) > 0)
        dtime <- nanotime::nanotime(as.Date(filedate)) + df$timestamp

      df[, ':=' (
        date = filedate,
        datetime = dtime,
        exchange = get_exchange_from_filename(file)
      )]
    })
  }

  # remove messages with empty msg_types, this can be the case if n_max was set
  # to a large value
  res <- lapply(res, function(df) df[msg_type != ""])

  # if the res list has only one element, unlist on one level!

  if (length(res) == 1) {
    res <- res[[1]]
  } else {
    # take only messages with nrow > 0
    res <- res[sapply(res, nrow) > 0]

    if (length(res) == 0 && !quiet)
      warning("No messages found for selected filters")
  }


  a <- gc()
  report_end(t0, quiet, orig_file)

  # if the file was gzipped and the force_cleanup=TRUE, delete unzipped file
  if (grepl("\\.gz$", orig_file) && force_cleanup && !raw_file_existed) {
    if (!quiet) cat(sprintf("[Cleanup]    Removing file '%s'\n", file))
    unlink(gsub("\\.gz$", "", file))
  }
  return(res)
}

## convenient wrapper for read functions for the different classes

#' @rdname read_functions
#' @export
#' @details
#' - `read_system_events`: Reads system event messages. Message type `S`
#'
#' @examples
#'
#' ## read_system_events()
#' se <- read_system_events(file, add_descriptions = TRUE, quiet = TRUE)
#' se
read_system_events <- function(file, ..., add_descriptions = FALSE) {
  dots <- list(...)
  dots$file <- file
  dots$filter_msg_class <- "system_events"
  res <- do.call(read_itch, dots)

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
#' - `read_stock_directory`: Reads stock trading messages. Message type `R`
#'
#' @examples
#'
#' ## read_stock_directory()
#' sd <- read_stock_directory(file, add_descriptions = TRUE, quiet = TRUE)
#' sd
read_stock_directory <- function(file, ..., add_descriptions = FALSE) {
  dots <- list(...)
  dots$file <- file
  dots$filter_msg_class <- "stock_directory"
  res <- do.call(read_itch, dots)

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
#' - `read_trading_status`: Reads trading status messages. Message type `H`
#'    and `h`
#'
#' @examples
#'
#' ## read_trading_status()
#' ts <- read_trading_status(file, add_descriptions = TRUE, quiet = TRUE)
#' ts
read_trading_status <- function(file, ..., add_descriptions = FALSE) {
  dots <- list(...)
  dots$file <- file
  dots$filter_msg_class <- "trading_status"
  res <- do.call(read_itch, dots)

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
#' - `read_reg_sho`: Reads messages regarding reg SHO. Message type `Y`
#'
#' @examples
#'
#' ## read_reg_sho()
#' \dontrun{
#' # note the example file has no reg SHO messages
#' rs <- read_reg_sho(file, add_descriptions = TRUE, quiet = TRUE)
#' rs
#' }
read_reg_sho <- function(file, ..., add_descriptions = FALSE) {
  dots <- list(...)
  dots$file <- file
  dots$filter_msg_class <- "reg_sho"
  res <- do.call(read_itch, dots)

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
#' - `read_market_participant_states`: Reads messages regarding the
#'    status of market participants. Message type `L`
#'
#' @examples
#'
#' ## read_market_participant_states()
#' \dontrun{
#' # note the example file has no market participant states
#' mps <- read_market_participant_states(file, add_descriptions = TRUE,
#'                                       quiet = TRUE)
#' mps
#' }
read_market_participant_states <- function(file, ..., add_descriptions = FALSE) {
  dots <- list(...)
  dots$file <- file
  dots$filter_msg_class <- "market_participant_states"
  res <- do.call(read_itch, dots)

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
#' - `read_mwcb`: Reads messages regarding Market-Wide-Circuit-Breakers
#'    (MWCB). Message type `V` and `W`
#'
#' @examples
#'
#' ## read_mwcb()
#' \dontrun{
#' # note the example file has no circuit breakers messages
#' mwcb <- read_mwcb(file, quiet = TRUE)
#' mwcb
#' }
read_mwcb <- function(file, ...) {
  dots <- list(...)
  dots$file <- file
  dots$filter_msg_class <- "mwcb"
  # no filter for mwcb... they are always set to 0! in the messages
  if ((length(dots$filter_stock_locate) > 0 && !is.na(dots$filter_stock_locate)) ||
      (length(dots$filter_stock) > 0 && !is.na(dots$filter_stock)))
    warning("filter_stock and filter_stock_locate not used for MWCB messages!")
  dots$filter_stock_locate <- NA_integer_
  dots$filter_stock <- NA_character_
  do.call(read_itch, dots)
}

#' @rdname read_functions
#' @export
#' @details
#' - `read_ipo`: Reads messages regarding IPOs. Message type `K`
#'
#' @examples
#'
#' ## read_ipo()
#' \dontrun{
#' # note the example file has no IPOs
#' ipo <- read_ipo(file, add_descriptions = TRUE, quiet = TRUE)
#' ipo
#' }
read_ipo <- function(file, ..., add_descriptions = FALSE) {
  dots <- list(...)
  dots$file <- file
  dots$filter_msg_class <- "ipo"
  res <- do.call(read_itch, dots)

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
#' - `read_luld`: Reads messages regarding LULDs (limit up-limit down)
#'    auction collars. Message type `J`
#'
#' @examples
#'
#' ## read_luld()
#' \dontrun{
#' # note the example file has no LULD messages
#' luld <- read_luld(file, quiet = TRUE)
#' luld
#' }
read_luld <- function(file, ...) {
  dots <- list(...)
  dots$file <- file
  dots$filter_msg_class <- "luld"
  do.call(read_itch, dots)
}

#' @rdname read_functions
#' @export
#' @details
#' - `read_orders`: Reads order messages. Message type `A` and `F`
#'
#' @examples
#'
#' ## read_orders()
#' od <- read_orders(file, quiet = TRUE)
#' od
read_orders <- function(file, ...) {
  dots <- list(...)
  dots$file <- file
  dots$filter_msg_class <- "orders"
  do.call(read_itch, dots)
}

#' @rdname read_functions
#' @export
#' @details
#' - `read_modifications`: Reads order modification messages. Message
#'    type `E`, `C`, `X`, `D`, and `U`
#'
#' @examples
#'
#' ## read_modifications()
#' mod <- read_modifications(file, quiet = TRUE)
#' mod
read_modifications <- function(file, ...) {
  dots <- list(...)
  dots$file <- file
  dots$filter_msg_class <- "modifications"
  do.call(read_itch, dots)
}

#' @rdname read_functions
#' @export
#' @details
#' - `read_trades`: Reads trade messages. Message type `P`, `Q` and `B`
#'
#' @examples
#'
#' ## read_trades()
#' tr <- read_trades(file, quiet = TRUE)
#' tr
read_trades <- function(file, ...) {
  dots <- list(...)
  dots$file <- file
  dots$filter_msg_class <- "trades"
  do.call(read_itch, dots)
}

#' @rdname read_functions
#' @export
#' @details
#' - `read_noii`: Reads Net Order Imbalance Indicatio (NOII) messages.
#'    Message type `I`
#'
#' @examples
#'
#' ## read_noii()
#' \dontrun{
#' # note the example file has no NOII messages
#' noii <- read_noii(file, add_descriptions = TRUE, quiet = TRUE)
#' noii
#' }
read_noii <- function(file, ..., add_descriptions = FALSE) {
  dots <- list(...)
  dots$file <- file
  dots$filter_msg_class <- "noii"
  res <- do.call(read_itch, dots)

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
#' - `read_rpii`: Reads Retail Price Improvement Indicator (RPII)
#'    messages. Message type `N`
#'
#' @examples
#'
#' ## read_rpii()
#' \dontrun{
#' # note the example file has no RPII messages
#' rpii <- read_rpii(file, add_descriptions = TRUE, quiet = TRUE)
#' rpii
#' }
read_rpii <- function(file, ..., add_descriptions = FALSE) {
  dots <- list(...)
  dots$file <- file
  dots$filter_msg_class <- "rpii"
  res <- do.call(read_itch, dots)

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
#'
#' - `get_orders`: Redirects to `read_orders`
#' @export
get_orders <- read_orders

#' @rdname read_functions
#' @export
#' @details
#'  - `get_trades`: Redirects to `read_trades`
#' @export
get_trades <- read_trades

#' @rdname read_functions
#' @export
#' @details
#' - `get_modifications`: Redirects to `read_modifications`
get_modifications <- read_modifications
