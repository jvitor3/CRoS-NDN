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
  tit = "Evolução do Percentual de Tráfego Útil"
  yl = "Eficiência de Sinalização - ES"
  xl = paste("Parâmetro ", eval(varx2))
  #TODO
}else
{
  tit = "Throughput Efficiency"
#  yl = "Throughput Efficiency - TE"
  yl = "Data Delivery Efficiency \n(Consumer-Received Data Packets / \n Sum Of Interest Sent On Each Link)"
  xl = paste("Parameter: ", varx2,sep = "")
  if (varx2 == "consxint")
  {
    xl = "Number of consumers"
    data$consxint <- factor (data$consxint) 
  }
  else if (varx2 == "prodxint")
  {
    xl = "Number of producers"
    data$nprefix <- factor (data$nprefix) 
  }
  else if (varx2 == "nprefix")
  {
    xl = "Number of name prefixes"
    data$nprefix <- factor (data$nprefix) 
  }
  else if (varx2 == "consperc")
  {
    xl = "FIB match fail fraction FF (%)"
    data$consperc <- factor (data$consperc) 
  }
  else if (varx2 == "topo")
  {
    xl = "Topology number of nodes"
    data$topo <- factor (data$topo) 
  }
  else if (varx2 == "rate")
  {
    xl = "Consumer content request rate"
    data$rate <- factor (data$rate) 
  }  
  else if (varx2 == "hellorate")
  {
    xl = "Hello keepalive rate"
    data$hellorate <- factor (data$hellorate)
  }

}

output = summarySE (data, measurevar="ise", groupvars=c("Strategy",paste(varx)) )
print (paste("Output: "))
print (output)

output2 = subset (output, (Strategy != 'Omniscient'))

pd <- position_dodge(.8)
g.all <- ggplot (output2, aes(x=eval(varx2), y=ise)) +
  geom_bar (stat="identity",position="dodge") +
  geom_errorbar (aes(ymin=ise-ci,ymax=ise+ci), width=1, position=position_dodge(.9), size=1) +  
  ylab (yl) +
  xlab (xl) +
  theme_bw() +
  theme(legend.position="none", text = element_text(size=14), legend.title=element_blank(),     
  axis.text = element_text(size = 14), legend.text = element_text(size = 14)) +
png (paste("./temp/ErrorXaxis.png",sep=""), width=600, height=500)
print (g.all)
dev.off ()


