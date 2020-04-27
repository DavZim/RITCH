
<!-- README.md is generated from README.Rmd. Please edit that file -->

# RITCH - an R interface to the ITCH Protocol

<!-- badges: start -->

[![Travis build
status](https://travis-ci.org/DavZim/RITCH.svg?branch=master)](https://travis-ci.org/DavZim/RITCH)
<!-- badges: end -->

## The What?\!

This R package allows you to read files that use the ITCH protocol
(version 5.0) of NASDAQ and parse it into a data.table.

The ITCH protocol allows NASDAQ to distribute financial information to
market participants. The financial information includes orders, trades,
order modifications, trading status, traded stocks, and more.

## The Why?\!

During my research I had to parse some ITCH files and couldn’t find any
R libraries. While parsing the files in pure R is certainly possible,
this library uses `C++` and `Rcpp` to speed up the parsing process.

A typical file containing a single trading day consists of something
like 30-50 million messages (BX-exchange) up to 230 million messages
(NASDAQ), thus speed makes a crucial difference. As the data is streamed
from the file, the execution time mainly depends on the reading/writing
speed of the hard-drive.

As a general benchmark, it takes my machine about 20 seconds to count
the messages of a plain-file (unzipped, 55 million order) and about 10
seconds longer for a `.gz`-file.

Enough of the talk, how can I use the package?

## The How?\!

Currently the code only lives on GitHub but eventually it may find its
way to CRAN. Until then, you have to use `remotes` or `devtools` to
install `RITCH`:

### Installation

You can install the development version from
[GitHub](https://github.com/DavZim/RITCH) with:

``` r
# install.packages("remotes")
remotes::install_github("DavZim/RITCH")
```

### Example Usage

As a first step, we want to count how often each message is found in a
given file.

### Counting Messages

RITCH is able to read gzipped (`*.XXX_ITCH_50.gz`) and unzipped files
(`*.XXX_ITCH_50`), for speed reasons, we gunzipped the file by hand as
we will use it multiple times throughout this example.

As I don’t own any rights to the data, the data is not included in this
package, but can be downloaded from NASDAQs FTP server here:
<ftp://emi.nasdaq.com/ITCH/>

``` r
library(RITCH)

file <- "20191230.BX_ITCH_50"

msg_count <- count_messages(file, add_meta_data = TRUE)
#> [Counting]   29,156,757 messages found
#> [Converting] to data.table
#> [Done]       in 0.39 secs
msg_count
#>     msg_type    count                                  msg_name                                    msg_group  doc_nr
#>  1:        S        6                      System Event Message                         System Event Message     4.1
#>  2:        R     8906                           Stock Directory                       Stock Related Messages   4.2.1
#>  3:        H     8961                      Stock Trading Action                       Stock Related Messages   4.2.2
#>  4:        Y     9013                       Reg SHO Restriction                       Stock Related Messages   4.2.3
#>  5:        L     6171               Market Participant Position                       Stock Related Messages   4.2.4
#>  6:        V        1                MWCB Decline Level Message                       Stock Related Messages 4.2.5.1
#>  7:        W        0                       MWCB Status Message                       Stock Related Messages 4.2.5.2
#>  8:        K        0                 IPO Quoting Period Update                       Stock Related Messages   4.2.6
#>  9:        J        0                       LULD Auction Collar                       Stock Related Messages   4.2.7
#> 10:        A 12210139                         Add Order Message                            Add Order Message   4.3.1
#> 11:        F    45058      Add Order - MPID Attribution Message                            Add Order Message   4.3.2
#> 12:        E   578839                    Order Executed Message                        Modify Order Messages   4.4.1
#> 13:        C     2686 Order Executed Message With Price Message                        Modify Order Messages   4.4.2
#> 14:        X   348198                      Order Cancel Message                        Modify Order Messages   4.4.3
#> 15:        D 11821540                      Order Delete Message                        Modify Order Messages   4.4.4
#> 16:        U  1741672                     Order Replace Message                        Modify Order Messages   4.4.5
#> 17:        P   134385                 Trade Message (Non-Cross)                               Trade Messages   4.5.1
#> 18:        Q        0                       Cross Trade Message                               Trade Messages   4.5.2
#> 19:        B        0                      Broken Trade Message                               Trade Messages   4.5.3
#> 20:        I        0                              NOII Message Net Order Imbalance Indicator (NOII) Message     4.6
#> 21:        N  2241182                   Retail Interest Message    Retail Price Improvement Indicator (RPII)     4.7
#>     msg_type    count                                  msg_name                                    msg_group  doc_nr
```

As you can see, there are a lot of different message types. Currently
this package parses only messages from the following groups - “Add Order
Messages” (type ‘A’ and ‘F’), - “Modify Order Messages” (type ‘E’, ‘C’,
‘X’, ‘D’, and ‘U’), - and “Trade Messages” (type ‘P’, ‘Q’, and ‘B’).

To get an overview of this information, you can also use the
`get_meta_data()` function.

``` r
get_meta_data()
#>     msg_type                                  msg_name                                    msg_group  doc_nr
#>  1:        S                      System Event Message                         System Event Message     4.1
#>  2:        R                           Stock Directory                       Stock Related Messages   4.2.1
#>  3:        H                      Stock Trading Action                       Stock Related Messages   4.2.2
#>  4:        Y                       Reg SHO Restriction                       Stock Related Messages   4.2.3
#>  5:        L               Market Participant Position                       Stock Related Messages   4.2.4
#>  6:        V                MWCB Decline Level Message                       Stock Related Messages 4.2.5.1
#>  7:        W                       MWCB Status Message                       Stock Related Messages 4.2.5.2
#>  8:        K                 IPO Quoting Period Update                       Stock Related Messages   4.2.6
#>  9:        J                       LULD Auction Collar                       Stock Related Messages   4.2.7
#> 10:        A                         Add Order Message                            Add Order Message   4.3.1
#> 11:        F      Add Order - MPID Attribution Message                            Add Order Message   4.3.2
#> 12:        E                    Order Executed Message                        Modify Order Messages   4.4.1
#> 13:        C Order Executed Message With Price Message                        Modify Order Messages   4.4.2
#> 14:        X                      Order Cancel Message                        Modify Order Messages   4.4.3
#> 15:        D                      Order Delete Message                        Modify Order Messages   4.4.4
#> 16:        U                     Order Replace Message                        Modify Order Messages   4.4.5
#> 17:        P                 Trade Message (Non-Cross)                               Trade Messages   4.5.1
#> 18:        Q                       Cross Trade Message                               Trade Messages   4.5.2
#> 19:        B                      Broken Trade Message                               Trade Messages   4.5.3
#> 20:        I                              NOII Message Net Order Imbalance Indicator (NOII) Message     4.6
#> 21:        N                   Retail Interest Message    Retail Price Improvement Indicator (RPII)     4.7
#>     msg_type                                  msg_name                                    msg_group  doc_nr
```

You can extract the different message-types by using the functions
`get_orders()`, `get_modifications()`, and `get_trades()`, respectively.
The doc-number refers to the section in the official documentation (see
link below).

If you are annoyed by the feedback the function gives you (`[Counting]
... [Converting]...`), you can always turn the feedback off with the
`quiet = TRUE` option (this applies to all functions of this package).

If you need more message type compatibility, you are more than welcome
to post an issue or open a pull request.

### Retrieve Orders

``` r
orders  <- get_orders(file)
#> [Counting]   12,255,197 messages found
#> [Loading]    .........
#> [Converting] to data.table
#> [Done]       in 2.61 secs
orders
#>           msg_type locate_code tracking_number      timestamp order_ref   buy shares stock  price mpid       date
#>        1:        A        8236               0 25200002107428         4  TRUE  11900   USO  12.96 <NA> 2019-12-30
#>        2:        A        2220               0 25200002110565     31989  TRUE  15000   DWT   3.38 <NA> 2019-12-30
#>        3:        A        8101               0 25200002132115         8  TRUE   1500   UCO  21.03 <NA> 2019-12-30
#>        4:        A        8236               0 25200002150516        12 FALSE  11900   USO  12.99 <NA> 2019-12-30
#>        5:        A        2220               0 25200002160093     31993 FALSE  15000   DWT   3.40 <NA> 2019-12-30
#>       ---                                                                                                        
#> 12255193:        A        4315               0 68374788588500  85855062 FALSE   1800   IWM 165.55 <NA> 2019-12-30
#> 12255194:        A        6556               0 68374789602669  82661763 FALSE   1000   QQQ 212.28 <NA> 2019-12-30
#> 12255195:        A        7279               0 68374789872250  89116736  TRUE    500  SMLL  52.67 <NA> 2019-12-30
#> 12255196:        A        8494               0 68374790724019  89116740  TRUE    400  VTWO 132.75 <NA> 2019-12-30
#> 12255197:        A        7451               0 68374891256781  89116748  TRUE    500   SPY 321.25 <NA> 2019-12-30
#>                                      datetime
#>        1: 2019-12-30T07:00:00.002107428+00:00
#>        2: 2019-12-30T07:00:00.002110565+00:00
#>        3: 2019-12-30T07:00:00.002132115+00:00
#>        4: 2019-12-30T07:00:00.002150516+00:00
#>        5: 2019-12-30T07:00:00.002160093+00:00
#>       ---                                    
#> 12255193: 2019-12-30T18:59:34.788588500+00:00
#> 12255194: 2019-12-30T18:59:34.789602669+00:00
#> 12255195: 2019-12-30T18:59:34.789872250+00:00
#> 12255196: 2019-12-30T18:59:34.790724019+00:00
#> 12255197: 2019-12-30T18:59:34.891256781+00:00
```

If you want to load only a specified number of messages (this applies to
all `get_*` functions), you can always specify a start and end message
number.

For example, if you want to get only the first 10 orders, you can use
the following code.

``` r
orders_small  <- get_orders(file, 1, 10)
#> [Counting]   10 messages found
#> [Loading]    .
#> [Converting] to data.table
#> [Done]       in 0.10 secs
orders_small
#>     msg_type locate_code tracking_number      timestamp order_ref   buy shares stock  price mpid       date
#>  1:        A        8236               0 25200002107428         4  TRUE  11900   USO  12.96 <NA> 2019-12-30
#>  2:        A        2220               0 25200002110565     31989  TRUE  15000   DWT   3.38 <NA> 2019-12-30
#>  3:        A        8101               0 25200002132115         8  TRUE   1500   UCO  21.03 <NA> 2019-12-30
#>  4:        A        8236               0 25200002150516        12 FALSE  11900   USO  12.99 <NA> 2019-12-30
#>  5:        A        2220               0 25200002160093     31993 FALSE  15000   DWT   3.40 <NA> 2019-12-30
#>  6:        A        8101               0 25200002170000        16 FALSE   1500   UCO  21.10 <NA> 2019-12-30
#>  7:        A        8167               0 25200002183311        20  TRUE   1300   UNG  17.07 <NA> 2019-12-30
#>  8:        A        1987               0 25200002198910     31997 FALSE    100  DGAZ 176.00 <NA> 2019-12-30
#>  9:        A        8167               0 25200002209200        24 FALSE   1300   UNG  17.13 <NA> 2019-12-30
#> 10:        A        8236               0 25200002243946        28  TRUE  11900   USO  12.95 <NA> 2019-12-30
#>                                datetime
#>  1: 2019-12-30T07:00:00.002107428+00:00
#>  2: 2019-12-30T07:00:00.002110565+00:00
#>  3: 2019-12-30T07:00:00.002132115+00:00
#>  4: 2019-12-30T07:00:00.002150516+00:00
#>  5: 2019-12-30T07:00:00.002160093+00:00
#>  6: 2019-12-30T07:00:00.002170000+00:00
#>  7: 2019-12-30T07:00:00.002183311+00:00
#>  8: 2019-12-30T07:00:00.002198910+00:00
#>  9: 2019-12-30T07:00:00.002209200+00:00
#> 10: 2019-12-30T07:00:00.002243946+00:00
```

### Retrieve Trades

``` r
trades <- get_trades(file)
#> [Counting]   134,385 messages found
#> [Loading]    .........
#> [Converting] to data.table
#> [Done]       in 0.87 secs
trades
#>         msg_type locate_code tracking_number      timestamp order_ref  buy shares stock   price match_number cross_type
#>      1:        P        8124               2 26366446396437         0 TRUE    100  UGAZ  76.050        17803       <NA>
#>      2:        P        1987               2 28995161549677         0 TRUE    100  DGAZ 175.640        17814       <NA>
#>      3:        P        1987               2 28995162132166         0 TRUE    100  DGAZ 175.610        17815       <NA>
#>      4:        P        8268               2 29008286649292         0 TRUE   1000   UWT  14.821        17817       <NA>
#>      5:        P        6098               2 29192675981664         0 TRUE     53  PEIX   0.702        17819       <NA>
#>     ---                                                                                                                
#> 134381:        P        6966               2 59697140243203         0 TRUE    100  SAVA   5.930       733868       <NA>
#> 134382:        P        7080               2 61030182372978         0 TRUE     27  SDRL   2.840       733872       <NA>
#> 134383:        P        7080               2 61340181587362         0 TRUE     47  SDRL   2.840       733873       <NA>
#> 134384:        P        3328               4 62788556629955         0 TRUE    400  GHSI   0.250       733875       <NA>
#> 134385:        P        3328               4 63090027940171         0 TRUE    300  GHSI   0.250       733878       <NA>
#>               date                            datetime
#>      1: 2019-12-30 2019-12-30T07:19:26.446396437+00:00
#>      2: 2019-12-30 2019-12-30T08:03:15.161549677+00:00
#>      3: 2019-12-30 2019-12-30T08:03:15.162132166+00:00
#>      4: 2019-12-30 2019-12-30T08:03:28.286649292+00:00
#>      5: 2019-12-30 2019-12-30T08:06:32.675981664+00:00
#>     ---                                               
#> 134381: 2019-12-30 2019-12-30T16:34:57.140243203+00:00
#> 134382: 2019-12-30 2019-12-30T16:57:10.182372978+00:00
#> 134383: 2019-12-30 2019-12-30T17:02:20.181587362+00:00
#> 134384: 2019-12-30 2019-12-30T17:26:28.556629955+00:00
#> 134385: 2019-12-30 2019-12-30T17:31:30.027940171+00:00
```

### Retrieve Order Modifications

``` r
mods <- get_modifications(file)
#> [Counting]   14,492,935 messages found
#> [Loading]    .........
#> [Converting] to data.table
#> [Done]       in 1.99 secs
mods
#>           msg_type locate_code tracking_number      timestamp order_ref shares match_number printable price
#>        1:        D         393               0 25200008944172     32009     NA         <NA>        NA    NA
#>        2:        D        8124               0 25200013213153        96     NA         <NA>        NA    NA
#>        3:        E        8124               2 25200013663284        56    100        17795        NA    NA
#>        4:        D         393               0 25200013964664     32033     NA         <NA>        NA    NA
#>        5:        D         687               0 25200015715177     32025     NA         <NA>        NA    NA
#>       ---                                                                                                  
#> 14492931:        D        2952               0 68400026060023  66995214     NA         <NA>        NA    NA
#> 14492932:        D        5125               0 68400026060337   4393583     NA         <NA>        NA    NA
#> 14492933:        D        5125               0 68400026069079  72136531     NA         <NA>        NA    NA
#> 14492934:        D        4155               0 68400026070313  47273658     NA         <NA>        NA    NA
#> 14492935:        D        2952               0 68400026070658  63304398     NA         <NA>        NA    NA
#>           new_order_ref       date                            datetime
#>        1:          <NA> 2019-12-30 2019-12-30T07:00:00.008944172+00:00
#>        2:          <NA> 2019-12-30 2019-12-30T07:00:00.013213153+00:00
#>        3:          <NA> 2019-12-30 2019-12-30T07:00:00.013663284+00:00
#>        4:          <NA> 2019-12-30 2019-12-30T07:00:00.013964664+00:00
#>        5:          <NA> 2019-12-30 2019-12-30T07:00:00.015715177+00:00
#>       ---                                                             
#> 14492931:          <NA> 2019-12-30 2019-12-30T19:00:00.026060023+00:00
#> 14492932:          <NA> 2019-12-30 2019-12-30T19:00:00.026060337+00:00
#> 14492933:          <NA> 2019-12-30 2019-12-30T19:00:00.026069079+00:00
#> 14492934:          <NA> 2019-12-30 2019-12-30T19:00:00.026070313+00:00
#> 14492935:          <NA> 2019-12-30 2019-12-30T19:00:00.026070658+00:00
```

To speed up the `get_*` functions, we can use the message-count
information from earlier. For example the following code yields the same
results as above, but saves time.

``` r

orders <- get_orders(file, msg_count)
#> [Counting]   12,255,197 messages found
#> [Loading]    .........
#> [Converting] to data.table
#> [Done]       in 1.91 secs
trades <- get_trades(file, msg_count)
#> [Counting]   134,385 messages found
#> [Loading]    .........
#> [Converting] to data.table
#> [Done]       in 0.56 secs
mods   <- get_modifications(file, msg_count)
#> [Counting]   14,492,935 messages found
#> [Loading]    .........
#> [Converting] to data.table
#> [Done]       in 1.41 secs
# # alternatively, provide the start and end number of messages:
# orders <- get_orders(file, 1, count_orders(msg_count))
# trades <- get_trades(file, 1, count_trades(msg_count))
# mods   <- get_modifications(file, 1, count_modifications(msg_count))
```

### Create a Plot with Trades and Orders of the largest ETFs

As a last step, a quick visualisation of the data

``` r
library(ggplot2)

# load the data
orders <- get_orders(file, msg_count)
#> [Counting]   12,255,197 messages found
#> [Loading]    .........
#> [Converting] to data.table
#> [Done]       in 1.80 secs
trades <- get_trades(file, msg_count)
#> [Counting]   134,385 messages found
#> [Loading]    .........
#> [Converting] to data.table
#> [Done]       in 0.62 secs

# data munging
tickers <- c("SPY", "IWO")
dt_orders <- orders[stock %in% tickers]
dt_trades <- trades[stock %in% tickers]

# for each ticker, use only orders that are within 1% of the range of traded prices
ranges <- dt_trades[, .(min_price = min(price), max_price = max(price)), by = stock]
# filter the orders
dt_orders <- dt_orders[ranges, on = "stock"][price >= 0.99 * min_price & price <= 1.01 * max_price]
# replace the buy-factor with something more useful
dt_orders[, buy := ifelse(buy, "Bid", "Ask")]
dt_orders[, stock := factor(stock, levels = tickers)]

# data visualization
ggplot() +
  # add the orders to the plot
  geom_point(data = dt_orders, 
             aes(x = as.POSIXct(datetime), y = price, color = buy), size = 0.5, alpha = 0.2) +
  # add the trades as a black line to the plot
  geom_step(data = dt_trades, 
            aes(x = as.POSIXct(datetime), y = price)) +
  # add a facet for each ETF
  facet_grid(stock~., scales = "free_y") +
  # some Aesthetics
  theme_light() +
  labs(title = "Orders and Trades of the largest ETFs",
       subtitle = "Date: 2019-12-30 | Exchange: BX", 
       caption = "Source: NASDAQ ITCH",
       x = "Time", y = "Price", 
       color = "Side") +
  scale_y_continuous(labels = scales::dollar) +
  scale_color_brewer(palette = "Set1")
```

<img src="man/figures/README-ETF_plot-1.png" width="100%" />

## Some considerations

All functions that take a file-name work for both `.gz`-files (i.e.,
`YYYYMMDD.XXX_ITCH_50.gz`) or or plain-files (`YYYYMMDD.XXX_ITCH_50`).
The compressed files will be uncompressed into a temp-file and deleted
after the function finishes, if you want to use multiple commands on one
file, it might be faster to use `R.utils::gunzip(..., remove = FALSE)`
in R or `gunzip -k YYYYMMDD.XXX_ITCH_50.gz` in the terminal and call the
functions on the plain, uncompressed file.

Parsing another example file (`20170130.BX_ITCH_50.gz`, 714MB gzipped
and 1.6GB unzipped) has a peak RAM-consumption of around 7GB on my
machine.

If the file is too large for your RAM, you can also process everything
in batches by providing a start and end message count. I.e., to only
parse the first 1,000 orders, you can use `get_orders(file, 1, 1000)`.

To speed the parsing up, you can also specify the number of messages.
I.e., count all messages once using `count_messages()`, and then provide
the number of trades/orders/order-modifications to `end_msg_count` in
each subsequent function call. This saves the parser one trip over the
file.

You can also just provide the message counts directly, e.g.:

``` r
msg_count <- count_messages(file)
#> [Counting]   29,156,757 messages found
#> [Converting] to data.table
#> [Done]       in 0.35 secs
orders <- get_orders(file, msg_count)
#> [Counting]   12,255,197 messages found
#> [Loading]    .........
#> [Converting] to data.table
#> [Done]       in 1.80 secs
```

versus providing the actual start and end position of messages:

``` r
msg_count <- count_messages(file)
#> [Counting]   29,156,757 messages found
#> [Converting] to data.table
#> [Done]       in 0.36 secs
orders <- get_orders(file, 1, count_orders(msg_count))
#> [Counting]   12,255,197 messages found
#> [Loading]    .........
#> [Converting] to data.table
#> [Done]       in 1.77 secs
```

## Additional Sources aka. Data

While this package does not contain any real financial data using the
ITCH format, NASDAQ provides some sample datasets on its FTP-server,
which you can find here: <ftp://emi.nasdaq.com/ITCH>

If you want to find out more about the protocol, have a look at the
official protocol specification, which you can find here:
<https://www.nasdaqtrader.com/content/technicalsupport/specifications/dataproducts/NQTVITCHspecification.pdf>

If you find this package useful or have any other kind of feedback, I’d
be happy if you let me know. Otherwise, if you need more functionality
for additional message types, please feel free to create an issue or a
pull request.

## Open Issues

To move the package towards CRAN, I want to include a smaller data file
containing fake or simulated data, this needs to be converted to the
ITCH format. This would allow the example code to run, but also to
properly use unit tests in the package.

Currently the `C++` code parses the data into data.frame, which is then
converted into a data.table. If possible, the data.table should be
directly populated. But I haven’t found the time to look into a Rcpp
data.table API.
