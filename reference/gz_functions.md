# Compresses and uncompresses files to and from gz-archives

Allows the compression and uncompression of files

## Usage

``` r
gunzip_file(
  infile,
  outfile = gsub("\\.gz$", "", infile),
  buffer_size = min(4 * file.size(infile), 2e+09)
)

gzip_file(
  infile,
  outfile = NA,
  buffer_size = min(4 * file.size(infile), 2e+09)
)
```

## Arguments

- infile:

  the file to be zipped or unzipped

- outfile:

  the resulting zipped or unzipped file

- buffer_size:

  the size of the buffer to read in at once, default is 4 times the
  file.size (max 2Gb).

## Value

The filename of the unzipped file, invisibly

## Details

Functions are

- `gunzip_file`: uncompresses a gz-archive to raw binary data

\-`gzip_file`: compresses a raw binary data file to a gz-archive

## Examples

``` r
gzfile <- system.file("extdata", "ex20101224.TEST_ITCH_50.gz", package = "RITCH")
file   <- system.file("extdata", "ex20101224.TEST_ITCH_50", package = "RITCH")

# uncompress file
(outfile <- gunzip_file(gzfile, "tmp"))
#> [1] "tmp"
file.info(outfile)
#>       size isdir mode               mtime               ctime
#> tmp 465048 FALSE  644 2026-04-10 19:32:46 2026-04-10 19:32:46
#>                   atime  uid  gid  uname grname
#> tmp 2026-04-10 19:32:46 1001 1001 runner runner
unlink(outfile)

# compress file
(outfile <- gzip_file(file))
#> [1] "ex20101224.TEST_ITCH_50.gz"
file.info(outfile)
#>                              size isdir mode               mtime
#> ex20101224.TEST_ITCH_50.gz 159965 FALSE  644 2026-04-10 19:32:46
#>                                          ctime               atime  uid  gid
#> ex20101224.TEST_ITCH_50.gz 2026-04-10 19:32:46 2026-04-10 19:32:46 1001 1001
#>                             uname grname
#> ex20101224.TEST_ITCH_50.gz runner runner
unlink(outfile)
```
