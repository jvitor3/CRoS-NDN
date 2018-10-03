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
#source("./graphs/variables.r")

#varx = 'Strategy'

print("Started summaryStrategy.r")
print("Summarize data for each strategy, final value for x parameter.")
flag=TRUE
toposet = c("1221","1755","2914","3257","3356","7018")
for (t in toposet)
{
	data = read.table (paste("./temp/summary-var-topo-value-",t,".txt-run-49.txt",sep = ""), header=T)
	data$Strategy = factor (data$Strategy)
	data = subset (data, TimeSec %in% c(max(data$TimeSec)))    
	#output = summarySE (data, measurevar="ise", groupvars=c("Strategy",paste(varx)))
	output = summarySE (data, measurevar="ise", groupvars=c("Strategy"))
	output = cbind(t,output)
	#print (output)
	write.table (output, "./temp/summaryStrategyXaxis.txt", append=!flag, row.names = FALSE, col.names = flag)
	flag=FALSE
}
