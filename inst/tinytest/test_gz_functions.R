library(RITCH)
library(tinytest)
setDTthreads(2)

# check that using gunzip_file and gzip_file return the same files as the originals!
raw_file <- system.file("extdata", "ex20101224.TEST_ITCH_50", package = "RITCH")
gz_file  <- system.file("extdata", "ex20101224.TEST_ITCH_50.gz", package = "RITCH")

tmpfile <- file.path(tempdir(), "raw_20101224.TEST_ITCH_50")
tmpfile2 <- file.path(tempdir(), "gz_20101224.TEST_ITCH_50.gz")

expect_true(file.exists(raw_file))
expect_true(file.exists(gz_file))

gunzip_file(gz_file, tmpfile)
expect_equal(
  tools::md5sum(raw_file)[[1]],
  tools::md5sum(tmpfile)[[1]]
)

gzip_file(raw_file, tmpfile2)

# check that the file contents are identical
expect_equal(
  read_itch(raw_file, quiet = TRUE),
  read_itch(tmpfile2, quiet = TRUE, force_gunzip = TRUE, force_cleanup = TRUE)
)
expect_equal(
  read_itch(raw_file, quiet = TRUE),
  read_itch(tmpfile2, quiet = TRUE, force_gunzip = TRUE, force_cleanup = TRUE)
)

unlink(c(tmpfile, tmpfile2))
