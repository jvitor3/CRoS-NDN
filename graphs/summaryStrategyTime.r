#!/usr/bin/env Rscript
# install.packages ('scales')  
library (scales)
# install.packages ('doBy')
library (doBy)
#install.packages('data.table')
library(data.table)
library (plyr)
source("./graphs/summarySE.r")
source("./graphs/graph-style.R")
source("./graphs/variables.r")

print("Started summaryStrategyTime.r")
print("Summarize time data for each strategy and each parameter value.")

data = read.table ("./temp/summarytime.txt", header=T)

temp = summarySE (data, measurevar="ise", groupvars=c("Strategy",paste(varx),"TimeSec") )
output = subset (temp, select = c(paste(varx), 'Strategy', 'TimeSec', 'ise', 'sd', 'se', 'ci'))
write.table (output, "./temp/summaryStrategyTime.txt", append=TRUE, row.names = FALSE, col.names = FALSE)

if (length(stratset) < 3)
{

temp = summarySE (data, measurevar="RouteRequests", groupvars=c("Strategy",paste(varx),"TimeSec") )
output = subset (temp, select = c(paste(varx), 'Strategy', 'TimeSec', 'RouteRequests', 'sd', 'se', 'ci'))
write.table (output, "./temp/summaryStrategyTimeRouteRequests.txt", append=TRUE, row.names = FALSE, col.names = FALSE)

temp = summarySE (data, measurevar="DiscoveryRequests", groupvars=c("Strategy",paste(varx),"TimeSec") )
output = subset (temp, select = c(paste(varx), 'Strategy', 'TimeSec', 'DiscoveryRequests', 'sd', 'se', 'ci'))
write.table (output, "./temp/summaryStrategyTimeDiscoveryRequests.txt", append=TRUE, row.names = FALSE, col.names = FALSE)

temp = summarySE (data, measurevar="RRouterRequests", groupvars=c("Strategy",paste(varx),"TimeSec") )
output = subset (temp, select = c(paste(varx), 'Strategy', 'TimeSec', 'RRouterRequests', 'sd', 'se', 'ci'))
write.table (output, "./temp/summaryStrategyTimeRRouterRequests.txt", append=TRUE, row.names = FALSE, col.names = FALSE)

temp = summarySE (data, measurevar="RContentRequests", groupvars=c("Strategy",paste(varx),"TimeSec") )
output = subset (temp, select = c(paste(varx), 'Strategy', 'TimeSec', 'RContentRequests', 'sd', 'se', 'ci'))
write.table (output, "./temp/summaryStrategyTimeRContentRequests.txt", append=TRUE, row.names = FALSE, col.names = FALSE)
}