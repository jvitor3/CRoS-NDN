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

print("Started summaryStrategy.r")
print("Summarize data for each strategy, final value for x parameter.")

data = read.table ("./temp/summaryXaxis.txt", header=T)
data$Strategy = factor (data$Strategy)    
output = summarySE (data, measurevar="ise", groupvars=c("Strategy",paste(varx)) )
#print (output)
write.table (output, "./temp/summaryStrategyXaxis.txt", append=TRUE, row.names = FALSE, col.names = FALSE)



