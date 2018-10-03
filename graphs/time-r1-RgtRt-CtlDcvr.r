#!/usr/bin/env Rscript

# install.packages ('ggplot2)
library (ggplot2)
# install.packages ('scales')  
library (scales)
# install.packages ('doBy')
library (doBy)
#install.packages('data.table')
library(data.table)
library (plyr)
library(gridExtra)
source("./graphs/summarySE.r")
source("./graphs/graph-style.R")
#source("./graphs/variables.r")

#rrate
#constime
#nprefix
constime=0 + 1
lang='en'
stratset=c('CRoS-NDN','null')
varx='rrate'
varx2=quote(rrate)
ttime=3000
maxXaxis=11
samplei=50

print ("Plot variable time curve.")

data = read.table ("./temp/summarytime.txt", header=T)
#data$Strategy = factor (data$Strategy)    
#data$TimeSec = factor (data$TimeSec)
 
if (lang == "br")
{
  tit = "Eficiência na Entrega de Dados"
  yl = "Eficiência na Entrega de Dados \n(Pacotes de Dados Recebidos pelo Consumidor / \n Soma de Interesses em Cada Enlace"
  xlb = "Tempo (Segundos)"
}else
{
  tit = "Data Delivery Efficiency"
  #yl = "Data Delivery Efficiency \n(Consumer-Received Data Packets / \n Sum Of Interests Sent On Each Link)"
  yl = "Data Delivery Efficiency"
  xlb = "Time (Seconds)"
}

if (varx2 == "consxint")
{
xl = "Number of Consumer Nodes"
data$consxint <- factor (data$consxint) 
}else if (varx2 == "prodxint")
{
xl = "Number of Producer Nodes"
data$nprefix <- factor (data$nprefix) 
}else if (varx2 == "nprefix")
{
xl = "Number of Prefixes"
data$nprefix <- factor (data$nprefix) 
}else if (varx2 == "consperc")
{
xl = "Probability of Prefix Change (%)"
data$consperc <- factor (data$consperc) 
}else if (varx2 == "topo")
{
xl = "Topology Number of Nodes"
data$topo <- factor (data$topo) 
}else if (varx2 == "rrate")
{
xl = "Consumer Interests per Second"
data$rrate <- factor (data$rrate) 
}else if (varx2 == "hellorate")
{
xl = "Hello Frequency (Hz)"
data$hellorate <- factor (data$hellorate)
}else if (varx2 == "fibsize")
{
xl = "Maximum FIB Size"
data$fibsize <- factor (data$fibsize)
}else if (varx2 == "constime")
{
xl = "Producer Roaming Interval (Seconds)"
data$constime <- factor (data$constime)
} 

output = summarySE (data, measurevar="ise", groupvars=c("Strategy",paste(varx),"TimeSec") )
print (paste("Output: "))
print (output)

lttsize=24
yvar=quote(DiscoveryRequests)
output1 = summarySE (data, measurevar=paste(yvar,sep = ""), groupvars=c("Strategy",paste(varx),"TimeSec") )
pd <- position_dodge(.8)
g.a <- ggplot (output1, aes (x=TimeSec, y=eval(yvar), linetype=eval(varx2)))+#, shape=eval(varx2)))+ #,color=eval(varx2))) +
#  geom_point (size=5) +
  geom_line (size=1) +
#  geom_errorbar (aes(ymin=(eval(yvar)-ci),ymax=(eval(yvar)+ci)), width=25, size=1) +  
  #ylab ("Controller\nDiscovery\n(Interests/s)") +
  ylab ("Interests/s") +
  theme_bw() +
  scale_linetype_discrete(name  = xl) +
#  scale_color_discrete(name  = xl) +
#  scale_shape_discrete(name  = xl, solid=FALSE) +
#  scale_shape(solid=FALSE)+
  theme(plot.margin = unit(c(10,2,-42,1),units="points"),legend.position="none", text = element_text(family="Times", size = lttsize), legend.title = element_text(family="Times", size = lttsize),
  axis.text = element_text(family="Times", size = lttsize), legend.text = element_text(family="Times", size = lttsize))

yvar1=quote(RRouterRequests)
output2 = summarySE (data, measurevar=paste(yvar1,sep = ""), groupvars=c("Strategy",paste(varx),"TimeSec") )
g.b <- ggplot (output2, aes (x=TimeSec, y=eval(yvar1), linetype=eval(varx2)))+#, shape=eval(varx2)))+ #,color=eval(varx2))) +
#  geom_point (size=5) +
  geom_line (size=1) +
#  geom_errorbar (aes(ymin=(eval(yvar1)-ci),ymax=(eval(yvar1)+ci)), width=25, size=1) +  
  #ylab ("Register\nRouter\n(Interests/s)") +
  ylab ("Interests/s") +
  theme_bw() +
  scale_linetype_discrete(name  = xl) +
#  scale_color_discrete(name  = xl) +
#  scale_shape_discrete(name  = xl, solid=FALSE) +
#  scale_shape(solid=FALSE)+
  theme(plot.margin = unit(c(0,2,-42,1),units="points"),legend.position="none", text = element_text(family="Times", size = lttsize), legend.title = element_text(family="Times", size = lttsize),
  axis.text = element_text(family="Times", size = lttsize), legend.text = element_text(family="Times", size = lttsize))  

output3 = subset (output, (Strategy != 'Omniscient'))  
g.c <- ggplot (output3, aes (x=TimeSec, y=ise, linetype=eval(varx2)))+#, shape=eval(varx2)))+ #,color=eval(varx2))) +
#  geom_point (size=5) +
  geom_line (size=1) +
#  geom_errorbar (aes(ymin=ise-ci,ymax=ise+ci), width=25, size=1) +  
  ylab (yl) +
  xlab (xlb) +
  theme_bw() +
  scale_linetype_discrete(name  = xl) +
#  scale_color_discrete(name  = xl) +
#  scale_shape_discrete(name  = xl, solid=FALSE) +
#  scale_shape(solid=FALSE)+
  theme(plot.margin = unit(c(0,2,1,1),units="points"),legend.position="bottom", text = element_text(family="Times", size = lttsize), legend.title = element_text(family="Times", size = lttsize),
  axis.text = element_text(family="Times", size = lttsize), legend.text = element_text(family="Times", size = lttsize))
  
#merge all three plots within one grid
png("temp/cirate.png", width=800, height=800)
grid.arrange(g.a, g.b, g.c,heights = c(1/5, 1/5, 3/5))  
dev.off()


