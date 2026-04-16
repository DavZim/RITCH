# Returns a data.table of the sample files on the server

The Server can be found at <https://emi.nasdaq.com/ITCH/Nasdaq%20ITCH/>

## Usage

``` r
list_sample_files()
```

## Value

a data.table of the files

## Examples

``` r
# \donttest{
if (interactive()) {
  list_sample_files()
}
# }
```
