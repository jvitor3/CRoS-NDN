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
stratset=c('ARPLike', 'CRoS-NDN', 'Flooding', 'NLSRLike', 'Omniscient', 'null')
varx='consperc'
varx2=quote(consperc)
ttime=3000
maxXaxis=11

data = read.table ("./temp/summary.txt", header=T)
data$Strategy = factor (data$Strategy)    
data$TimeSec = factor (data$TimeSec)    
output = summarySE (data, measurevar="ise", groupvars=c("Strategy","TimeSec") )

if (lang == "br")
{
  tit = "Evolução Temporal do Percentual de Tráfego Útil"
  yl = "Eficiência de Sinalização - ES"
  xl = "Tempo - t (Segundos)"
}else
{
  tit = "Throughput Efficiency Time Evolution"
  yl = "Throughput Efficiency - TE"
  xl = "Time - t (Seconds)"
}

output2 = subset (output, (Strategy != 'Omniscient'))

pd <- position_dodge(.1)
g.all <- ggplot (output2, aes (x=TimeSec, y=ise, linetype=Strategy, group=Strategy, shape=Strategy)) +
  geom_point (position=pd, size=9) +
  geom_line (aes(linetype=Strategy), position=pd, size=2) +  
  geom_errorbar (aes(ymin=ise-ci,ymax=ise+ci), width=2, position=pd, size=2) +  
  scale_linetype_manual(values=c("solid", "dotdash", "dotted", "dashed", "longdash", "twodash"))+
  ylab (yl) +
  scale_x_discrete (xl, breaks = seq(0,ttime,by=500), labels = seq(0,ttime,by=500)) +
  theme_bw() +
  scale_linetype_discrete(name  = xl) +
  scale_color_discrete(name  = xl) +
  scale_shape_discrete(name  = xl, solid=FALSE) +  
  theme(legend.position="bottom", text = element_text(size=14), legend.title = element_blank(),
  axis.text = element_text(size = 14), legend.text = element_text(size = 14))  
  #theme(legend.position="bottom", text = element_text(size=21), legend.title=element_blank(),  
  #axis.text = element_text(size = 25), axis.title = element_text(size = 25)) +
png (paste("./temp/Error.png",sep=""), width=600, height=500)
print (g.all)
dev.off ()
