% Generated by roxygen2: do not edit by hand
% Please edit documentation in R/data_samples.R
\name{download_sample_file}
\alias{download_sample_file}
\title{Downloads a sample ITCH File from NASDAQs Server}
\usage{
download_sample_file(
  choice = c("smallest", "largest", "earliest", "latest", "random", "all"),
  file = NA,
  exchanges = NA,
  dir = ".",
  force_download = FALSE,
  check_md5sum = TRUE,
  quiet = FALSE
)
}
\arguments{
\item{choice}{which file should be chosen? One of: smallest (default), largest,
earliest (date-wise), latest, random, or all.}

\item{file}{the name of a specific file, overrules the choice and exchanges arguments}

\item{exchanges}{A vector of exchanges, can be NASDAQ, BX, or PSX.
The default value is to consider all exchanges.}

\item{dir}{The directory where the files will be saved to, default is current working directory.}

\item{force_download}{If the file should be downloaded even if it already exists locally.
Default value is FALSE.}

\item{check_md5sum}{If the md5-sum (hash-value) of the downloaded file should be checked, default value is TRUE.}

\item{quiet}{if TRUE, the status messages are suppressed, defaults to FALSE}
}
\value{
an invisible vector of the files
}
\description{
The Server can be found at \url{https://emi.nasdaq.com/ITCH/Nasdaq\%20ITCH/}
}
\details{
Warning: the smallest file is around 300 MB, with the largest exceeding 5 GB.
There are about 17 files in total. Downloading all might take a considerable amount of time.
}
\examples{
\dontrun{
download_sample_file()
file <- download_sample_file()
file

# download a specific sample file
file <- download_sample_file(file = "2019130.BX_ITCH_50.gz")
file
}
}
