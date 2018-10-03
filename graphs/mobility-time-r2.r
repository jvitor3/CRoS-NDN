#!/usr/bin/env Rscript
# install.packages ('ggplot2')  
library (ggplot2)
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

constime=0 + 1
lang='en'
varx='prodxint'
varx2=quote(prodxint)
ttime=501

print("strategy-95v1.r started!")

data = read.table ("./temp/summary.txt", header=T)
data$Strategy = factor (data$Strategy)    
data$TimeSec = factor (data$TimeSec)    
output = summarySE (data, measurevar="ise", groupvars=c("Strategy","TimeSec") )

if (lang == "br")
{
  tit = "Eficiência na Entrega de Dados"
  yl = "Eficiência na Entrega de Dados \n(Pacotes de Dados Recebidos pelo Consumidor / \n Soma de Interesses em Cada Enlace"
  xl = "Tempo (Segundos)"
}else
{
  tit = "Data Delivery Efficiency"
  #yl = "Data Delivery Efficiency \n(Consumer-Received Data Packets / \n Sum Of Interests Sent On Each Link)"
  yl = "Data Delivery Efficiency"
  xl = "Time (Seconds)"
}

output2 = subset (output, (Strategy != 'Omniscient'))

if (ttime < 600)
{
  brks = 50
}else
{
  brks = 500
}

pd <- position_dodge(.1)
lttsize=24
g.all <- ggplot (output2, aes (x=TimeSec, y=ise, group=Strategy, shape=Strategy)) +
  #geom_point (size=5) +
  geom_line (aes(linetype=Strategy), size=1) +  
  #geom_errorbar (aes(ymin=ise-ci,ymax=ise+ci), width=1, size=1) +  
  scale_shape(solid=FALSE)+ 
  scale_linetype_manual(values=c("solid", "dotdash", "dotted", "dashed", "longdash", "twodash"))+
  ylab (yl) +
  scale_x_discrete (xl, breaks = seq(0,ttime,by=brks), labels = seq(0,ttime,by=brks)) +
  theme_bw() +
  theme(legend.position="bottom", text = element_text(family="Times", size = lttsize), legend.title=element_blank(),  
  axis.text = element_text(family="Times", size = lttsize), axis.title = element_text(family="Times", size = lttsize)) +
png (paste("./temp/MobilityTime-r2.png",sep=""), width=500, height=500)
print (g.all)
dev.off ()
