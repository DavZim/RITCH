#' Retrieves the modifications of an ITCH-file
#'
#' If the file is too large to be loaded into the file at once,
#' you can specify different start_msg_count/end_msg_counts to load only some messages.
#' 
#' @param file the path to the input file, either a gz-file or a plain-text file
#' @param buffer_size the size of the buffer in bytes, defaults to 1e8 (100 MB), 
#' if you have a large amount of RAM, 1e9 (1GB) might be faster 
#' @param start_msg_count the start count of the messages, defaults to 0, or a data.frame of msg_types and counts, as outputted by count_messages()
#' @param end_msg_count the end count of the messages, defaults to all messages
#' @param quiet if TRUE, the status messages are supressed, defaults to FALSE
#' @param force_gunzip only applies if file is a gz-file and a file with the same (gunzipped) name already exists.
#'        if set to TRUE, the existing file is overwritten. Default value is FALSE
#' @param force_cleanup only applies if file is a gz-file. If force_cleanup=TRUE, the gunzipped raw file will be deleted afterwards.
#'
#' @return a data.table containing the order modifications
#' @export
#'
#' @examples
#' \dontrun{
#'   raw_file <- "20170130.PSX_ITCH_50"
#'   get_modifications(raw_file)
#'   get_modifications(raw_file, quiet = TRUE)
#' 
#'   # load only the message 20, 21, 22 (index starts at 1)
#'   get_modifications(raw_file, startMsgCount = 20, endMsgCount = 22)
#' }
#' 
#' \dontrun{
#'   gz_file <- "20170130.PSX_ITCH_50.gz"
#'   get_modifications(gz_file)
#'   get_modifications(gz_file, quiet = T)
#'   
#'   msg_count <- count_messages(raw_file)
#'   get_modifications(raw_file, msg_count)
#' }
get_modifications <- function(file, start_msg_count = 0, end_msg_count = -1, 
                              buffer_size = -1, quiet = FALSE,
                              force_gunzip = FALSE, force_cleanup = FALSE) {

  msg_types <- c("E", "C", "X", "D", "U")
  ll <- check_inputs(file, msg_types, buffer_size, start_msg_count, end_msg_count, force_gunzip, quiet)
  
  # -1 because we want it 1 indexed (cpp is 0-indexed) 
  # and max(0, xxx) b.c. the variable is unsigned!
  df <- getModifications_impl(ll$file, max(ll$start_msg_count - 1, 0),
                              max(ll$end_msg_count - 1, -1), ll$buffer_size, quiet)

  # if the file was gzipped and the force_cleanup=TRUE, delete unzipped file 
  if (grepl("\\.gz$", ll$orig_file) && force_cleanup) unlink(gsub("\\.gz", "", ll$file))
  
  if (!quiet) cat("\n[Converting] to data.table\n")
  
  df <- data.table::setalloccol(df)
  
  # add the date
  df[, date := ll$filedate]
  df[, datetime := nanotime(as.Date(ll$filedate)) + timestamp]
  
  df[, exchange := get_exchange_from_filename(ll$file)]

  a <- gc()
  
  diff_secs <- as.numeric(difftime(Sys.time(), ll$t0, units = "secs"))
  if (!quiet) cat(sprintf("[Done]       in %.2f secs\n", diff_secs))
  
  return(df[])
}
