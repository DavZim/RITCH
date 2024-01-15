#' Filters an ITCH file to another ITCH file
#'
#' This function allows to perform very fast filter operations on large ITCH
#' files. The messages are written to another ITCH file.
#'
#' Note that this can be especially useful on larger files or where memory
#' is not large enough to filter the datalimits the analysis.
#'
#' As with the [read_itch()] functions, it allows to filter for
#' `msg_class`, `msg_type`, `stock_locate`/`stock`, and
#' `timestamp`.
#'
#' @inheritParams read_functions
#' @param infile the input file where the messages are taken from, can be a
#' gz-archive or a plain ITCH file.
#' @param outfile the output file where the filtered messages are written to.
#' Note that the date and exchange information from the `infile` are used,
#' see also [add_meta_to_filename()] for further information.
#' @param append if the messages should be appended to the outfile, default is
#' false. Note, this is helpful if `skip` and or `n_max` are used for
#' batch filtering.
#' @param gz if the output file should be gzip-compressed. Note that the name
#' of the output file will be appended with .gz if not already present. The
#' final output name is returned. Default value is false.
#' @param overwrite if an existing outfile with the same name should be
#' overwritten. Default value is false
#'
#' @return the name of the output file (maybe different from the inputted
#' outfile due to adding the date and exchange), silently
#' @export
#'
#' @examples
#' infile <- system.file("extdata", "ex20101224.TEST_ITCH_50", package = "RITCH")
#' outfile <- tempfile(fileext = "_20101224.TEST_ITCH_50")
#' filter_itch(
#'   infile, outfile,
#'   filter_msg_class = c("orders", "trades"),
#'   filter_msg_type = "R", # stock_directory
#'   skip = 0, n_max = 100
#' )
#'
#' # expecting 100 orders, 100 trades, and 3 stock_directory entries
#' count_messages(outfile)
#'
#' # check that the output file contains the same
#' res  <- read_itch(outfile, c("orders", "trades", "stock_directory"))
#' sapply(res, nrow)
#'
#' res2 <- read_itch(infile,  c("orders", "trades", "stock_directory"),
#'                   n_max = 100)
#'
#' all.equal(res, res2)
filter_itch <- function(infile, outfile,
                        filter_msg_class = NA_character_,
                        filter_msg_type = NA_character_,
                        filter_stock_locate = NA_integer_,
                        min_timestamp = bit64::as.integer64(NA),
                        max_timestamp = bit64::as.integer64(NA),
                        filter_stock = NA_character_, stock_directory = NA,
                        skip = 0, n_max = -1, append = FALSE, overwrite = FALSE,
                        gz = FALSE, buffer_size = -1, quiet = FALSE,
                        force_gunzip = FALSE, force_cleanup = TRUE) {
  t0 <- Sys.time()
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

  if (!any(is.na(filter_msg_class))) {
    filter_msg_type <- c(
      filter_msg_type,
      as.character(unlist(msg_classes[tolower(filter_msg_class)]))
    )
  }

  if (!file.exists(infile))
    stop(sprintf("File '%s' not found!", infile))

  date <- get_date_from_filename(infile)
  exch <- get_exchange_from_filename(infile)
  outfile <- add_meta_to_filename(outfile, date, exch)

  # check that the directory for outfile exists
  outfile_dir <- gsub("[^/]+$", "", outfile)
  if (outfile_dir != "" && !dir.exists(outfile_dir)) {
    if (overwrite) {
      dir.create(outfile_dir, recursive = TRUE)
    } else {
      stop(sprintf(
        "Directory '%s' not found, to create/overwrite use overwrite = TRUE",
        outfile_dir
      ))
    }
  }

  # first write to unzipped file, than gzip the file later...
  if (grepl("\\.gz$", outfile)) outfile <- gsub("\\.gz$", "", outfile)

  if (file.exists(outfile) && !append && !overwrite)
    stop(sprintf("File '%s' already found, to overwrite use overwrite = TRUE or use append = TRUE",
                 outfile))

  if (!quiet) {
    sprintf("[infile]     '%s'\n", infile)
    sprintf("[outfile]    '%s'\n", outfile)
  }

  # treat n_max
  if (is.data.frame(n_max))
    stop("n_max cannot be a data.frame in filter_itch!")

  # +1 as we want to skip, -1 as cpp is zero indexed
  start <- max(skip, 0)
  end <- max(skip + n_max - 1, -1)
  if (end < start) end <- -1

  if (!quiet && (start != 0 | end != -1))
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
                                             filter_stock_locate, infile)

  if (!quiet && length(filter_stock_locate) > 0)
    cat(paste0("[Filter]     stock_locate: '",
               paste(filter_stock_locate, collapse = "', '"),
               "'\n"))

  # Set the default value of the buffer size
  buffer_size <- check_buffer_size(buffer_size, infile)

  filedate <- get_date_from_filename(infile)

  orig_infile <- infile
  # only needed for gz files; gz files are not deleted when the raw file already existed
  raw_file_existed <- file.exists(basename(gsub("\\.gz$", "", infile)))
  infile <- check_and_gunzip(infile, dirname(outfile), buffer_size, force_gunzip, quiet)

  filter_itch_impl(infile, outfile, start, end,
                   filter_msg_type, filter_stock_locate,
                   min_timestamp, max_timestamp,
                   append, buffer_size, quiet)

  if (gz) {
    if (!quiet) cat(sprintf("[gzip]       outfile\n"))
    of <- outfile
    outfile <- gzip_file(infile = outfile,
                         outfile = paste0(outfile, ".gz"))
    unlink(of) # delete the temporary file
  }

  a <- gc()

  report_end(t0, quiet, infile)

  # if the file was gzipped and the force_cleanup=TRUE, delete unzipped file
  if (grepl("\\.gz$", orig_infile) && force_cleanup && !raw_file_existed) {
    if (!quiet) cat(sprintf("[Cleanup]    Removing file '%s'\n", infile))
    unlink(basename(gsub("\\.gz$", "", infile)))
  }

  return(invisible(outfile))
}
