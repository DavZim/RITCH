
<!-- README.md is generated from README.Rmd. Please edit that file -->

# RITCH - an R interface to the ITCH Protocol

<!-- badges: start -->

[![R-CMD-check](https://github.com/DavZim/RITCH/workflows/R-CMD-check/badge.svg)](https://github.com/DavZim/RITCH/actions)
<!-- badges: end -->

The `RITCH` library provides an `R` interface to NASDAQs ITCH protocol,
which is used to distribute financial messages to participants. Messages
include orders, trades, market status, and much more financial
information. A full list of messages is shown later. The main purpose of
this package is to parse the binary ITCH files to a
[`data.table`](https://CRAN.R-project.org/package=data.table) in `R`.

The package leverages [`Rcpp`](https://CRAN.R-project.org/package=Rcpp)
and `C++` for efficient message parsing. As an example, parsing 100
million orders from the `01302020.NASDAQ_ITCH50.gz` NASDAQ sample file
(13 GB uncompressed) takes around 30 seconds or 0.3 secs per 1 million
orders.

Note that the package provides a small simulated sample dataset in the
`ITCH_50` format for testing and example purposes. Helper functions are
provided to list and download sample files from NASDAQs official FTP
server.

## Install

To install `RITCH` you can use the following

``` r
# stable version:
# not yet available 
# install.packages("RITCH")

# development version:
# install.packages("remotes")
remotes::install_github("DavZim/RITCH")
```

## Quick Overview

The main functions of `RITCH` are read-related and are easily identified
by their `read_` prefix.

Due to the inherent structural differences between message classes, each
class has its own read function. A list of message types and the
respective classes are provided later in this Readme.

Example message classes used in this example are *orders* and *trades*.
First we define the file to load and count the messages, then we read in
the orders and the first 100 trades

``` r
library(RITCH)
# use built in example dataset
file <- system.file("extdata", "ex20101224.TEST_ITCH_50", package = "RITCH")

# count the number of messages in the file
msg_count <- count_messages(file)
#> [Counting]   12,012 messages found
#> [Converting] to data.table
#> [Done]       in 0.00 secs
str(msg_count)
#> Classes 'data.table' and 'data.frame':   22 obs. of  2 variables:
#>  $ msg_type: chr  "S" "R" "H" "Y" ...
#>  $ count   :integer64 6 3 3 0 0 0 0 0 ... 
#>  - attr(*, ".internal.selfref")=<externalptr>

# read the orders into a data.table
orders <- read_orders(file)
#> [Counting]   5,000 messages found
#> [Loading]    .
#> [Converting] to data.table
#> [Done]       in 0.08 secs
str(orders)
#> Classes 'data.table' and 'data.frame':   5000 obs. of  13 variables:
#>  $ msg_type       : chr  "A" "A" "F" "A" ...
#>  $ stock_locate   : int  2 2 2 2 2 2 2 2 2 2 ...
#>  $ tracking_number: int  0 0 0 0 0 0 0 0 0 0 ...
#>  $ timestamp      :integer64 31139052372053 31141354532167 32813425752711 32826656500150 32827351405783 32893988026867 33067242028997 33300886636321 ... 
#>  $ order_ref      :integer64 0 100 84836 87020 87040 93032 105532 121012 ... 
#>  $ buy            : logi  TRUE TRUE TRUE FALSE FALSE FALSE ...
#>  $ shares         : int  1000 1000 100 1220 2000 600 2000 200 100 3000 ...
#>  $ stock          : chr  "BOB" "BOB" "BOB" "BOB" ...
#>  $ price          : num  5.32 5.32 5.29 5.42 5.42 ...
#>  $ mpid           : chr  NA NA "VIRT" NA ...
#>  $ date           : POSIXct, format: "2010-12-24" "2010-12-24" "2010-12-24" "2010-12-24" ...
#>  $ datetime       :integer64 1293179939052372053 1293179941354532167 1293181613425752711 1293181626656500150 1293181627351405783 1293181693988026867 1293181867242028997 1293182100886636321 ... 
#>  $ exchange       : chr  "TEST" "TEST" "TEST" "TEST" ...
#>  - attr(*, ".internal.selfref")=<externalptr>

# read the first 100 trades
trades <- read_trades(file, n_max = 100)
#> NOTE: as n_max overrides counting the messages, the numbers for messages may be off.
#> [Counting]   100 messages found
#> [Loading]    .
#> [Converting] to data.table
#> [Done]       in 0.04 secs
str(trades)
#> Classes 'data.table' and 'data.frame':   100 obs. of  14 variables:
#>  $ msg_type       : chr  "P" "P" "P" "P" ...
#>  $ stock_locate   : int  2 2 2 2 2 3 3 3 3 3 ...
#>  $ tracking_number: int  2 2 2 2 2 2 4 2 2 2 ...
#>  $ timestamp      :integer64 34210128591201 34210355475120 34210767188977 34211127433476 34212046014088 34235711475708 34239928637481 34239928703094 ... 
#>  $ order_ref      :integer64 0 0 0 0 0 0 0 0 ... 
#>  $ buy            : logi  TRUE TRUE TRUE TRUE TRUE TRUE ...
#>  $ shares         : int  200 300 100 47 200 100 100 100 1 40 ...
#>  $ stock          : chr  "BOB" "BOB" "BOB" "BOB" ...
#>  $ price          : num  5.33 5.33 5.33 5.33 5.33 ...
#>  $ match_number   :integer64 19447 19451 19493 19515 19547 20148 20242 20241 ... 
#>  $ cross_type     : chr  NA NA NA NA ...
#>  $ date           : POSIXct, format: "2010-12-24" "2010-12-24" "2010-12-24" "2010-12-24" ...
#>  $ datetime       :integer64 1293183010128591201 1293183010355475120 1293183010767188977 1293183011127433476 1293183012046014088 1293183035711475708 1293183039928637481 1293183039928703094 ... 
#>  $ exchange       : chr  "TEST" "TEST" "TEST" "TEST" ...
#>  - attr(*, ".internal.selfref")=<externalptr>
```

Note that the file can be a plain `ITCH_50` file or a gzipped
`ITCH_50.gz` file, which will be decompressed to the current directory.

If you want to know more about the functions of the package, read on.

## Writing ITCH Files

`RITCH` also provides functionality for writing ITCH files. Although it
could be stored in other file formats (for example a database or a
[`qs`](https://CRAN.R-project.org/package=qs) file), ITCH files are
quite optimized regarding size as well as write/read speeds. Thus the
`write_itch()` function allows you to write a single or multiple types
of message to an `ITCH_50` file. Note however, that only the standard
columns are supported. Additional columns will not be written to file!

Additional information can be saved in the filename. By default the
date, exchange, and fileformat information is added to the filename
unless you specify `add_meta = FALSE`, in which case the given name is
used.

As a last note: if you write your data to an ITCH file and want to
filter for stocks later on, make sure to save the stock directory of
that day/exchange, either externally or in the ITCH file directly (see
example below).

### Simple Write Example

A simple write example would be to read all modifications from an ITCH
file and save it to a separate file to save space, reduce read times
later on, etc.

``` r
file <- system.file("extdata", "ex20101224.TEST_ITCH_50", package = "RITCH")
md <- read_modifications(file, quiet = TRUE)
dim(md)
#> [1] 2000   13

outfile <- write_itch(md, "modifications", compress = TRUE)
#> [Counting]   2,000 messages (44,748 bytes) found
#> [Converting] to binary .
#> [Writing]    to file
#> [Outfile]    'modifications_20101224.TEST_ITCH_50.gz'
#> [Done]       in 0.01 secs

# compare file sizes
files <- c(full_file = file, subset_file = outfile)
sapply(files, function(x) file.info(x)[["size"]])
#>   full_file subset_file 
#>      465048       23950
```

### Comprehensive Write Example

A typical work flow would look like this:

-   read in some message classes from file and filter for certain stocks
-   save the results for later analysis, also compress to save disk
    space

``` r
## Read in the different message classes
file <- system.file("extdata", "ex20101224.TEST_ITCH_50", package = "RITCH")

# create a list of functions which are used to read the data, in this case:
# system_events, stock_directory, and orders
read_functions <- list(
  system_event = read_system_events, 
  stock_directory = read_stock_directory, 
  orders = read_orders)

# read in the different message types
data <- lapply(
  read_functions, 
  function(f) f(file, filter_stock_locate = c(1, 3), quiet = TRUE)
)
str(data, max.level = 1)
#> List of 3
#>  $ system_event   :Classes 'data.table' and 'data.frame':    6 obs. of  8 variables:
#>   ..- attr(*, ".internal.selfref")=<externalptr> 
#>  $ stock_directory:Classes 'data.table' and 'data.frame':    2 obs. of  21 variables:
#>   ..- attr(*, ".internal.selfref")=<externalptr> 
#>  $ orders         :Classes 'data.table' and 'data.frame':    2518 obs. of  13 variables:
#>   ..- attr(*, ".internal.selfref")=<externalptr>


## Write the different message classes
outfile <- write_itch(data, "alc_char_subset", compress = TRUE)
#> [Counting]   2,526 messages (95,850 bytes) found
#> [Converting] to binary .
#> [Writing]    to file
#> [Outfile]    'alc_char_subset_20101224.TEST_ITCH_50.gz'
#> [Done]       in 0.01 secs
outfile
#> [1] "alc_char_subset_20101224.TEST_ITCH_50.gz"

# compare file sizes
files <- c(full_file = file, subset_file = outfile)
sapply(files, function(x) file.info(x)[["size"]])
#>   full_file subset_file 
#>      465048       37951


## Lastly, compare the two datasets to see if they are identical
data2 <- lapply(read_functions, function(f) f(outfile, quiet = TRUE))
all.equal(data, data2)
#> [1] TRUE
```

For comparison, the same format in the
[`qs`](https://CRAN.R-project.org/package=qs) format results in `44956`
bytes.
<!---qs::qsave(data, "data.qs", preset = "archive");file.info("data.qs")[["size"]]-->

## ITCH Messages

There are a total of 22 different message types which are grouped into
13 classes by `RITCH`.

The messages and their respective classes are:

| Type | Official Name                                      |             `RITCH` class |              `RITCH` read function | ITCH Section |
|-----:|:---------------------------------------------------|--------------------------:|-----------------------------------:|-------------:|
|  `S` | System Event Message                               |             System Events |             `read_system_events()` |          4.1 |
|  `R` | Stock Directory                                    |           Stock Directory |           `read_stock_directory()` |        4.2.1 |
|  `H` | Stock Trading Action                               |            Trading Status |            `read_trading_status()` |        4.2.2 |
|  `Y` | Reg SHO Short Sale Price Test Restricted Indicator |                   Reg SHO |                   `read_reg_sho()` |        4.2.3 |
|  `L` | Market Participant Position                        | Market Participant States | `read_market_participant_states()` |        4.2.4 |
|  `V` | MWCB Decline Level Message                         |                      MWCB |                      `read_mwcb()` |      4.2.5.1 |
|  `W` | MWCB Status Message                                |                      MWCB |                      `read_mwcb()` |      4.2.5.2 |
|  `K` | IPO Quoting Period Update                          |                       IPO |                       `read_ipo()` |        4.2.6 |
|  `J` | Limit Up - Limit Down (LULD) Auction Collar        |                      LULD |                      `read_luld()` |        4.2.7 |
|  `h` | Operational Halt                                   |            Trading Status |            `read_trading_status()` |        4.2.8 |
|  `A` | Add Order - no MPID Attribution                    |                    Orders |                    `read_orders()` |        4.3.1 |
|  `F` | Add Order with MPID Attribution                    |                    Orders |                    `read_orders()` |        4.3.2 |
|  `E` | Order Executed Message                             |                    Trades |                    `read_trades()` |        4.4.1 |
|  `C` | Order Executed with Price Message                  |                    Trades |                    `read_trades()` |        4.4.2 |
|  `X` | Order Cancel Message                               |                    Trades |                    `read_trades()` |        4.4.3 |
|  `D` | Order Delete Message                               |                    Trades |                    `read_trades()` |        4.4.4 |
|  `U` | Order Replace Message                              |                    Trades |                    `read_trades()` |        4.4.5 |
|  `P` | Trade Message (Non-Cross)                          |             Modifications |             `read_modifications()` |        4.5.1 |
|  `Q` | Cross Trade Message                                |             Modifications |             `read_modifications()` |        4.5.2 |
|  `B` | Broken Trade/Order Execution Message               |             Modifications |             `read_modifications()` |        4.5.3 |
|  `I` | Net Order Imbalance Indicator (NOII) Message       |                      NOII |                      `read_noii()` |          4.6 |
|  `N` | Retail Price Improvement Indicator (RPII)          |                      RPII |                      `read_rpii()` |          4.7 |

Note that if you are interested in the exact definition of the messages
and its components, you should look into the [official ITCH
specification](https://www.nasdaqtrader.com/content/technicalsupport/specifications/dataproducts/NQTVITCHspecification.pdf),
which can also be opened by calling `open_itch_specification()`.

## Data

The `RITCH` package provides a small, artificial dataset in the ITCH
format for example and testing purposes. To learn more about the dataset
check `?ex20101224.TEST_ITCH_50`.

To access the dataset use:

``` r
file <- system.file("extdata", "ex20101224.TEST_ITCH_50", package = "RITCH")
count_messages(file, add_meta_data = TRUE, quiet = TRUE)
#>     msg_type count                                  msg_name                 msg_class  doc_nr
#>  1:        S     6                      System Event Message             System Events     4.1
#>  2:        R     3                           Stock Directory           Stock Directory   4.2.1
#>  3:        H     3                      Stock Trading Action            Trading Status   4.2.2
#>  4:        Y     0                       Reg SHO Restriction                   Reg SHO   4.2.3
#>  5:        L     0               Market Participant Position Market Participant States   4.2.4
#>  6:        V     0                MWCB Decline Level Message                      MWCB 4.2.5.1
#>  7:        W     0                       MWCB Status Message                      MWCB 4.2.5.2
#>  8:        K     0                 IPO Quoting Period Update                       IPO   4.2.6
#>  9:        J     0                       LULD Auction Collar                      LULD   4.2.7
#> 10:        A  4997                         Add Order Message                    Orders   4.3.1
#> 11:        F     3      Add Order - MPID Attribution Message                    Orders   4.3.2
#> 12:        E   198                    Order Executed Message                    Orders   4.4.1
#> 13:        C     0 Order Executed Message With Price Message             Modifications   4.4.2
#> 14:        X    45                      Order Cancel Message             Modifications   4.4.3
#> 15:        D  1745                      Order Delete Message             Modifications   4.4.4
#> 16:        U    12                     Order Replace Message             Modifications   4.4.5
#> 17:        P  5000                 Trade Message (Non-Cross)                    Trades   4.5.1
#> 18:        Q     0                       Cross Trade Message                    Trades   4.5.2
#> 19:        B     0                      Broken Trade Message                    Trades   4.5.3
#> 20:        I     0                              NOII Message                      NOII     4.6
#> 21:        N     0                   Retail Interest Message                      RPII     4.7
#>     msg_type count                                  msg_name                 msg_class  doc_nr
```

Note that the example dataset does not contain messages from all classes
but is limited to 6 system messages, 3 stock directory, 3 stock trading
action, 5000 trade, 5000 order, and 2000 order modification messages. As
seen by the 3 stock directory messages, the file contains data about 3
made up stocks (see also the plot later in the Readme).

MASDAQ provides sample ITCH files on their official FTP server at
<ftp://emi.nasdaq.com/ITCH/> (or in R use `open_itch_ftp()`) which can
be used to test code on larger datasets. Note that the sample files are
up to 5GB compressed, which inflate to about 13GB. To interact with the
sample files, use `list_sample_files()` and `download_sample_files()`.

## Notes on Memory and Speed

There are some tweaks available to deal with memory and speed issues.
For faster reading speeds, you can increase the buffer size of the
`read_` functions to something around 1 GB or more
(`buffer_size = 1e9`).

### Provide Message Counts

If you have to read from a single file multiple times, for example
because you want to extract orders and trades, you can count the
messages beforehand and provide it to each read’s `n_max` argument,
reducing the need to pass the file for counting the number of messages.

``` r
# count messages once
n_msgs <- count_messages(file, quiet = TRUE)

# use counted messages multiple times, saving file passes
orders <- read_orders(file, quiet = TRUE, n_max = n_msgs)
trades <- read_trades(file, quiet = TRUE, n_max = n_msgs)
```

### Batch Read

If the dataset does not fit entirely into RAM, you can do a partial read
specifying `skip` and `n_max`, similar to this:

``` r
file <- system.file("extdata", "ex20101224.TEST_ITCH_50", package = "RITCH")

n_messages <- count_orders(count_messages(file, quiet = TRUE))
n_messages
#> [1] 5000

# read 1000 messages at a time
n_batch <- 1000
n_parsed <- 0

while (n_parsed < n_messages) {
  cat(sprintf("Parsing Batch %04i - %04i", n_parsed, n_parsed + n_batch))
  # read in a batch
  df <- read_orders(file, quiet = TRUE, skip = n_parsed, n_max = n_batch)
  cat(sprintf(": with %04i orders\n", nrow(df)))
  # do someting with the data, e.g., save data
  # ...
  n_parsed <- n_parsed + n_batch
}
#> Parsing Batch 0000 - 1000: with 1000 orders
#> Parsing Batch 1000 - 2000: with 1000 orders
#> Parsing Batch 2000 - 3000: with 1000 orders
#> Parsing Batch 3000 - 4000: with 1000 orders
#> Parsing Batch 4000 - 5000: with 1000 orders
```

### Filter Data

You can also filter a dataset directly while reading messages for
`msg_type`, `stock_locate`, `timestamp` range, as well as `stock`. Note
that filtering for a specific stock, is just a shorthand lookup for the
stocks `stock_locate`s, therefore a `stock_directory` needs to be
supplied (either by providing the output from `read_stock_directory()`
or `download_stock_locate()`) or the function will try to extract the
stock directory from the file.

``` r
# read in the stock directory as we filter for stock names later on
sdir <- read_stock_directory(file, quiet = TRUE)

od <- read_orders(
  file, 
  filter_msg_type = "A",          # take only no MPID add orders
  min_timestamp = 43200000000000, # start at 12:00:00.000000
  max_timestamp = 55800000000000, # end at 15:30:00.000000
  filter_stock_locate = 1,        # take only stock with code 1
  filter_stock = "CHAR",          # but also take stock CHAR
  stock_directory = sdir          # provide the stock_directory to match stock names to stock_locates
)
#> [Filter]     msg_type: 'A'
#> [Filter]     timestamp: 43200000000000 - 55800000000000 
#> [Filter]     stock_locate: '1', '3'
#> NOTE: as filter arguments were given, the number of messages may be off
#> [Counting]   5,000 messages found
#> [Loading]    .
#> [Converting] to data.table
#> [Done]       in 0.04 secs

od[, .(n = .N), by = msg_type]
#>    msg_type    n
#> 1:        A 1082
range(od$timestamp)
#> integer64
#> [1] 43235810473334 55792143963723
od[, .(n = .N), by = .(stock_locate, stock)]
#>    stock_locate stock   n
#> 1:            3  CHAR 574
#> 2:            1   ALC 508
```

If you are interested in gaining a better understanding of the internal
data structures, converting data to and from binary, have a look at the
`debug` folder and its contents.

### Create a Plot with Trades and Orders of the largest ETFs

As a last step, a quick visualization of the example dataset

``` r
library(ggplot2)

file <- system.file("extdata", "ex20101224.TEST_ITCH_50", package = "RITCH")

# load the data
orders <- read_orders(file, quiet = TRUE)
trades <- read_trades(file, quiet = TRUE)

# replace the buy-factor with something more useful
orders[, buy := ifelse(buy, "Bid", "Ask")]

ggplot() +
  geom_point(data = orders, 
             aes(x = as.POSIXct(datetime), y = price, color = buy), alpha = 0.2) +
  geom_step(data = trades, aes(x = as.POSIXct(datetime), y = price), size = 0.2) +
  facet_grid(stock~., scales = "free_y") +
  theme_light() +
  labs(title = "Orders and Trades of Three Simulated Stocks",
       subtitle = "Date: 2010-12-24 | Exchange: TEST", 
       caption = "Source: RITCH package", x = "Time", y = "Price", color = "Side") +
  scale_y_continuous(labels = scales::dollar) +
  scale_color_brewer(palette = "Set1")
```

<img src="man/figures/README-ETF_plot-1.png" width="100%" />

## Open Issues

If you find this package useful or have any other kind of feedback, I’d
be happy if you let me know. Otherwise, if you need more functionality,
please feel free to create an issue or a pull request.

Citation and CRAN release are WIP.
