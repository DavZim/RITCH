#' Writes a data.frame or a list of data.frames of ITCH messages to file
#'
#' Note that additional information, e.g., columns that were added, will be
#' dropped in the process and only ITCH-compliant information is saved.
#'
#' Note that the ITCH filename contains the information for the date and exchange.
#' This can be specified explicitly in the file argument or it is added if not
#' turned off `add_meta = FALSE`.
#'
#' @param ll a data.frame or a list of data.frames of ITCH messages, in the format
#'  that the [read_functions()] return
#' @param file the filename of the target file. If the folder to the file does
#'   not exist, it will be created recursively
#' @param add_meta if date and file information should be added to the filename.
#'   Default value is TRUE. Note that adding meta information changes the filename.
#' @param append if the information should be appended to the file. Default value
#'   is FALSE
#' @param compress if the file should be gzipped. Default value is FALSE.
#'   Note that if you compress a file, buffer_size matters a lot, with larger
#'   buffers you are more likely to get smaller filesizes in the end.
#'   Alternatively, but slower, is to write the file without compression fully
#'   and then gzip the file using another program.
#' @param buffer_size the maximum buffer size. Default value is 1e8 (100MB).
#'   Accepted values are > 52 and < 5e9
#' @param quiet if TRUE, the status messages are suppressed, defaults to FALSE
#' @param append_warning if append is set, a warning about timestamp ordering is
#'  given. Set `append_warning = FALSE` to silence the warning. Default
#'  value is TRUE
#'
#' @return the filename (invisibly)
#' @export
#'
#' @examples
#' infile <- system.file("extdata", "ex20101224.TEST_ITCH_50", package = "RITCH")
#' sys <- read_system_events(infile, quiet = TRUE)
#' outfile <- tempfile()
#' write_itch(sys, outfile)
#'
#' # create a list of events, stock directory, and orders and write to a file
#' sdir <- read_stock_directory(infile, quiet = TRUE)
#' od   <- read_orders(infile, quiet = TRUE)
#'
#' ll <- list(sys, sdir, od)
#' write_itch(ll, outfile)
write_itch <- function(ll, file, add_meta = TRUE,
                       append = FALSE, compress = FALSE,
                       buffer_size = 1e8, quiet = FALSE,
                       append_warning = TRUE) {

  t0 <- Sys.time()
  if (is.data.frame(ll)) ll <- list(ll)

  if (add_meta) {
    exchange <- NA
    date <- NA

    has_exchange <- sapply(ll, function(x) "exchange" %in% names(x))
    has_name <- sapply(ll, function(x) "date" %in% names(x))

    if (any(has_exchange) && any(has_name)) {
      idx <- seq_along(ll)[has_exchange][1]
      exchange <- ll[[idx]]$exchange[1]

      idx <- seq_along(ll)[has_name][1]
      date <- ll[[idx]]$date[1]
    } else {
      warning("add_meta = TRUE but no exchange or date variable found in ll")
    }

    file <- add_meta_to_filename(file, date, exchange)
  }

  if (append && append_warning)
    warning(paste("ITCH files are sorted by timestamp, by appending to an",
                  "existing file, this is likely not guaranteed!"))

  # check that all lls are about correct
  chk <- sapply(ll, function(x)
    is.data.frame(x) &&
      all(c("msg_type", "stock_locate", "tracking_number", "timestamp") %in% names(x)))
  if (!all(chk))
    stop("All elements in ll need to be a data.frame of ITCH messages")

  ll <- lapply(ll, data.table::setorder, timestamp)

  # check and correct filename .gz ending...
  if (compress && !substr(file, nchar(file) - 2, nchar(file)) == ".gz")
    file <- paste0(file, ".gz")

  # check that the file-folder exists
  folder <- gsub("[/\\][^/\\]+$", "", file)
  if (folder != file && !dir.exists(folder))
    dir.create(folder, recursive = TRUE)

  bytes <- write_itch_impl(ll, file, append = append, gz = compress,
                           max_buffer_size = buffer_size, quiet = quiet)

  if (!quiet) cat(sprintf("[Outfile]    '%s'\n", file))

  report_end(t0, quiet, file)

  return(invisible(file))
}
