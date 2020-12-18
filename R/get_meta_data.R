#' Returns the meta data for the messages
#' 
#' All information is handled according to the official ITCH 5.0
#' documentation as found here: 
#' \url{http://www.nasdaqtrader.com/content/technicalsupport/specifications/dataproducts/NQTVITCHSpecification.pdf}
#' 
#' \code{msg_type} the type of the message 
#' \code{msg_name} the official name of the message
#' \code{msg_group} the group the message belongs to
#' \code{doc_nr} the number of the message in the documentation
#'
#' @return a data.table with the information of the message-types
#' @export
#'
#' @examples
#' get_meta_data()
get_meta_data <- function() {
  meta <- data.table(
    msg_type = c("S", "R", "H", "Y", "L", "V", "W", "K", "J", "A", "F", "E", 
                 "C", "X", "D", "U", "P", "Q", "B", "I", "N"),
    msg_name = c("System Event Message", "Stock Directory", 
                 "Stock Trading Action", "Reg SHO Restriction", 
                 "Market Participant Position", "MWCB Decline Level Message",
                 "MWCB Status Message", "IPO Quoting Period Update",
                 "LULD Auction Collar", "Add Order Message", 
                 "Add Order - MPID Attribution Message",
                 "Order Executed Message", 
                 "Order Executed Message With Price Message",
                 "Order Cancel Message", "Order Delete Message",
                 "Order Replace Message", "Trade Message (Non-Cross)",
                 "Cross Trade Message", "Broken Trade Message", 
                 "NOII Message",
                 "Retail Interest Message"),
    msg_class = c("System Events", "Stock Directory",
                  "Trading Status", "Reg SHO",
                  "Market Participant States", "MWCB", "MWCB", "IPO",
                  "LULD", "Orders", "Orders", "Orders", "Modifications", 
                  "Modifications", "Modifications", "Modifications",   
                  "Trades", "Trades", "Trades", "NOII", "RPII"),
    doc_nr = c("4.1", "4.2.1", "4.2.2", "4.2.3", "4.2.4", "4.2.5.1", "4.2.5.2",
               "4.2.6", "4.2.7", "4.3.1", "4.3.2", "4.4.1", "4.4.2", "4.4.3",
               "4.4.4", "4.4.5", "4.5.1", "4.5.2", "4.5.3", "4.6", "4.7")
  )
  return(meta)
}
