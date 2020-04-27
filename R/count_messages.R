#' Counts the messages of an ITCH-file
#'
#' @param file the path to the input file, either a gz-file or a plain-text file
#' @param add_meta_data if the meta-data of the messages should be added, defaults to FALSE
#' @param buffer_size the size of the buffer in bytes, defaults to 1e8 (100 MB), if you have a large amount of RAM, 1e9 (1GB) might be faster 
#' @param quiet if TRUE, the status messages are supressed, defaults to FALSE
#' @param force_gunzip only applies if file is a gz-file and a file with the same (gunzipped) name already exists.
#'        if set to TRUE, the existing file is overwritten. Default value is FALSE
#' @param force_cleanup only applies if file is a gz-file. If force_cleanup=TRUE, the gunzipped raw file will be deleted afterwards.
#'
#' @return a data.table containing the message-type and their counts
#' @export
#'
#' @examples
#' \dontrun{
#'   raw_file <- "20170130.PSX_ITCH_50"
#'   count_messages(raw_file)
#' 
#'   count_messages(raw_file, TRUE)
#' }
#' 
#' \dontrun{
#'   gz_file <- "20170130.PSX_ITCH_50.gz"
#'   count_messages(gz_file)
#'   count_messages(gz_file, TRUE)
#' }
count_messages <- function(file, add_meta_data = FALSE, buffer_size = -1, quiet = FALSE,
                           force_gunzip = FALSE, force_cleanup = FALSE) {
  
  t0 <- Sys.time()
  
  if (!file.exists(file)) stop("File not found!")
  
  # Set the default value of the buffer size
  if (buffer_size < 0)
    buffer_size <- ifelse(grepl("\\.gz$", file), max(3 * file.size(file), 1e9), 1e8)
  
  if (buffer_size < 50) stop("buffer_size has to be at least 50 bytes, otherwise the messages won't fit")
  if (buffer_size > 5e9) warning("You are trying to allocate a large array on the heap, if the function crashes, try to use a smaller buffer_size")

  orig_file <- file
  file <- check_and_gunzip(file, buffer_size, force_gunzip, quiet)
  
  df <- getMessageCountDF(file, buffer_size, quiet)
  df <- data.table::setalloccol(df)
  
  if (grepl("\\.gz$", orig_file) && force_cleanup) unlink(gsub("\\.gz", "", file))

  if (add_meta_data) df <- df[RITCH::get_meta_data(), on = "msg_type"]
  
  diff_secs <- as.numeric(difftime(Sys.time(), t0, units = "secs"))
  if (!quiet) cat(sprintf("[Done]       in %.2f secs\n", diff_secs))
  return(df)
}
