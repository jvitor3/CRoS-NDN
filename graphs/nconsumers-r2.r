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
varx='consxint'
varx2=quote(consxint)

print("Plot summarized curve")

data = read.table ("./temp/summaryXaxis.txt", header=T)
data$Strategy = factor (data$Strategy)    
data$Scenario = factor (data$Scenario) 
 
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

if (varx2 == "consxint")
{
xl = "Number of Consumer Nodes"
data$consxint <- factor (data$consxint) 
}else if (varx2 == "prodxint")
{
xl = "Number of Producer Nodes"
data$prodxint <- factor (data$prodxint) 
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
}else if (varx2 == "rate")
{
xl = "Consumer Interests per Second"
data$rate <- factor (data$rate) 
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
data$fibsize <- factor (data$constime)
}else if (varx2 == "script")
{
xl = "Routing Scheme"
data$script <- factor (data$script)
} 


lttsize=24
g.all <- ggplot (data, aes(x=eval(varx2), y=ise, fill=Scenario, shape=Scenario)) +
#  geom_point (size=5,position=position_dodge(.9)) +
  geom_bar (stat="identity", position="dodge") +
  geom_errorbar (aes(ymin=ise-ci,ymax=ise+ci), width=.8, size=1, position=position_dodge(.9)) +  
  ylab (yl) +
  xlab (xl) +
  scale_fill_grey(name ="") +
#  scale_shape(name ="")+
  theme_bw() +
  theme(legend.position="bottom", text = element_text(family="Times", size = lttsize),     
  axis.text = element_text(family="Times", size = lttsize)) +
png (paste("./temp/mandelbrotnconsumers.png",sep=""), width=500, height=500)
print (g.all)
dev.off ()


