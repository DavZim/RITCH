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
tmp_raw <- tempfile(pattern = "ritch_raw_", tmpdir = tempdir())
(outfile <- gunzip_file(gzfile, tmp_raw))
#> [1] "/tmp/Rtmp0LDw4D/ritch_raw_19f15abafe66"
file.info(outfile)
#>                                          size isdir mode               mtime
#> /tmp/Rtmp0LDw4D/ritch_raw_19f15abafe66 465048 FALSE  644 2026-04-16 14:42:52
#>                                                      ctime               atime
#> /tmp/Rtmp0LDw4D/ritch_raw_19f15abafe66 2026-04-16 14:42:52 2026-04-16 14:42:52
#>                                         uid  gid  uname grname
#> /tmp/Rtmp0LDw4D/ritch_raw_19f15abafe66 1001 1001 runner runner
unlink(outfile)

# compress file
tmp_raw <- tempfile(pattern = "ritch_raw_", tmpdir = tempdir())
file.copy(file, tmp_raw)
#> [1] TRUE
(outfile <- gzip_file(tmp_raw))
#> [1] "/tmp/Rtmp0LDw4D/ritch_raw_19f123e19245.gz"
file.info(outfile)
#>                                             size isdir mode               mtime
#> /tmp/Rtmp0LDw4D/ritch_raw_19f123e19245.gz 159965 FALSE  644 2026-04-16 14:42:52
#>                                                         ctime
#> /tmp/Rtmp0LDw4D/ritch_raw_19f123e19245.gz 2026-04-16 14:42:52
#>                                                         atime  uid  gid  uname
#> /tmp/Rtmp0LDw4D/ritch_raw_19f123e19245.gz 2026-04-16 14:42:52 1001 1001 runner
#>                                           grname
#> /tmp/Rtmp0LDw4D/ritch_raw_19f123e19245.gz runner
unlink(c(tmp_raw, outfile))
```
