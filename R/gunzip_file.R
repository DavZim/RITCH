#' gunzips a binary file to a destination
#'
#' @param infile the gzipped binary file
#' @param outfile a raw file
#' @param buffer_size the size of the buffer to read in at once, default is 4 times the file.size (max 2Gb).
#'
#' @return The filename of the unzipped file
#' @export
#'
#' @examples
#' \dontrun{
#'   file <- system.file("extdata", "ex20101224.TEST_ITCH_50.gz", package = "RITCH")
#'   gunzip_file(file, "tmp")
#' }
gunzip_file <- function(infile, outfile = gsub("\\.gz$", "", infile),
                        buffer_size = min(4 * file.size(infile), 2e9)) {
  stopifnot(file.exists(infile))
  if (file.exists(outfile)) unlink(outfile)
  
  gunzipFile_impl(infile, outfile, buffer_size)
  return(outfile)
}

check_and_gunzip <- function(file, buffer_size, force_gunzip, quiet) {
  
  file <- path.expand(file)
  if (!grepl("\\.gz$", file)) return(file)
  
  raw_file <- gsub("\\.gz$", "", file)
  # check if the raw-file already exists, if so use this (unless force_gunzip = TRUE)
  if (file.exists(raw_file) && !quiet && !force_gunzip)
    cat(sprintf("[INFO] Unzipped file %s already found, using that (overwrite with force_gunzip=TRUE)\n", raw_file))
  
  # if the unzipped file doesnt exist or the force_gunzip flag is set, unzip file
  if (!file.exists(raw_file) || force_gunzip) {
    unlink(raw_file)
    if (!quiet) cat(sprintf("[Decompressing] %s\n", file))
    
    gunzip_file(file, raw_file, buffer_size)
  }
  return(raw_file)
}
