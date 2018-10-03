#!/usr/bin/env Rscript
print("Started summaryRoundAppDelay.r")
print("Process and summarizes simulation raw data.")
# install.packages ('scales')  
library (scales)
# install.packages ('doBy')
library (doBy)
#install.packages('data.table')
library(data.table)
source("./graphs/graph-style.R")
source("./graphs/variables.r")
source("./graphs/varx.r")

for ( Strategy in stratset)
if (Strategy != 'null')
{
  print (Strategy)
  source(paste("./temp/",suffix,"/cface-",Strategy,".r", sep = ""))
  fname=paste("./temp/",suffix,"/app-delays-trace-",Strategy,topo, sep = "")
  data <- fread (fname, header=T)#, colClasses = classes, nrows=numRow)
  print ("  Read file Ok")
#  data = subset (data, data$Time %in% c(min(data$Time):max(data$Time)))
  print("  Filter time Ok")
  data$Node = factor (data$Node)
  data$AppId = factor(data$AppId)
  data$Type = factor (data$Type)
  data$TimeSec = 1 * ceiling (data$Time / 1)
  if (exists('datacons'))
  {
    rm('datacons')
  }
  datacons0 = subset (data, Type %in% c("FullDelay"))
  #datacons0 = subset (data, Type %in% c("LastDelay"))
  datacons0 = subset (datacons0, Node %in% c(consumer))
  datacons0 = subset (datacons0, AppId %in% c(faceid))

  for (i in (1:length(consumer)))
  {
    if (!exists('datacons'))
    {
      datacons = subset (datacons0, (Node %in% consumer[i] & AppId %in% faceid[i]))
    }
    else
    {
      datacons = rbind(datacons, subset (datacons0, (Node %in% consumer[i] & AppId %in% faceid[i])))
    }
  }
  print("  Filter packets Ok")

  datacons = subset(datacons, select=c('TimeSec','Time','DelayS'))
  #datacons.sum = summaryBy (. ~ TimeSec, data=datacons, FUN=max)
  datacons.sum = summaryBy (. ~ TimeSec, data=datacons, FUN=mean)

    
  print("")
  datacons.sum = cbind(Strategy, datacons.sum)
  
#  datacons.sum = subset (datacons.sum, select = c('Strategy','TimeSec','DelayS'))
  
  if (!exists('aux'))
  {
    aux = datacons.sum
  }
  else
  {
    aux = rbind(aux, datacons.sum)
  }
  print("  Calculus Ok")
}

aux = subset (aux, select = c('TimeSec', 'Strategy','DelayS.mean'))
xtime = cbind(varx1,aux)
write.table (xtime, "./temp/summaryAppDelaytime.txt", append=TRUE, row.names = FALSE, col.names = FALSE)
print(xtime)

xaxis = subset (xtime, select = c('Strategy','DelayS.mean'))
xaxis = summaryBy (. ~ Strategy, data=xaxis, FUN=mean)
print (paste("Log varx: ",varx))
xaxis = cbind(varx1,xaxis)
print(xaxis)
write.table (xaxis, "./temp/summaryAppDelayVar.txt", append=TRUE, row.names = FALSE, col.names = FALSE)
