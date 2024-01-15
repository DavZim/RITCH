
#' @name gz_functions
#' @rdname gz_functions
#' @title Compresses and uncompresses files to and from gz-archives
#'
#' @description
#'
#' Allows the compression and uncompression of files
#'
#' @param infile the file to be zipped or unzipped
#' @param outfile the resulting zipped or unzipped file
#' @param buffer_size the size of the buffer to read in at once, default is 4 times the file.size (max 2Gb).
#'
#' @details Functions are
#'
#' @return The filename of the unzipped file, invisibly
#'
#' @examples
#' gzfile <- system.file("extdata", "ex20101224.TEST_ITCH_50.gz", package = "RITCH")
#' file   <- system.file("extdata", "ex20101224.TEST_ITCH_50", package = "RITCH")
#'
NULL

#' @rdname gz_functions
#' @export
#' @details
#' - `gunzip_file`: uncompresses a gz-archive to raw binary data
#'
#' @examples
#' # uncompress file
#' (outfile <- gunzip_file(gzfile, "tmp"))
#' file.info(outfile)
#' unlink(outfile)
#'
gunzip_file <- function(infile, outfile = gsub("\\.gz$", "", infile),
                        buffer_size = min(4 * file.size(infile), 2e9)) {

  if (!file.exists(infile)) stop(sprintf("File '%s' not found!", infile))
  if (file.exists(outfile)) unlink(outfile)

  gunzip_file_impl(infile, outfile, buffer_size)
  return(invisible(outfile))
}

#' @rdname gz_functions
#' @export
#' @details
#' -`gzip_file`: compresses a raw binary data file to a gz-archive
#'
#' @examples
#' # compress file
#' (outfile <- gzip_file(file))
#' file.info(outfile)
#' unlink(outfile)
gzip_file <- function(infile,
                      outfile = NA,
                      buffer_size = min(4 * file.size(infile), 2e9)) {

  if (!file.exists(infile)) stop(sprintf("File '%s' not found!", infile))

  if (is.na(outfile)) {
    outfile <- ifelse(grepl("\\.gz$", infile),
                      infile,
                      paste0(infile, ".gz"))
    # remove path
    xx <- strsplit(outfile, "\\\\|/")[[1]]
    outfile <- xx[length(xx)]
  }
  if (file.exists(outfile)) unlink(outfile)

  if (grepl("\\.gz$", infile)) {
    warning("Infile is already a gzipped-archive")
    return(invisible(infile))
  }

  gzip_file_impl(infile, outfile, buffer_size)
  return(invisible(outfile))
}


# Helper function
# returns the (if needed gunzipped) file
# note that it only operates in the dir directory
check_and_gunzip <- function(file, dir = dirname(file), buffer_size, force_gunzip, quiet) {
  file <- path.expand(file)
  if (!grepl("\\.gz$", file)) return(file)

  outfile <- file.path(dir, basename(gsub("\\.gz$", "", file)))
  # check if the raw-file at target directory already exists, if so use this (unless force_gunzip = TRUE)
  if (file.exists(outfile) && !quiet && !force_gunzip) {
    cat(sprintf("[INFO] Unzipped file '%s' already found, using that (overwrite with force_gunzip = TRUE)\n",
                outfile))
    return(outfile)
  }

  # check if the raw-file at current directory already exists, if so use this (unless force_gunzip = TRUE)
  if (file.exists(outfile) && !force_gunzip) {
    if (!quiet)
      cat(sprintf("[INFO] Unzipped file '%s' already found, using that (overwrite with force_gunzip = TRUE)\n",
                  outfile))
    return(outfile)
  } else {
    # if the unzipped file doesnt exist or the force_gunzip flag is set, unzip file
    unlink(outfile)
    if (!quiet)
      cat(sprintf("[Decompressing] '%s' to '%s'\n", file, outfile))

    gunzip_file(file, outfile, buffer_size)
  }
  return(outfile)
}
