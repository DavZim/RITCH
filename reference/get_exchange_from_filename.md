# Returns the exchange from an ITCH-filename

Returns the exchange from an ITCH-filename

## Usage

``` r
get_exchange_from_filename(file)
```

## Arguments

- file:

  a filename

## Value

The exchange

## Examples

``` r
get_exchange_from_filename("03302017.NASDAQ_ITCH50")
#> [1] "NASDAQ"
get_exchange_from_filename("20170130.BX_ITCH_50.gz")
#> [1] "BX"
get_exchange_from_filename("S030220-v50-bx.txt.gz")
#> [1] "BX"
get_exchange_from_filename("Unknown_file_format")
#> [1] NA
```
