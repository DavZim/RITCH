# Downloads a sample ITCH File from NASDAQs Server

The Server can be found at <https://emi.nasdaq.com/ITCH/Nasdaq%20ITCH/>

## Usage

``` r
download_sample_file(
  choice = c("smallest", "largest", "earliest", "latest", "random", "all"),
  file = NA,
  exchanges = NA,
  dir = ".",
  force_download = FALSE,
  check_md5sum = TRUE,
  quiet = FALSE
)
```

## Arguments

- choice:

  which file should be chosen? One of: smallest (default), largest,
  earliest (date-wise), latest, random, or all.

- file:

  the name of a specific file, overrules the choice and exchanges
  arguments

- exchanges:

  A vector of exchanges, can be NASDAQ, BX, or PSX. The default value is
  to consider all exchanges.

- dir:

  The directory where the files will be saved to, default is current
  working directory.

- force_download:

  If the file should be downloaded even if it already exists locally.
  Default value is FALSE.

- check_md5sum:

  If the md5-sum (hash-value) of the downloaded file should be checked,
  default value is TRUE.

- quiet:

  if TRUE, the status messages are suppressed, defaults to FALSE

## Value

an invisible vector of the files

## Details

Warning: the smallest file is around 300 MB, with the largest exceeding
5 GB. There are about 17 files in total. Downloading all might take a
considerable amount of time.

## Examples

``` r
if (FALSE) { # \dontrun{
download_sample_file()
file <- download_sample_file()
file

# download a specific sample file
file <- download_sample_file(file = "2019130.BX_ITCH_50.gz")
file
} # }
```
