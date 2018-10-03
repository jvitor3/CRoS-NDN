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

print("Started summaryStrategyTimeDelay.r")
print("Summarize time data for each strategy and each parameter value.")

data = read.table ("./temp/summaryAppDelaytime.txt", header=T)

temp = summarySE (data, measurevar="Delay", groupvars=c("Strategy",paste(varx),"TimeSec") )
output = subset (temp, select = c(paste(varx), 'Strategy', 'TimeSec', 'Delay', 'sd', 'se', 'ci'))
write.table (output, "./temp/summaryAppDelaySumtime.txt", append=TRUE, row.names = FALSE, col.names = FALSE)
