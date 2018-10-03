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
varx='rrate'
varx2=quote(rrate)
ttime=250
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
#  yl = "Throughput Efficiency - TE"
  yl = "Data Delivery Efficiency \n(Consumer-Received Data Packets / \n Sum Of Interest Sent On Each Link)"
#  xl = paste("Parameter: ", varx2,sep = "")
  xlb = "Time (Seconds)"
  xlc = "Routing Strategy"

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
    xl = "Number of Prefixes"
    data$nprefix <- factor (data$nprefix) 
  }
  else if (varx2 == "consperc")
  {
    xl = "Probability of Prefix Change (%)"
    data$consperc <- factor (data$consperc) 
  }
  else if (varx2 == "topo")
  {
    xl = "Topology number of nodes"
    data$topo <- factor (data$topo) 
  }
  else if (varx2 == "rrate")
  {
    xl = "Consumer Interests per Second"
    data$rrate <- factor (data$rrate) 
  }  
  else if (varx2 == "hellorate")
  {
    xl = "Hello frequency (Hz)"
    data$hellorate <- factor (data$hellorate)
  }

}

output = summarySE (data, measurevar="ise", groupvars=c("Strategy",paste(varx),"TimeSec") )
print (paste("Output: "))
print (output)

output2 = subset (output, (Strategy != 'Omniscient'))

pd <- position_dodge(.8)
g.all <- ggplot (output2, aes (x=TimeSec, y=ise, linetype=Strategy, shape=eval(varx2)))+ #,color=eval(varx2))) +
  geom_point (size=5) +
  geom_line (size=1) +
  geom_errorbar (aes(ymin=ise-ci,ymax=ise+ci), width=4, size=1) +  
  ylab (yl) +
  xlab (xlb) +
  theme_bw() +
  scale_linetype_discrete(name  = xlc) +
#  scale_color_discrete(name  = xl) +
  scale_shape_discrete(name  = xl, solid=FALSE) +
#  scale_shape(solid=FALSE)+
  theme(legend.position="bottom", text = element_text(size=14), legend.title = element_text(size=14),
  axis.text = element_text(size = 14), legend.text = element_text(size = 14))
  #labs(linetype = "Hello frequency (Hz)") +
png (paste("./temp/ErrorTime.png",sep=""), width=600, height=500)
print (g.all)
dev.off ()


