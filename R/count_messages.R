#' Counts the messages of an ITCH-file
#'
#' @param file the path to the input file, either a gz-file or a plain-text file
#' @param add_meta_data if the meta-data of the messages should be added, defaults to FALSE
#' @param buffer_size the size of the buffer in bytes, defaults to 1e8 (100 MB), if you have a large amount of RAM, 1e9 (1GB) might be faster 
#' @param quiet if TRUE, the status messages are supressed, defaults to FALSE
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
count_messages <- function(file, add_meta_data = FALSE, buffer_size = 1e8, quiet = FALSE) {
  
  t0 <- Sys.time()
  # ADD GZ-possibility!
  # ADD VERBOSITY!
  # 
  if (!file.exists(file)) stop("File not found!")
  if (buffer_size < 50) stop("buffer_size has to be at least 50 bytes, otherwise the messages won't fit")
  if (buffer_size > 1e9) warning("You are trying to allocate a large array on the heap, if the function crashes, try to use a smaller buffer_size")
  
  if (grepl("\\.gz$", file)) {
    if (!quiet) cat(sprintf("[Extracting] from %s\n", file))
    
    tmp_file <- "__tmp_gzip_extract__"
    if (file.exists(tmp_file)) unlink(tmp_file)
    R.utils::gunzip(filename = file, destname = tmp_file, remove = F)
    file <- tmp_file
  }

  df <- getMessageCountDF(file, buffer_size, quiet)
  setDT(df)

  if (file.exists("__tmp_gzip_extract__")) unlink("__tmp_gzip_extract__")
  
  if (add_meta_data) df <- df[RITCH::get_meta_data(), on = "msg_type"]
  
  diff_secs <- as.numeric(difftime(Sys.time(), t0, units = "secs"))
  if (!quiet) cat(sprintf("[Done]       in %.2f secs\n", diff_secs))
  return(df)
}
