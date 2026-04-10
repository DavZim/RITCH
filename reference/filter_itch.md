# Filters an ITCH file to another ITCH file

This function allows to perform very fast filter operations on large
ITCH files. The messages are written to another ITCH file.

## Usage

``` r
filter_itch(
  infile,
  outfile,
  filter_msg_class = NA_character_,
  filter_msg_type = NA_character_,
  filter_stock_locate = NA_integer_,
  min_timestamp = bit64::as.integer64(NA),
  max_timestamp = bit64::as.integer64(NA),
  filter_stock = NA_character_,
  stock_directory = NA,
  skip = 0,
  n_max = -1,
  append = FALSE,
  overwrite = FALSE,
  gz = FALSE,
  buffer_size = -1,
  quiet = FALSE,
  force_gunzip = FALSE,
  force_cleanup = TRUE
)
```

## Arguments

- infile:

  the input file where the messages are taken from, can be a gz-archive
  or a plain ITCH file.

- outfile:

  the output file where the filtered messages are written to. Note that
  the date and exchange information from the `infile` are used, see also
  [`add_meta_to_filename()`](https://davzim.github.io/RITCH/reference/add_meta_to_filename.md)
  for further information.

- filter_msg_class:

  a vector of classes to load, can be "orders", "trades",
  "modifications", ... see also
  [`get_msg_classes()`](https://davzim.github.io/RITCH/reference/get_msg_classes.md).
  Default value is to take all message classes.

- filter_msg_type:

  a character vector, specifying a filter for message types. Note that
  this can be used to only return 'A' orders for instance.

- filter_stock_locate:

  an integer vector, specifying a filter for locate codes. The locate
  codes can be looked up by calling
  [`read_stock_directory()`](https://davzim.github.io/RITCH/reference/read_functions.md)
  or by downloading from NASDAQ by using
  [`download_stock_directory()`](https://davzim.github.io/RITCH/reference/download_stock_directory.md).
  Note that some message types (e.g., system events, MWCB, and IPO) do
  not use a locate code.

- min_timestamp:

  an 64 bit integer vector (see also
  [`bit64::as.integer64()`](https://rdrr.io/pkg/bit64/man/as.integer64.character.html))
  of minimum timestamp (inclusive). Note: min and max timestamp must be
  supplied with the same length or left empty.

- max_timestamp:

  an 64 bit integer vector (see also
  [`bit64::as.integer64()`](https://rdrr.io/pkg/bit64/man/as.integer64.character.html))
  of maxium timestamp (inclusive). Note: min and max timestamp must be
  supplied with the same length or left empty.

- filter_stock:

  a character vector, specifying a filter for stocks. Note that this a
  shorthand for the `filter_stock_locate` argument, as it tries to find
  the stock_locate based on the `stock_directory` argument, if this is
  not found, it will try to extract the stock directory from the file,
  else an error is thrown.

- stock_directory:

  A data.frame containing the stock-locate code relationship. As
  outputted by
  [`read_stock_directory()`](https://davzim.github.io/RITCH/reference/read_functions.md).
  Only used if `filter_stock` is set. To download the stock directory
  from NASDAQs server, use
  [`download_stock_directory()`](https://davzim.github.io/RITCH/reference/download_stock_directory.md).

- skip:

  Number of messages to skip before starting parsing messages, note the
  skip parameter applies to the specific message class, i.e., it would
  skip the messages for each type (e.g., skip the first 10 messages for
  each class).

- n_max:

  Maximum number of messages to parse, default is to read all values.
  Can also be a data.frame of msg_types and counts, as returned by
  [`count_messages()`](https://davzim.github.io/RITCH/reference/count_functions.md).
  Note the n_max parameter applies to the specific message class not the
  whole file.

- append:

  if the messages should be appended to the outfile, default is false.
  Note, this is helpful if `skip` and or `n_max` are used for batch
  filtering.

- overwrite:

  if an existing outfile with the same name should be overwritten.
  Default value is false

- gz:

  if the output file should be gzip-compressed. Note that the name of
  the output file will be appended with .gz if not already present. The
  final output name is returned. Default value is false.

- buffer_size:

  the size of the buffer in bytes, defaults to 1e8 (100 MB), if you have
  a large amount of RAM, 1e9 (1GB) might be faster

- quiet:

  if TRUE, the status messages are suppressed, defaults to FALSE

- force_gunzip:

  only applies if the input file is a gz-archive and a file with the
  same (gunzipped) name already exists. if set to TRUE, the existing
  file is overwritten. Default value is FALSE

- force_cleanup:

  only applies if the input file is a gz-archive. If force_cleanup=TRUE,
  the gunzipped raw file will be deleted afterwards. Only applies when
  the gunzipped raw file did not exist before.

## Value

the name of the output file (maybe different from the inputted outfile
due to adding the date and exchange), silently

## Details

Note that this can be especially useful on larger files or where memory
is not large enough to filter the datalimits the analysis.

As with the
[`read_itch()`](https://davzim.github.io/RITCH/reference/read_functions.md)
functions, it allows to filter for `msg_class`, `msg_type`,
`stock_locate`/`stock`, and `timestamp`.

## Examples

``` r
infile <- system.file("extdata", "ex20101224.TEST_ITCH_50", package = "RITCH")
outfile <- tempfile(fileext = "_20101224.TEST_ITCH_50")
filter_itch(
  infile, outfile,
  filter_msg_class = c("orders", "trades"),
  filter_msg_type = "R", # stock_directory
  skip = 0, n_max = 100
)
#> [Filter]     skip: 0 n_max: 100 (1 - 100)
#> [Filter]     msg_type: 'R', 'A', 'F', 'P', 'Q', 'B'
#> [Bytes]      scanned 465048, filtered 8527
#> [Messages]   scanned 12012, filtered 203
#> [Done]       in 0.12 secs at 3.88MB/s

# expecting 100 orders, 100 trades, and 3 stock_directory entries
count_messages(outfile)
#> [Counting]   203 total messages found
#> [Converting] to data.table
#> [Done]       in 0.00 secs at 46.75MB/s
#>     msg_type count
#>       <char> <i64>
#>  1:        S     0
#>  2:        R     3
#>  3:        H     0
#>  4:        Y     0
#>  5:        L     0
#>  6:        V     0
#>  7:        W     0
#>  8:        K     0
#>  9:        J     0
#> 10:        h     0
#> 11:        A    99
#> 12:        F     1
#> 13:        E     0
#> 14:        C     0
#> 15:        X     0
#> 16:        D     0
#> 17:        U     0
#> 18:        P   100
#> 19:        Q     0
#> 20:        B     0
#> 21:        I     0
#> 22:        N     0
#>     msg_type count
#>       <char> <i64>

# check that the output file contains the same
res  <- read_itch(outfile, c("orders", "trades", "stock_directory"))
#> [Counting]   num messages 203
#> [Counting]   num 'stock_directory' messages 3
#> [Counting]   num 'orders' messages 100
#> [Counting]   num 'trades' messages 100
#> [Converting] to data.table
#> [Done]       in 0.12 secs at 68.69KB/s
sapply(res, nrow)
#>          orders          trades stock_directory 
#>             100             100               3 

res2 <- read_itch(infile,  c("orders", "trades", "stock_directory"),
                  n_max = 100)
#> [Note]       n_max overrides counting the messages. Number of messages may be off
#> [Filter]     skip: 0 n_max: 100 (1 - 100)
#> [Counting]   num 'stock_directory' messages 100
#> [Counting]   num 'orders' messages 200
#> [Counting]   num 'trades' messages 300
#> [Converting] to data.table
#> [Done]       in 0.12 secs at 3.82MB/s

all.equal(res, res2)
#> [1] TRUE
```
