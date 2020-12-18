library(RITCH)
library(tinytest)
library(data.table)

infile <- system.file("extdata", "ex20101224.TEST_ITCH_50", package = "RITCH")
read_funcs <- list(read_system_events, read_stock_directory, read_orders, read_trades)

ll <- lapply(read_funcs, function(f) f(infile, quiet = TRUE))

outfile_base <- file.path(tempdir(), "test")
outfile <- write_itch(ll, outfile_base, quiet = TRUE)

#### Testing base write functionality
expect_equal(file.info(outfile)[["size"]], 420219)

# read in the file again and compare to outfile
ll2 <- lapply(read_funcs, function(f) f(outfile, quiet = TRUE))
expect_equal(ll, ll2)

# Appending doubles file size

# appending throws warning
outfile <- write_itch(ll, outfile, quiet = TRUE, add_meta = FALSE)
expect_warning(
  outfile <- write_itch(ll, outfile, quiet = TRUE, add_meta = FALSE, 
                        append = TRUE)
)
expect_equal(file.info(outfile)[["size"]], 420219 * 2)

# read in again and compare to original doubled data
ll3 <- lapply(ll, function(x) rbindlist(list(x, x)))
ll4 <- lapply(read_funcs, function(f) f(outfile, quiet = TRUE))
expect_equal(ll3, ll4)



#### Testing buffer_size
# buffer too large
expect_warning(
  outfile <- write_itch(ll, outfile, buffer_size = 5e9 + 1,
                        quiet = TRUE, add_meta = FALSE)
)
# buffer too small
expect_warning(
  outfile <- write_itch(ll, outfile, buffer_size = 51,
                        quiet = TRUE, add_meta = FALSE)
)
# small but ok buffer
outfile <- write_itch(ll, outfile, buffer_size = 52,
                      quiet = TRUE, add_meta = FALSE)

expect_equal(file.info(outfile)[["size"]], 420219)
# read in the file again and compare to outfile
ll2 <- lapply(read_funcs, function(f) f(outfile, quiet = TRUE))
expect_equal(ll, ll2)



#### Test gz compression file
outfile <- write_itch(ll, outfile_base, compress = TRUE, quiet = TRUE)

expect_equal(file.info(outfile)[["size"]], 139732)
# read in the file again and compare to outfile
ll2 <- lapply(read_funcs, function(f) f(outfile, quiet = TRUE, force_gunzip = TRUE))
expect_equal(ll, ll2)

# test gz with smaller buffer size
outfile <- write_itch(ll, outfile_base, compress = TRUE, buffer_size = 100, quiet = TRUE)

# with smaller buffer sizes when using compress = TRUE, the filesize will increase!
expect_equal(file.info(outfile)[["size"]], 375563)
# read in the file again and compare to outfile
ll2 <- lapply(read_funcs, function(f) f(outfile, quiet = TRUE, force_gunzip = TRUE))
expect_equal(ll, ll2)



#### check append and compress
write_itch(ll, outfile, compress = TRUE, buffer_size = 100, add_meta = FALSE, quiet = TRUE)
expect_equal(file.info(outfile)[["size"]], 375563)
expect_warning(
  outfile <- write_itch(ll, outfile, compress = TRUE, append = TRUE, 
                        buffer_size = 100, add_meta = FALSE, quiet = TRUE)
)
# note that appending to a gzipped file will linearly increase file size...
# only the buffers are compressed!
expect_equal(file.info(outfile)[["size"]], 375563 * 2)

expect_equal(lapply(ll, function(x) rbindlist(list(x, x))),
             lapply(read_funcs, function(f) f(outfile, quiet = TRUE, force_gunzip = TRUE)))
