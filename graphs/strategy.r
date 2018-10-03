#!/usr/bin/env Rscript
# install.packages ('ggplot2')  
library (ggplot2)
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
  fname=paste("./temp/",suffix,"/aggregate-trace-",Strategy,topo, sep = "")
  data <- fread (fname, header=T)#, colClasses = classes, nrows=numRow)
  print ("  Read file Ok")
  data = subset (data, data$Time %in% c(min(data$Time):max(data$Time)))
  print("  Filter time Ok")
  data$Node = factor (data$Node)
  data$FaceId = factor(data$FaceId)
  data$Type = factor (data$Type)
  data$TimeSec = 1 * ceiling (data$Time / 1)
  if (exists('datacons'))
  {
    rm('datacons')
  }
  datatimej = subset (data, Type %in% c("OutInterests"))
  datacons0 = subset (data, Type %in% c("OutData"))
  datacons0 = subset (datacons0, Node %in% c(consumer))
  datacons0 = subset (datacons0, FaceId %in% c(faceid))
  for (i in (1:length(consumer)))
  {
    if (!exists('datacons'))
    {
      datacons = subset (datacons0, (Node %in% consumer[i] & FaceId %in% faceid[i]))
    }
    else
    {
      datacons = rbind(datacons, subset (datacons0, (Node %in% consumer[i] & FaceId %in% faceid[i])))
    }
  }
  print("  Filter packets Ok")
  datatimej.sum = summaryBy (. ~ TimeSec, data=datatimej, FUN=sum)
  datacons.sum = summaryBy (. ~ TimeSec, data=datacons, FUN=sum)
  datatimej.sum$ise = datacons.sum$Packets.sum/datatimej.sum$Packets.sum
  datatimej.sum = cbind(Strategy, datatimej.sum)
  datatimej.sum = subset (datatimej.sum, select = c('Strategy','TimeSec','ise'))
  
  if (!exists('aux'))
  {
    aux = datatimej.sum
  }
  else
  {
    aux = rbind(aux, datatimej.sum)
  }
  print("  Calculus Ok")
}

if (lang == "br")
{
  tit = "Eficiência na Entrega de Dados"
  yl = "Eficiência na Entrega de Dados \n(Pacotes de Dados Recebidos pelo Consumidor / \n Soma de Interesses em Cada Enlace"
  xl = "Tempo (Segundos)"
}else
{
  tit = "Data Delivery Efficiency"
  yl = "Data Delivery Efficiency \n(Consumer-Received Data Packets / \n Sum Of Interests Sent On Each Link)"
  xl = "Time (Seconds)"
}
aux$ise[which(aux$ise=='NaN')] = 0
print (aux)
maxise = max(aux$ise)
aux$ise = aux$ise/maxise
print (maxise)
write.table (aux, "./temp/summary.txt", append=TRUE, row.names = FALSE, col.names = FALSE)

xaxis = subset (aux, TimeSec %in% c(max(aux$TimeSec)))
xaxis = subset (xaxis, select = c('Strategy','ise'))
print (paste("Log varx: ",varx))
xaxis = cbind(varx1,xaxis)
write.table (xaxis, "./temp/summaryXaxis.txt", append=TRUE, row.names = FALSE, col.names = FALSE)

aux2 = subset (aux, (Strategy != 'Omniscient'))

pd <- position_dodge(.1)
g.all <- ggplot (aux2, aes (x=TimeSec, y=ise, shape=Strategy, linetype=Strategy) ) +
  geom_point (size=9) +
  geom_line (size=2) +
  scale_shape(solid=FALSE)+
  scale_linetype_manual(values=c("solid", "dotdash", "dotted", "dashed", "longdash", "twodash"))+
  ylab (yl) +
  theme_bw() +
  theme(legend.position="bottom", text = element_text(family="Times", size = 16), legend.title=element_blank(),   
    axis.text = element_text(family="Times", size = 16)) +
png (paste("./temp/SignallingEfficiency-Time.png", sep = ""), width=600, height=500)
print (g.all)
dev.off ()

