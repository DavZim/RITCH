# Writes a data.frame or a list of data.frames of ITCH messages to file

Note that additional information, e.g., columns that were added, will be
dropped in the process and only ITCH-compliant information is saved.

## Usage

``` r
write_itch(
  ll,
  file,
  add_meta = TRUE,
  append = FALSE,
  compress = FALSE,
  buffer_size = 1e+08,
  quiet = FALSE,
  append_warning = TRUE
)
```

## Arguments

- ll:

  a data.frame or a list of data.frames of ITCH messages, in the format
  that the
  [`read_functions()`](https://davzim.github.io/RITCH/reference/read_functions.md)
  return

- file:

  the filename of the target file. If the folder to the file does not
  exist, it will be created recursively

- add_meta:

  if date and file information should be added to the filename. Default
  value is TRUE. Note that adding meta information changes the filename.

- append:

  if the information should be appended to the file. Default value is
  FALSE

- compress:

  if the file should be gzipped. Default value is FALSE. Note that if
  you compress a file, buffer_size matters a lot, with larger buffers
  you are more likely to get smaller filesizes in the end.
  Alternatively, but slower, is to write the file without compression
  fully and then gzip the file using another program.

- buffer_size:

  the maximum buffer size. Default value is 1e8 (100MB). Accepted values
  are \> 52 and \< 5e9

- quiet:

  if TRUE, the status messages are suppressed, defaults to FALSE

- append_warning:

  if append is set, a warning about timestamp ordering is given. Set
  `append_warning = FALSE` to silence the warning. Default value is TRUE

## Value

the filename (invisibly)

## Details

Note that the ITCH filename contains the information for the date and
exchange. This can be specified explicitly in the file argument or it is
added if not turned off `add_meta = FALSE`.

## Examples

``` r
infile <- system.file("extdata", "ex20101224.TEST_ITCH_50", package = "RITCH")
sys <- read_system_events(infile, quiet = TRUE)
outfile <- tempfile()
write_itch(sys, outfile)
#> [Counting]   6 messages (84 bytes) found
#> [Converting] to binary .
#> [Writing]    to file
#> [Outfile]    '/tmp/RtmpH56qgV/file19d576781db3_20101224.TEST_ITCH_50'
#> [Done]       in 0.00 secs at 89.92KB/s

# create a list of events, stock directory, and orders and write to a file
sdir <- read_stock_directory(infile, quiet = TRUE)
od   <- read_orders(infile, quiet = TRUE)

ll <- list(sys, sdir, od)
write_itch(ll, outfile)
#> [Counting]   5,009 messages (190,219 bytes) found
#> [Converting] to binary .
#> [Writing]    to file
#> [Outfile]    '/tmp/RtmpH56qgV/file19d576781db3_20101224.TEST_ITCH_50'
#> [Done]       in 0.02 secs at 11.98MB/s
```
