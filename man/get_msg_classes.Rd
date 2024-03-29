% Generated by roxygen2: do not edit by hand
% Please edit documentation in R/count_functions.R
\name{get_msg_classes}
\alias{get_msg_classes}
\title{Returns the message class data for the message types}
\usage{
get_msg_classes()
}
\value{
a data.table with the information of the message-types
}
\description{
All information is handled according to the official ITCH 5.0
documentation as found here:
\url{http://www.nasdaqtrader.com/content/technicalsupport/specifications/dataproducts/NQTVITCHSpecification.pdf}
}
\details{
\itemize{
\item \code{msg_type} the type of the message
\item \code{msg_class} the group the message belongs to
\item \code{msg_name} the official name of the message
\item \code{doc_nr} the number of the message in the documentation
}
}
\examples{
get_msg_classes()
}
\seealso{
\code{open_itch_specification()}
}
