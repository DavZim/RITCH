# Adds meta information (date and exchange) to an itch filename

Note that if date and exchange information are already present, they are
overwritten

## Usage

``` r
add_meta_to_filename(file, date, exchange)
```

## Arguments

- file:

  the filename

- date:

  the date as a date-class or as a string that is understood by
  [`base::as.Date()`](https://rdrr.io/r/base/as.Date.html).

- exchange:

  the name of the exchange

## Value

the filename with exchanged or added date and exchange information

## Examples

``` r
add_meta_to_filename("03302017.NASDAQ_ITCH50", "2010-12-24", "TEST")
#> [1] "12242010.TEST_ITCH50"
add_meta_to_filename("20170130.BX_ITCH_50.gz", "2010-12-24", "TEST")
#> [1] "20101224.TEST_ITCH_50.gz"
add_meta_to_filename("S030220-v50-bx.txt.gz", "2010-12-24", "TEST")
#> [1] "S122410-v50-TEST.txt.gz"
add_meta_to_filename("unknown_file.ITCH_50", "2010-12-24", "TEST")
#> [1] "unknown_file_20101224.TEST_ITCH_50"
```
