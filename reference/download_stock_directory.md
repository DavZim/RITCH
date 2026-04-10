# Downloads the stock directory (stock locate codes) for a given date and exchange

The data is downloaded from NASDAQs server, which can be found here
<https://emi.nasdaq.com/ITCH/Stock_Locate_Codes/>

## Usage

``` r
download_stock_directory(exchange, date, cache = FALSE, quiet = FALSE)
```

## Arguments

- exchange:

  The exchange, either NASDAQ (equivalent to NDQ), BX, or PSX

- date:

  The date, should be of class Date. If not the value is converted using
  `as.Date`.

- cache:

  If the stock directory should be cached, can be set to TRUE to save
  the stock directories in the working directory or a character for a
  target directory.

- quiet:

  If the download function should be quiet, default is FALSE.

## Value

a data.table of the tickers, the respective stock locate codes, and the
exchange/date information

## Examples

``` r
if (FALSE) { # \dontrun{
  download_stock_directory("BX", "2019-07-02")
  download_stock_directory(c("BX", "NDQ"), c("2019-07-02", "2019-07-03"))
  download_stock_directory("BX", "2019-07-02", cache = TRUE)

  download_stock_directory("BX", "2019-07-02", cache = "stock_directory")
  dir.exists("stock_directory")
  list.files("stock_directory")
} # }
```
