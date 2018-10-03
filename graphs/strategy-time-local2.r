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
stratset=c('CRoS-NDN','null')
varx='topo'
varx2=quote(topo)
ttime=1000
maxXaxis=11

print ("Plot variable time curve.")

data = read.table ("./temp/summarytime.txt", header=T)
#data$Strategy = factor (data$Strategy)    
#data$TimeSec = factor (data$TimeSec)
 
if (lang == "br")
{
  tit = "Evolução do Percentual de Tráfego Útil"
  yl = "Eficiência de Sinalização - ES"
  xl = paste("Parâmetro ", eval(varx2))
  #TODO
}else
{
  tit = "Throughput Efficiency"
  yl = "Throughput Efficiency - TE"
#  xl = paste("Parameter: ", varx2,sep = "")
  xl = "Time (Seconds)"
  if (varx2 == "consxint")
  {
#    xl = "Number of consumers"
    data$consxint <- factor (data$consxint) 
  }
  else if (varx2 == "prodxint")
  {
#    xl = "Number of producers"
    data$nprefix <- factor (data$nprefix) 
  }
  else if (varx2 == "nprefix")
  {
#    xl = "Number of name prefixes"
    data$nprefix <- factor (data$nprefix) 
  }
  else if (varx2 == "consperc")
  {
#    xl = "FIB match fail fraction FF (%)"
    data$consperc <- factor (data$consperc) 
  }
  else if (varx2 == "topo")
  {
#    xl = "Topology number of nodes"
    data$topo <- factor (data$topo) 
  }
  else if (varx2 == "rrate")
  {
#    xl = "Consumer content request rate"
    data$rrate <- factor (data$rrate) 
  }  
  else if (varx2 == "hellorate")
  {
#    xl = "Hello keepalive rate"
    data$hellorate <- factor (data$hellorate)
  }

}

output = summarySE (data, measurevar="ise", groupvars=c("Strategy",paste(varx),"TimeSec") )
print (paste("Output: "))
print (output)

output2 = subset (output, (Strategy != 'Omniscient'))

pd <- position_dodge(.8)
g.all <- ggplot (output2, aes (x=TimeSec, y=ise, color=eval(Strategy), shape=eval(varx2)) ) +
  geom_point (size=9) +
  geom_line (size=2) +
  scale_shape(solid=FALSE)+
#  geom_errorbar (aes(ymin=ise-ci,ymax=ise+ci), width=1, position=position_dodge(.9), size=1) +  
  ylab (yl) +
  xlab (xl) +
  theme_bw() +
  theme(legend.position="bottom", text = element_text(size=23), legend.title=element_blank(),     
  axis.text = element_text(size = 25)) +
png (paste("./temp/ErrorTime.png",sep=""), width=600, height=500)
print (g.all)
dev.off ()


