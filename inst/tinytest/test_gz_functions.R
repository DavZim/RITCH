library(RITCH)
library(tinytest)

# check that using gunzip_file and gzip_file return the same files as the originals!
raw_file <- system.file("extdata", "ex20101224.TEST_ITCH_50", package = "RITCH")
gz_file  <- system.file("extdata", "ex20101224.TEST_ITCH_50.gz", package = "RITCH")

expect_true(file.exists(raw_file))
expect_true(file.exists(gz_file))

tmpfile <- tempfile(fileext = "_20101224.TEST_ITCH_50")
tmpfile

gunzip_file(gz_file, tmpfile)
expect_equal(
  tools::md5sum(raw_file)[[1]],
  tools::md5sum(tmpfile)[[1]]
)

tmpfile2 <- tempfile("20101224", fileext = ".TEST_ITCH_50.gz")

gzip_file(tmpfile, tmpfile2)
expect_equal(
  tools::md5sum(gz_file)[[1]],
  tools::md5sum(tmpfile2)[[1]]
)

unlink(c(tmpfile, tmpfile2))
