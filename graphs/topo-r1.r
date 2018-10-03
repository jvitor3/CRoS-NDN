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
library(gridExtra)
source("./graphs/summarySE.r")
source("./graphs/graph-style.R")
#source("./graphs/variables.r")

constime=0 + 1
lang='en'
stratset=c('CRoS-NDN', 'null')
varx='topo'
varx2=quote(topo)
ttime=250
maxXaxis=11
print("Plot summarized curve")

data = read.table ("./temp/summaryXaxis.txt", header=T)
data$Strategy = factor (data$Strategy)    
 
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
xl = "Network Number of Nodes"
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
} 

output = summarySE (data, measurevar="ise", groupvars=c("Strategy",paste(varx)) )
#pathlength = c(1.60, 2.25, 4.51, 4.57, 4.26) #original
#pathlength = c(2.60, 3.25, 5.51, 5.57, 5.26) #added 1 hop
#pathlengthci = c(0.37, 0.25, 0.028, 0.024, 0.015)
#nlinks = c(4, 12, 366, 350, 731)
#nnodes = c("5", "10", "163", "191", "279")

pathlength = c(5.26, 5.51, 5.57, 2.60, 3.25) #added 1 hop
pathlengthci = c(0.015, 0.028, 0.024, 0.37, 0.25)
nlinks = c(731, 366, 350, 4, 12)
nnodes = c(279, 163, 191, 5, 10)

output = cbind(output, pathlength)
output = cbind(output, pathlengthci)
output = cbind(output, nlinks)
output = cbind(output, nnodes)


output$nnodes <- factor(output$nnodes)
output = orderBy (~-ise, data=output)
print (paste("Output: "))
print (output)

output2 = subset (output, (Strategy != 'Omniscient'))


lttsize=24
#x=eval(varx2)
pd <- position_dodge(.8)
g.a <- ggplot (output2, aes(x=nnodes, y=pathlength)) +
  #geom_point (size=5) +
  geom_bar (stat="identity",position="dodge",width=.5, fill="gray") +
  geom_errorbar (aes(ymin=pathlength-pathlengthci,ymax=pathlength+pathlengthci), width=.5, position=position_dodge(.9), size=1) + 
  #geom_line (size=5) +
  #scale_linetype_manual(values=c("solid"))+  
  #ylab ("Topology\nDistances\n(hops)") +
  ylab ("Distances\n(hops)") +
  #xlab (xl) +
  theme_bw() +
  theme(plot.margin = unit(c(10,2,-42,2),units="points"), legend.position="none", text = element_text(family="Times", size = lttsize), axis.text = element_text(family="Times", size = lttsize)) 
  
g.b <- ggplot (output2, aes(x=nnodes, y=nlinks)) +
  #geom_point (size=5) +
  geom_bar (stat="identity",position="dodge",width=.5, fill="gray") +
  #geom_line (size=5) +
  #scale_linetype_manual(values=c("solid"))+  
  #ylab ("Topology Number\nof Links") +
  ylab ("#Links") +
  #xlab (xl) +
  theme_bw() +
  theme(plot.margin = unit(c(0,2,-42,3),units="points"), legend.position="none", text = element_text(family="Times", size = lttsize), axis.text = element_text(family="Times", size = lttsize))  
  
g.c <- ggplot (output2, aes(x=nnodes, y=ise)) +
  #geom_point (size=5) +
  geom_bar (stat="identity",position="dodge",width=.5, fill="gray") +
  geom_errorbar (aes(ymin=ise-ci,ymax=ise+ci), width=.5, position=position_dodge(.9), size=1) + 
  #scale_linetype_manual(values=c("solid"))+  
  ylab (yl) +
  xlab (xl) +
  theme_bw() +
  theme(plot.margin = unit(c(0,2,1,9),units="points"), legend.position="none", text = element_text(family="Times", size = lttsize), legend.title=element_blank(),     
  axis.text = element_text(family="Times", size = lttsize)) 

#merge all three plots within one grid
png("temp/nnodes.png", width=800, height=800)
grid.arrange(g.a, g.b, g.c,heights = c(1/5, 1.1/5, 2.9/5))
dev.off()



