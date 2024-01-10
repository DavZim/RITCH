library(RITCH)
library(tinytest)
library(data.table)
setDTthreads(2)

infile <- system.file("extdata", "ex20101224.TEST_ITCH_50", package = "RITCH")

ll <- read_itch(infile, quiet = TRUE)

################################################################################
################################################################################
#### Testing base write functionality
outfile_base <- file.path(tempdir(), "testfile")
outfile <- write_itch(ll, outfile_base, quiet = TRUE)

expect_equal(file.size(infile)[[1]], file.size(outfile)[[1]])

################################################################################
# expect identical files
expect_equal(tools::md5sum(infile)[[1]],
             tools::md5sum(outfile)[[1]])

# read in the file again and compare to outfile
ll2 <- read_itch(outfile, quiet = TRUE)
expect_equal(ll, ll2)


################################################################################
################################################################################
# Appending doubles file size
# appending throws warning
outfile <- write_itch(ll, outfile, quiet = TRUE, add_meta = FALSE)
expect_warning(
  outfile <- write_itch(ll, outfile, quiet = TRUE, add_meta = FALSE,
                        append = TRUE)
)
expect_equal(file.size(outfile), 465048 * 2)

################################################################################
# read in again and compare to original doubled data
ll3 <- lapply(ll, function(x) rbindlist(list(x, x)))
ll4 <- read_itch(outfile, quiet = TRUE)
expect_equal(ll3, ll4)


################################################################################
################################################################################
#### Testing buffer_size
# buffer too large
expect_warning(
  outfile <- write_itch(ll, outfile, buffer_size = 5e9 + 1,
                        quiet = TRUE, add_meta = FALSE)
)
################################################################################
# buffer too small
expect_warning(
  outfile <- write_itch(ll, outfile, buffer_size = 51,
                        quiet = TRUE, add_meta = FALSE)
)
################################################################################
# small but ok buffer
outfile <- write_itch(ll, outfile, buffer_size = 52,
                      quiet = TRUE, add_meta = FALSE)

expect_equal(file.size(outfile), 465048)
# read in the file again and compare to outfile
ll2 <- read_itch(outfile, quiet = TRUE)
expect_equal(ll, ll2)

unlink(outfile)


################################################################################
################################################################################
#### Test gz compression file
outfile <- write_itch(ll, outfile_base, compress = TRUE, quiet = TRUE)

expect_equal(file.size(outfile), 159965)

# read in the file again and compare to outfile
ll2 <- read_itch(outfile, quiet = TRUE, force_gunzip = TRUE, force_cleanup = TRUE)
expect_equal(ll, ll2)

################################################################################
# test gz with smaller buffer size
outfile <- write_itch(ll, outfile_base, compress = TRUE, buffer_size = 100,
                      quiet = TRUE)

# with smaller buffer sizes when using compress = TRUE, the filesize will increase!
expect_equal(file.size(outfile), 419608)
# read in the file again and compare to outfile
ll2 <- read_itch(outfile, quiet = TRUE, force_gunzip = TRUE, force_cleanup = TRUE)
expect_equal(ll, ll2)

unlink(outfile)


################################################################################
################################################################################
#### check append and compress
write_itch(ll, outfile, compress = TRUE, buffer_size = 100, add_meta = FALSE,
           quiet = TRUE)
expect_equal(file.size(outfile), 419608)

expect_warning(
  outfile <- write_itch(ll, outfile, compress = TRUE, append = TRUE,
                        buffer_size = 100, add_meta = FALSE, quiet = TRUE)
)

# note that appending to a gzipped file will linearly increase file size...
# only the buffers are compressed!
expect_equal(file.size(outfile), 419608 * 2)

expect_equal(lapply(ll, function(x) rbindlist(list(x, x))),
             read_itch(outfile, quiet = TRUE, force_gunzip = TRUE,
                       force_cleanup = TRUE))

unlink(outfile)
