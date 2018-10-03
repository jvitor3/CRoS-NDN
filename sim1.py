#!/usr/bin/env python
# -*- Mode: python; py-indent-offset: 4; indent-tabs-mode: nil; coding: utf-8; -*-
from datetime import datetime 
from subprocess import call 
from sys import argv 
import os 
import subprocess 
import workerpool 
import multiprocessing 
import argparse 
import random
import copy
import sys, traceback
######################################################################
######################################################################
######################################################################
parser = argparse.ArgumentParser(description='Simulation runner') 

parser.add_argument('scenarios', metavar='scenario', type=str, nargs='*', 
			help='Scenario to run') 

parser.add_argument('-l', '--list', dest="list", action='store_true', default=False, 
			help='Get list of available scenarios') 

parser.add_argument('-s', '--simulate', dest="simulate", action='store_true', default=False,
                    help='Run simulation and postprocessing (false by default)') 

parser.add_argument('-g', '--no-graph', dest="graph", action='store_false', default=True,
                    help='Do not build a graph for the scenario (builds a graph by default)')
                    
parser.add_argument('-r', '--rm', dest="rmresults", action='store_true', default=False,
                    help='Clean and delete all files in -temp- folder')
                    
parser.add_argument('-u', '--upload', dest="upload", action='store_true', default=False,
                    help='Move gross results to localresults folder and processed results to results folder') 

args = parser.parse_args() 

if not args.list and not args.rmresults and not args.upload and len(args.scenarios)==0:
    print "ERROR: at least one scenario need to be specified"
    parser.print_help()
    exit (1) 

dttm = datetime.now()
folder = dttm.strftime("%Y%m%d-%H%M%S")	
	
if args.list:
    print "Available scenarios: " 
else:
    if args.simulate:
        print "Simulating the following scenarios: " + ",".join (args.scenarios)
        os.mkdir("./results/%s" % folder)
        os.mkdir("./localresults/%s" % folder)
	
    if args.graph:
        print "Building graphs for the following scenarios: " + ",".join (args.scenarios)
        
    if args.rmresults:
        print "Cleaning all files in -temp- folder!"
        print "Erasing directory: " + os.getcwd() + "/temp/*"
        os.system("rm -r ./temp/*")
        exit(0)
    if args.upload:
        print "Move gross results to localresults folder and processed results to results folder"
        now = datetime.now()
        nowstr = now.strftime("%Y%m%d-%H%M%S")
        os.mkdir("./localresults/%s" % nowstr)
        os.mkdir("./results/%s" % nowstr)
        os.system("mv ./temp/var* ./localresults/%s/" % nowstr)
        os.system("mv ./temp/* ./results/%s/" % nowstr)
        exit(0)
        
######################################################################
######################################################################
######################################################################
class SimulationJob (workerpool.Job):
    "Job to simulate things"
    def __init__ (self, cmdline):
        self.cmdline = cmdline
    def run (self):
        print (" ".join (self.cmdline))
        subprocess.check_call(" ".join (self.cmdline), shell=True)
        subprocess.call (" ".join (self.cmdline), shell=True) 

pool = workerpool.WorkerPool(size = multiprocessing.cpu_count() - 1) 

class Processor:
	def run (self):
		if args.list:
			print " " + self.name
			return

		if "all" not in args.scenarios and self.name not in args.scenarios:
			return
		if args.list:
			pass
		else:
			if args.simulate:
				self.simulate ()
				pool.join ()
			if args.graph:
				self.graph ()
			if ((len(args.scenarios) > 1) or ("all" in args.scenarios)):
				self.movefiles()				

	def graph (self):
		os.system("rm ./temp/*.png")
		os.system("rm ./temp/*.txt")
		self.postprocess ()
		
	def movefiles (self):
		print "Move gross results to localresults folder"
		fdname = "%s/%s/" % (folder,self.name)
		os.mkdir("./results/" + fdname)
		#os.system("cp ./temp/*.png ./results/" + fdname)		
		os.system("cp ./temp/*.txt ./results/" + fdname)
		os.system("cp ./temp/*.log ./results/" + fdname)
		os.mkdir("./localresults/" + fdname)
		#os.mkdir("./results/%s" % nowstr)
		os.system("mv ./temp/var* ./localresults/" + fdname)
		os.system("mv ./temp/* ./localresults/" + fdname)
	
class Base:
	def __init__ (self, name, script, stratset, nprefix, topo, controller, rate, hellorate, consxint, cons, prodxint, prod, consprefs, consperc, 
ttime, failtime, constime, regrate, fibsize, samplei, lang, varx, maxXaxis, suffix, runs, counter, varset):
		self.name = name
		self.script = script
		self.stratset = stratset
		self.nprefix = nprefix
		self.topo = topo
		self.controller = controller
		self.rate = rate
		self.hellorate = hellorate
		self.consxint = consxint
		self.cons = cons
		self.prodxint = prodxint
		self.prod = prod
		self.consprefs = consprefs
		self.consperc = consperc
		self.ttime = ttime
		self.failtime = failtime
		self.constime = constime
		self.regrate = regrate
		self.fibsize = fibsize
		self.samplei = samplei
		self.lang = lang
		self.varx = varx
		self.maxXaxis = maxXaxis
		self.suffix = suffix
		self.runs = runs
		self.counter = counter
		self.varset = varset

	def writefiles (self):
		f = open("graphs/variables.r","w")
		f.write("constime=%s + 1" % self.constime)
		f.write("\nlang='%s'" % self.lang)
		f.write("\nstratset=c(")
		for strat in self.stratset:
			f.write("'%s'," % strat)
		f.write("'null')")
		f.write("\nvarx='%s'" % self.varx)
		f.write("\nvarx2=quote(%s)" % self.varx)
		f.write("\nttime=%s" % self.ttime)
		f.write("\nmaxXaxis=%s" % self.maxXaxis)
		f.write("\nsamplei=%s" % self.samplei)
		f.close()
		if not ((os.path.exists("./temp/memcpu.log"))):
			f = open("./temp/memcpu.log", "a")
			f.write("Strategy\t%s\tcputime\tmemory\n" % self.varx)
			f.close()
			
	def writecmdline (self):
		self.seed = random.randint(1,10000)
		os.mkdir("./temp/%s" % self.suffix)
		f = open("./graphs/varx.r","w")
		f.write("varx1='%s'" % self.counter)
		f.write("\nsuffix='%s'" % self.suffix)
		f.close()
		for strat in self.stratset:
			cmdline = ["/usr/bin/time --format='%s\t%s\t" % (strat, self.counter), "%E\t%M'", "-o ./temp/memcpu.log -a", "./ptimized/%s" % self.script,"--strategy=%s" % strat, "--nprefix=%s" % 
self.nprefix, "--topo=%s" % self.topo, "--consumer=%s" % self.cons, "--producer=%s" % self.prod, "--controller=%s" % self.controller, "--rate=%s" % 
self.rate, "--hellorate=%s" % self.hellorate, "--consxint=%s" % self.consxint, "--prodxint=%s" % self.prodxint, "--consprefs=%s" % self.consprefs, 
"--consperc=%s" % self.consperc, "--time=%s" % self.ttime, "--failtime=%s" % self.failtime, "--constime=%s" % self.constime, "--regrate=%s" % 
self.regrate, "--fibsize=%s" % self.fibsize, "--seed=%s" % self.seed, "--samplei=%s" % self.samplei, "--suffix=%s" % self.suffix,]
			f = open("./temp/cmdline.log","a")
			f.write("\nCMDLINE: "+ " ".join (cmdline))
			f.close()
			job = SimulationJob (cmdline)
			pool.put (job)

	def writecmdline1 (self):
		self.seed = random.randint(1,10000)
		os.mkdir("./temp/%s" % self.suffix)
		f = open("./graphs/varx.r","w")
		f.write("varx1='%s'" % self.counter)
		f.write("\nsuffix='%s'" % self.suffix)
		f.close()
		for strat in self.stratset:
			cmdline = ["/usr/bin/time --format='%s\t%s\t" % (strat, self.counter), "%E\t%M'", "-o ./temp/memcpu.log -a", "./ptimized/%s" % strat,"--strategy=%s" % strat, "--nprefix=%s" % 
self.nprefix, "--topo=%s" % self.topo, "--consumer=%s" % self.cons, "--producer=%s" % self.prod, "--controller=%s" % self.controller, "--rate=%s" % 
self.rate, "--hellorate=%s" % self.hellorate, "--consxint=%s" % self.consxint, "--prodxint=%s" % self.prodxint, "--consprefs=%s" % self.consprefs, 
"--consperc=%s" % self.consperc, "--time=%s" % self.ttime, "--failtime=%s" % self.failtime, "--constime=%s" % self.constime, "--regrate=%s" % 
self.regrate, "--fibsize=%s" % self.fibsize, "--seed=%s" % self.seed, "--samplei=%s" % self.samplei, "--suffix=%s" % self.suffix,]
			f = open("./temp/cmdline.log","a")
			f.write("\nCMDLINE: "+ " ".join (cmdline))
			f.close()
			job = SimulationJob (cmdline)
			pool.put (job)		
			
	def writedatafile (self):
		f = open("temp/summarytime.txt","w")
		if (len(self.stratset) == 1):
			f.write("%s Strategy TimeSec ise RouteRequests DiscoveryRequests RRouterRequests RContentRequests\n" % self.varx)
		else:
			f.write("%s Strategy TimeSec ise\n" % self.varx)
		f.close()
		f = open("temp/summaryXaxis.txt","w")
		f.write("%s Strategy ise\n" % self.varx)
		f.close()
		f = open("temp/summaryStrategyXaxis.txt","w")
		f.write("Strategy %s N ise sd se ci\n" % self.varx)
		f.close()		
		f = open("temp/summaryStrategyTime.txt","w")
		f.write("%s Strategy TimeSec ise sd se ci\n" % self.varx)
		f.close()		
		f = open("temp/summaryStrategyTimeRouteRequests.txt","w")
		f.write("%s Strategy TimeSec RouteRequests sd se ci\n" % self.varx)
		f.close()
		f = open("temp/summaryStrategyTimeDiscoveryRequests.txt","w")
		f.write("%s Strategy TimeSec DiscoveryRequests sd se ci\n" % self.varx)
		f.close()
		f = open("temp/summaryStrategyTimeRRouterRequests.txt","w")
		f.write("%s Strategy TimeSec RRouterRequests sd se ci\n" % self.varx)
		f.close()
		f = open("temp/summaryStrategyTimeRContentRequests.txt","w")
		f.write("%s Strategy TimeSec RContentRequests sd se ci\n" % self.varx)
		f.close()		
	
	def processdata (self):
		self.writefiles()
		self.writedatafile()
		for item in self.varset:
			f = open("temp/summary.txt","w")
			f.write("Strategy TimeSec ise\n")
			f.close()
			run = 0
			while (run < self.runs):
				self.suffix = "var-%s-value-%s-run-%s" % (self.varx, item, run)
				f = open("./graphs/varx.r","w")
				f.write("varx1='%s'" % item)
				f.write("\nsuffix='%s'" % self.suffix)
				f.close()
				subprocess.call ("./graphs/strategy.r", shell=True)
				os.rename("temp/SignallingEfficiency-Time.png","temp/SignallingEfficiency-Time-%s.png" % self.suffix)
				run = run + 1
			subprocess.call ("./graphs/strategy-95.r", shell=True)
			os.rename("temp/summary.txt","temp/summary-%s.txt" % self.suffix)
			os.rename("temp/Error.png","temp/Error-%s.png" % self.suffix)
	        subprocess.call ("./graphs/strategy-xaxis.r", shell=True)

	def processdata1 (self):
		self.writefiles()
		self.writedatafile()
		for item in self.varset:
			#f = open("temp/summary.txt","w")
			#f = open("temp/summarytime.txt","a")
			#f.write("Strategy TimeSec ise\n")
			#f.write("Strategy TimeSec ise RouteRequests DiscoveryRequests RRouterRequests RContentRequests\n")
			#f.close()
			run = 0
			while (run < self.runs):
				self.suffix = "var-%s-value-%s-run-%s" % (self.varx, item, run)
				f = open("./graphs/varx.r","w")
				f.write("varx1='%s'" % item)
				f.write("\nsuffix='%s'" % self.suffix)
				f.close()
				subprocess.call ("./graphs/summaryRound.r", shell=True)
				run = run + 1
		subprocess.call ("./graphs/summaryStrategy.r", shell=True)
		subprocess.call ("./graphs/summaryStrategyTime.r", shell=True)
		
class OneRunOneStrategy (Processor):
	def __init__ (self, parameters):
		self.parameters = parameters
		self.name = parameters.name
	def simulate (self):
		self.parameters.writefiles()
		self.parameters.writecmdline()
	
	def postprocess (self):
		self.parameters.writefiles()
		self.parameters.writedatafile()
		subprocess.call ("./graphs/strategy1.r", shell=True)
		pass 

class ConsumerRate (Processor):
	def __init__ (self, parameters):
		self.parameters = parameters
		self.name = parameters.name
		
	def simulate (self):
		self.parameters.writefiles()
		for counter in self.parameters.varset:
			run = 0
			while (run < self.parameters.runs):	
				self.parameters.suffix = "var-%s-value-%s-run-%s" % (self.parameters.varx, counter,run)
				self.parameters.rate = counter
				self.parameters.counter = counter
				self.parameters.writecmdline()
				run = run + 1
	
	def postprocess (self):
		self.parameters.processdata1() 
		
class ConsumerRate2 (Processor):
	def __init__ (self, parameters):
		self.parameters = parameters
		self.name = parameters.name
		
	def simulate (self):
		self.parameters.writefiles()
		for counter in self.parameters.varset:
			run = 0
			while (run < self.parameters.runs):
				self.parameters.cons = random.randint(1,10000)
				self.parameters.prod = random.randint(1,10000)
				self.parameters.controller = random.randint(1,10000)			
				self.parameters.suffix = "var-%s-value-%s-run-%s" % (self.parameters.varx, counter,run)
				self.parameters.rate = counter
				self.parameters.counter = counter
				self.parameters.writecmdline()
				run = run + 1
	
	def postprocess (self):
		self.parameters.processdata1() 
		
class ConsumerRate3 (Processor):
	def __init__ (self, parameters):
		self.parameters = parameters
		self.name = parameters.name
		
	def simulate (self):
		self.parameters.writefiles()
		for counter in self.parameters.varset:
			run = 0
			while (run < self.parameters.runs):
				self.parameters.cons = random.randint(1,10000)
				self.parameters.prod = random.randint(1,10000)
				self.parameters.controller = random.randint(1,10000)			
				self.parameters.suffix = "var-%s-value-%s-run-%s" % (self.parameters.varx, counter,run)
				self.parameters.rate = counter
				self.parameters.counter = counter
				self.parameters.writecmdline1()
				run = run + 1
	
	def postprocess (self):
		self.parameters.processdata1() 		
		subprocess.call ("./graphs/cachesize-time-r2.r", shell=True)
		subprocess.call ("./graphs/nscripts-cachesize-xaxisv1.r", shell=True)		

class HelloRate (Processor):
	def __init__ (self, parameters):
		self.parameters = parameters
		self.name = parameters.name
		
	def simulate (self):
		self.parameters.writefiles()
		for counter in self.parameters.varset:
			run = 0
			while (run < self.parameters.runs):
				self.parameters.suffix = "var-%s-value-%s-run-%s" % (self.parameters.varx, counter,run)
				self.parameters.hellorate = counter
				self.parameters.counter = counter
				self.parameters.writecmdline()
				run = run + 1
	
	def postprocess (self):
		self.parameters.processdata1()

class Topo (Processor):
	def __init__ (self, parameters):
        	self.parameters = parameters
	        self.name = parameters.name

	def simulate (self):
        	self.parameters.writefiles()
	        for counter in self.parameters.varset:
        	    run = 0
	            while (run < self.parameters.runs):
				self.parameters.cons = random.randint(1,10000)
				self.parameters.prod = random.randint(1,10000)
				self.parameters.controller = random.randint(1,10000)
				self.parameters.suffix = "var-%s-value-%s-run-%s" % (self.parameters.varx, counter,run)
				self.parameters.topo = counter
				self.parameters.counter = counter
				self.parameters.writecmdline()
				run = run + 1

	def postprocess (self):
		self.parameters.processdata1()

class Topo1 (Processor):
	def __init__ (self, parameters):
        	self.parameters = parameters
	        self.name = parameters.name

	def simulate (self):
        	self.parameters.writefiles()
	        for counter in self.parameters.varset:
        	    run = 0
	            while (run < self.parameters.runs):
				self.parameters.cons = random.randint(1,10000)
				self.parameters.prod = random.randint(1,10000)
				self.parameters.controller = random.randint(1,10000)
				self.parameters.suffix = "var-%s-value-%s-run-%s" % (self.parameters.varx, counter,run)
				self.parameters.topo = counter
				self.parameters.counter = counter
				self.parameters.writecmdline1()
				run = run + 1

	def postprocess (self):
		self.parameters.processdata1()	
		subprocess.call ("./graphs/cachesize-time-r2.r", shell=True)
		subprocess.call ("./graphs/nscripts-cachesize-xaxisv1.r", shell=True)		
		
class PrefixChangeProbability (Processor):
	def __init__ (self, parameters):
		self.parameters = parameters
		self.name = parameters.name
		
	def simulate (self):
		self.parameters.writefiles()
		for counter in self.parameters.varset:
			run = 0
			while (run < self.parameters.runs):
				self.parameters.suffix = "var-%s-value-%s-run-%s" % (self.parameters.varx, counter,run)
				self.parameters.consperc = counter
				self.parameters.counter = counter
				self.parameters.writecmdline()
				run = run + 1
	
	def postprocess (self):
		self.parameters.processdata1()				

class NumberofPrefixes (Processor):
	def __init__ (self, parameters):
		self.parameters = parameters
		self.name = parameters.name
		
	def simulate (self):
		self.parameters.writefiles()
		for counter in self.parameters.varset:
			run = 0
			while (run < self.parameters.runs):
				self.parameters.suffix = "var-%s-value-%s-run-%s" % (self.parameters.varx, counter,run)
				self.parameters.nprefix = counter
				self.parameters.consprefs = counter
				self.parameters.counter = counter
				self.parameters.writecmdline()
				run = run + 1
	
	def postprocess (self):
		self.parameters.processdata1()

class NumberofPrefixes2 (Processor):
	def __init__ (self, parameters):
		self.parameters = parameters
		self.name = parameters.name

		
	def simulate (self):
		self.parameters.writefiles()
		nofixedconsprefs = False
		if (self.parameters.nprefix == self.parameters.consprefs):
			nofixedconsprefs = True
		for counter in self.parameters.varset:
			run = 0
			while (run < self.parameters.runs):
				self.parameters.cons = random.randint(1,10000)
				self.parameters.prod = random.randint(1,10000)
				self.parameters.controller = random.randint(1,10000)
				self.parameters.suffix = "var-%s-value-%s-run-%s" % (self.parameters.varx, counter,run)
				self.parameters.nprefix = counter
				if (nofixedconsprefs):
					self.parameters.consprefs = counter
				self.parameters.counter = counter
				self.parameters.writecmdline()
				run = run + 1
	
	def postprocess (self):
		self.parameters.processdata1()		

class FIBSize (Processor):
	def __init__ (self, parameters):
		self.parameters = parameters
		self.name = parameters.name
		
	def simulate (self):
		self.parameters.writefiles()
		for counter in self.parameters.varset:
			run = 0
			while (run < self.parameters.runs):
				self.parameters.suffix = "var-%s-value-%s-run-%s" % (self.parameters.varx, counter,run)
				self.parameters.fibsize = counter
				self.parameters.counter = counter
				self.parameters.writecmdline()
				run = run + 1
	
	def postprocess (self):
		self.parameters.processdata1()	

class FIBSize2 (Processor):
	def __init__ (self, parameters):
		self.parameters = parameters
		self.name = parameters.name
		
	def simulate (self):
		self.parameters.writefiles()
		for counter in self.parameters.varset:
			run = 0
			while (run < self.parameters.runs):
				self.parameters.cons = random.randint(1,10000)
				self.parameters.prod = random.randint(1,10000)
				self.parameters.controller = random.randint(1,10000)
				self.parameters.suffix = "var-%s-value-%s-run-%s" % (self.parameters.varx, counter,run)
				self.parameters.fibsize = counter
				self.parameters.counter = counter
				self.parameters.writecmdline()
				run = run + 1
	
	def postprocess (self):
		self.parameters.processdata1()		
		
class FIBSize3 (Processor):
	def __init__ (self, parameters):
		self.parameters = parameters
		self.name = parameters.name
		
	def simulate (self):
		self.parameters.writefiles()
		for counter in self.parameters.varset:
			run = 0
			while (run < self.parameters.runs):
				self.parameters.cons = random.randint(1,10000)
				self.parameters.prod = random.randint(1,10000)
				self.parameters.controller = random.randint(1,10000)
				self.parameters.suffix = "var-%s-value-%s-run-%s" % (self.parameters.varx, counter,run)
				self.parameters.fibsize = counter
				self.parameters.counter = counter
				self.parameters.nprefix = counter*30
				self.parameters.consprefs = counter*30
				self.parameters.writecmdline()
				run = run + 1
	
	def postprocess (self):
		self.parameters.processdata1()			

class FIBSize4 (Processor):
	def __init__ (self, parameters, factor):
		self.parameters = parameters
		self.name = parameters.name
		self.factor = factor
		
	def simulate (self):
		self.parameters.writefiles()
		for counter in self.parameters.varset:
			run = 0
			while (run < self.parameters.runs):
				self.parameters.cons = random.randint(1,10000)
				self.parameters.prod = random.randint(1,10000)
				self.parameters.controller = random.randint(1,10000)
				self.parameters.suffix = "var-%s-value-%s-run-%s" % (self.parameters.varx, counter,run)
				self.parameters.fibsize = counter
				self.parameters.counter = counter
				self.parameters.nprefix = counter*self.factor
				self.parameters.consprefs = counter*self.factor
				self.parameters.writecmdline()
				run = run + 1
	
	def postprocess (self):
		self.parameters.processdata1()		
		
class NConsumers (Processor):
	def __init__ (self, parameters):
		self.parameters = parameters
		self.name = parameters.name
		
	def simulate (self):
		self.parameters.writefiles()
		for counter in self.parameters.varset:
			run = 0
			while (run < self.parameters.runs):
				self.parameters.cons = random.randint(1,10000)
				self.parameters.prod = random.randint(1,10000)
				self.parameters.controller = random.randint(1,10000)
				self.parameters.suffix = "var-%s-value-%s-run-%s" % (self.parameters.varx, counter,run)
				self.parameters.consxint = counter
				self.parameters.counter = counter
				self.parameters.writecmdline()
				run = run + 1
	
	def postprocess (self):
		self.parameters.processdata1()	

class NConsumersFixedRate (Processor):
	def __init__ (self, parameters):
		self.parameters = parameters
		self.name = parameters.name
		
	def simulate (self):
		self.parameters.writefiles()
		for counter in self.parameters.varset:
			run = 0
			while (run < self.parameters.runs):
				self.parameters.cons = random.randint(1,10000)
				self.parameters.prod = random.randint(1,10000)
				self.parameters.controller = random.randint(1,10000)
				self.parameters.suffix = "var-%s-value-%s-run-%s" % (self.parameters.varx, counter,run)
				self.parameters.consxint = counter
				self.parameters.counter = counter
				self.parameters.rate = 100.0/counter
				self.parameters.writecmdline()
				run = run + 1
	
	def postprocess (self):
		self.parameters.processdata1()			

class NProducers (Processor):
	def __init__ (self, parameters):
		self.parameters = parameters
		self.name = parameters.name
		
	def simulate (self):
		self.parameters.writefiles()
		for counter in self.parameters.varset:
			run = 0
			while (run < self.parameters.runs):
				self.parameters.cons = random.randint(1,10000)
				self.parameters.prod = random.randint(1,10000)
				self.parameters.controller = random.randint(1,10000)
				self.parameters.suffix = "var-%s-value-%s-run-%s" % (self.parameters.varx, counter,run)
				self.parameters.prodxint = counter
				self.parameters.counter = counter
				self.parameters.writecmdline()
				run = run + 1
	
	def postprocess (self):
		self.parameters.processdata1()		
		
class Mobility (Processor):
	def __init__ (self, parameters):
		self.parameters = parameters
		self.name = parameters.name
		
	def simulate (self):
		self.parameters.writefiles()
		for counter in self.parameters.varset:
			run = 0
			while (run < self.parameters.runs):
				self.parameters.suffix = "var-%s-value-%s-run-%s" % (self.parameters.varx, counter,run)
				self.parameters.constime = counter
				self.parameters.counter = counter
				self.parameters.writecmdline()
				run = run + 1
	
	def postprocess (self):
		self.parameters.processdata1()	

class RegisterPrefixRate (Processor):
	def __init__ (self, parameters):
		self.parameters = parameters
		self.name = parameters.name
		
	def simulate (self):
		self.parameters.writefiles()
		for counter in self.parameters.varset:
			run = 0
			while (run < self.parameters.runs):
				self.parameters.suffix = "var-%s-value-%s-run-%s" % (self.parameters.varx, counter,run)
				self.parameters.regrate = counter
				self.parameters.counter = counter
				self.parameters.writecmdline()
				run = run + 1
	
	def postprocess (self):
		self.parameters.processdata1()
		
class NScripts (Processor):
	def __init__ (self, parameters):
		self.parameters = parameters
		self.name = parameters.name
		
	def simulate (self):
		self.parameters.writefiles()
		for counter in self.parameters.varset:
			run = 0
			while (run < self.parameters.runs):
				self.parameters.cons = random.randint(1,10000)
				self.parameters.prod = random.randint(1,10000)
				self.parameters.controller = random.randint(1,10000)
				self.parameters.suffix = "var-%s-value-%s-run-%s" % (self.parameters.varx, counter,run)
				self.parameters.script = counter
				self.parameters.counter = counter
				self.parameters.writecmdline()
				run = run + 1
	
	def postprocess (self):
		self.parameters.processdata1()
		subprocess.call ("./graphs/nscripts-time-r2.r", shell=True)
		subprocess.call ("./graphs/nscripts-xaxisv1.r", shell=True)

class CacheSize (Processor):
	def __init__ (self, parameters):
		self.parameters = parameters
		self.name = parameters.name
		
	def simulate (self):
		self.parameters.writefiles()
		for counter in self.parameters.varset:
			run = 0
			while (run < self.parameters.runs):
				self.parameters.cons = random.randint(1,10000)
				self.parameters.prod = random.randint(1,10000)
				self.parameters.controller = random.randint(1,10000)
				self.parameters.suffix = "var-%s-value-%s-run-%s" % (self.parameters.varx, counter,run)
				self.parameters.consperc = counter
				self.parameters.counter = counter
				self.parameters.writecmdline1()					
				run = run + 1
	
	def postprocess (self):
		self.parameters.processdata1()
		subprocess.call ("./graphs/nscripts-cachesize-time-r2.r", shell=True)
		subprocess.call ("./graphs/nscripts-cachesize-xaxisv1.r", shell=True)
		
try:

	stratset1=["ARPLike", "CRoS-NDN", "Flooding", "NLSRLike", "Omniscient", "OSPFLike",]
	stratset2=["ARPLike", "CRoS-NDN", "NLSRLike", "OSPFLike",]
	stratset3=["ARPLike", "CRoS-NDN", "OSPFLike",]
	stratset4=["CRoS-NDN", "NLSRLike",]
	stratset5=["CRoS-NDN",]
	stratset6=["NLSRLike",]

    # Simulation, processing, and graph building
	baseparameters = Base (name="base",
					script="revista2b",			
					stratset=stratset2,
					nprefix=5,
					topo="topo-3-way.txt",
					controller=random.randint(1,10000),
					rate=20,
					hellorate=0.1,
					consxint=1,
					cons = random.randint(1,10000),
					prodxint=1,
					prod = random.randint(1,10000),
					consprefs=5,
					consperc=10,
					ttime=501,
					failtime=0,
					constime=0,
					regrate=20,
					fibsize=100,
					samplei=50,
					lang="en",
					varx="rrate",
					maxXaxis=11,
					suffix="single-run",
					runs=1,
					counter=1,
					varset=[20,])
	baseexperiment = OneRunOneStrategy (parameters = baseparameters)
	baseexperiment.run ()
	
	convergencepar = copy.deepcopy(baseparameters)
	convergencepar.name = "convergence-delay"
	convergencepar.controller="Ctr1"
	convergencepar.cons = "Src1"
	convergencepar.prod = "Prd1"
	convergencepar.script="revista2-convergenceb"
	convergencepar.failtime = 2
	convergencepar.ttime = 201
	convergencepar.runs = 1
	convergencepar.samplei = 5
	convergenceexp = OneRunOneStrategy (parameters = convergencepar)
	convergenceexp.run() 
	
	hellorate = [0.2, 0.1, 0.05]
	helloparameters1 = copy.deepcopy(convergencepar)
	helloparameters1.name = "helloratedetail"
	helloparameters1.varset = hellorate
	helloparameters1.varx = "hellorate"
	helloparameters1.runs = 20
	helloexperiment1 = HelloRate (parameters = helloparameters1)
	helloexperiment1.run ()	
	
	numberofprefixes = [2, 20, 200,]
	numberofprefixes2 = [3,]	
	numberofprefixes3 = [5, 10, 20,]
	numberofprefixes4 = [2, 10, 50,]
	
	unlimitedfibpar = copy.deepcopy(convergencepar)
	unlimitedfibpar.name = "unlimitedfib"
	unlimitedfibpar.varset = numberofprefixes4
	unlimitedfibpar.varx = "nprefix"
	unlimitedfibpar.runs = 10
	unlimitedfibexp = NumberofPrefixes (parameters = unlimitedfibpar)
	unlimitedfibexp.run ()
	
	regrateset = [1, 2, 4,]
	newprefixpar = copy.deepcopy(convergencepar)
	newprefixpar.name = "newprefix"
	newprefixpar.varset = regrateset
	newprefixpar.varx = "regrate"
	newprefixpar.runs = 31
	newprefixpar.nprefix = 100
	newprefixpar.consprefs = 3
	newprefixpar.stratset = stratset4
	newprefixpar.fibsize = 2000
	newprefixpar.failtime = 0
	#newprefixpar.constime = 10
	newprefixexp = RegisterPrefixRate (parameters = newprefixpar)
	newprefixexp.run ()	
	
	nprefixparam = copy.deepcopy(baseparameters)
	nprefixparam.name = "nprefix-randposition"
	nprefixparam.script = "revista2b"
	nprefixparam.varx = "nprefix"
	nprefixparam.varset = numberofprefixes
	nprefixparam.consprefs = 2
	nprefixparam.topo = "1755.txt"
	nprefixparam.rate = 40
	nprefixparam.runs = 31
	nprefixparam.samplei = 20
	nprefixparam.ttime = 501
	nprefixparam.fibsize = 300
	nprefixparam.failtime = 0
	nprefixparam.stratset = stratset2
	nprefixexperiment = NumberofPrefixes2 (parameters = nprefixparam)
	nprefixexperiment.run()
	
	nprefixparam1 = copy.deepcopy(nprefixparam)
	nprefixparam1.name = "nprefix-randposition1"
	nprefixparam1.rate = 80
	nprefixexperiment1 = NumberofPrefixes2 (parameters = nprefixparam1)
	nprefixexperiment1.run()
	
	nfibentries = [5, 10, 15,]
	fiblimparam = copy.deepcopy(baseparameters)
	fiblimparam.name = "fiblimit-randposition"
	fiblimparam.script = "revista2b"
	fiblimparam.varx = "fibsize"
	fiblimparam.varset = nfibentries	
	fiblimparam.nprefix = 15
	fiblimparam.consprefs = 15
	fiblimparam.topo = "1755.txt"
	fiblimparam.rate = 10	
	fiblimparam.runs = 31
	fiblimparam.samplei = 20
	fiblimparam.ttime = 501
	fiblimparam.failtime = 0
	fiblimparam.stratset = stratset2
	fibexperiment = FIBSize2 (parameters = fiblimparam)
	fibexperiment.run()

	nfibentries1 = [50, 100, 150,]	
	fiblimparam1 = copy.deepcopy(fiblimparam)
	fiblimparam1.name = "fiblimit-randposition1"
	fiblimparam1.varset = nfibentries1	
	fiblimparam1.nprefix = 150
	fiblimparam1.rate = 1
	fiblimparam1.consprefs = 150	
	fibexperiment1 = FIBSize2 (parameters = fiblimparam1)
	fibexperiment1.run()	
	
	fiblimparam2 = copy.deepcopy(fiblimparam1)
	fiblimparam2.name = "fiblimit-randposition2"
	fiblimparam2.varset = nfibentries1	
	fiblimparam2.nprefix = 150
	fiblimparam2.rate = 1
	fiblimparam2.consprefs = 150	
	fibexperiment2 = FIBSize2 (parameters = fiblimparam2)
	fibexperiment2.run()

	crateparam = copy.deepcopy(fiblimparam1)
	crateparam.name = "crate-limitedfib"
	crateparam.varx = "rate"
	crateparam.varset = [1, 2, 4, 8,]
	crateparam.fibsize = 100
	crateexperiment = ConsumerRate2(parameters = crateparam)
	crateexperiment.run()
	
	crateparam1 = copy.deepcopy(crateparam)
	crateparam1.name = "crate-unlimitedfib"
	crateparam1.fibsize = 150
	crateexperiment1 = ConsumerRate2(parameters = crateparam1)
	crateexperiment1.run()
	
	nconsumersparam = copy.deepcopy(baseparameters)
	nconsumersparam.name = "nconsumers"
	nconsumersparam.script = "revista2b"
	nconsumersparam.varx = "consxint"
	nconsumersparam.varset = [5, 10, 15,]	
	nconsumersparam.rate = 2
	nconsumersparam.nprefix = 1
	nconsumersparam.consprefs = 1
	nconsumersparam.topo = "1755.txt"
	nconsumersparam.runs = 10
	nconsumersparam.samplei = 20
	nconsumersparam.failtime = 0
	nconsumersparam.stratset = stratset2
	nconsumersexperiment = NConsumers (parameters = nconsumersparam)
	nconsumersexperiment.run()

	nproducersparam = copy.deepcopy(baseparameters)
	nproducersparam.name = "nproducers"
	nproducersparam.script = "revista2b"
	nproducersparam.varx = "prodxint"
	nproducersparam.varset = [5, 10, 15,]	
	nproducersparam.rate = 20
	nproducersparam.nprefix = 1
	nproducersparam.consprefs = 1
	nproducersparam.topo = "1755.txt"
	nproducersparam.runs = 10
	nproducersparam.samplei = 20
	nproducersparam.failtime = 0
	nproducersparam.stratset = stratset2
	nproducersexperiment = NProducers (parameters = nproducersparam)
	nproducersexperiment.run()	
	
	cacheparam = copy.deepcopy(baseparameters)
	cacheparam.name = "cache-default"
	cacheparam.script = "revista2b-cache"
	cacheparam.varx = "consxint"
	cacheparam.varset = [5, 10, 15,]	
	cacheparam.rate = 2
	cacheparam.nprefix = 1
	cacheparam.consprefs = 1
	cacheparam.topo = "1755.txt"
	cacheparam.runs = 10
	cacheparam.samplei = 20
	cacheparam.failtime = 0
	cacheparam.stratset = stratset2
	cacheexperiment = NConsumers (parameters = cacheparam)
	cacheexperiment.run()
	
	nproducerscacheparam = copy.deepcopy(baseparameters)
	nproducerscacheparam.name = "nproducerscache"
	nproducerscacheparam.script = "revista2b-cache"
	nproducerscacheparam.varx = "prodxint"
	nproducerscacheparam.varset = [5, 25, 125,]	
	nproducerscacheparam.rate = 20
	nproducerscacheparam.nprefix = 1
	nproducerscacheparam.consprefs = 1
	nproducerscacheparam.topo = "1755.txt"
	nproducerscacheparam.runs = 10
	nproducerscacheparam.samplei = 20
	nproducerscacheparam.failtime = 0
	nproducerscacheparam.constime = 200
	nproducerscacheparam.stratset = stratset5
	nproducerscacheexperiment = NProducers (parameters = nproducerscacheparam)
	nproducerscacheexperiment.run()	
	
	consrateparam = copy.deepcopy(baseparameters)
	consrateparam.name = "consumerrate"
	consrateparam.script = "revista2b-cache"
	consrateparam.varx = "rrate"
	consrateparam.varset = [5, 10, 15,]	
	consrateparam.nprefix = 1
	consrateparam.consprefs = 1
	consrateparam.consxint = 1
	consrateparam.topo = "1755.txt"
	consrateparam.runs = 10
	consrateparam.samplei = 20
	consrateparam.failtime = 0
	consrateparam.stratset = stratset2
	consrateexperiment = ConsumerRate2 (parameters = consrateparam)
	consrateexperiment.run()

	routingparam = copy.deepcopy(baseparameters)
	routingparam.name = "routing-cache"
	routingparam.varx = "script"
	routingparam.varset = ["NoCache", "PathCache", "ConsumerCache", "PathConsumerCache",]
	#routingparam.varset = ["PathCache", "PathConsumerCache",]
	#routingparam.varset = ["NoCache", "PathCache", "PathConsumerCache",]
	routingparam.consxint = 25
	routingparam.rate = 20
	routingparam.nprefix = 1
	routingparam.consprefs = 1
	routingparam.topo = "1755.txt"
	routingparam.runs = 31
	routingparam.samplei = 20
	routingparam.failtime = 0	
	routingparam.ttime = 501
	routingparam.stratset = ["CRoS-NDN",]
	routingexperiment = NScripts (parameters = routingparam)
	routingexperiment.run()
	
	cachesizeparam = copy.deepcopy(routingparam)
	cachesizeparam.name = "cache-size"
	cachesizeparam.varx = "consperc"
	cachesizeparam.varset = [10, 1000, 100000]
	cachesizeparam.stratset = ["ConsumerERouterCache", "RouterCache",]
	cachesizeparam.runs = 50
	cachesizeparam.ttime = 601
	cachesizeexperiment = CacheSize(parameters = cachesizeparam)
	cachesizeexperiment.run()
	
	cachesizeparam2 = copy.deepcopy(cachesizeparam)
	cachesizeparam2.name = "cache-size2"
	cachesizeparam2.stratset = ["ConsumerERouterCache2", "RouterCache2",]
	cachesizeparam2.ttime = 501
	cachesizeexperiment2 = CacheSize(parameters = cachesizeparam2)
	cachesizeexperiment2.run()
	
	cachesizeparam3 = copy.deepcopy(cachesizeparam)
	cachesizeparam3.name = "cache-size3"
	cachesizeparam3.stratset = ["ConsumerERouterCache3", "RouterCache3",]
	cachesizeparam3.ttime = 501
	cachesizeparam3.rate = 500
	cachesizeexperiment3 = CacheSize(parameters = cachesizeparam3)
	cachesizeexperiment3.run()
	
	cachesizeparam4 = copy.deepcopy(cachesizeparam)
	cachesizeparam4.name = "cache-size100000-consumer-rate"
	cachesizeparam4.varx = "rate"
	cachesizeparam4.varset = [2, 20, 200]
	cachesizeparam4.consperc = 100000 #cachesize	
	cachesizeparam4.stratset = ["ConsumerERouterCache3", "RouterCache3",]
	cachesizeparam4.ttime = 501
	cachesizeexperiment4 = ConsumerRate3(parameters = cachesizeparam4)
	cachesizeexperiment4.run()
	
	cachesizeparam5 = copy.deepcopy(cachesizeparam)
	cachesizeparam5.name = "cache-size100000-topo"
	cachesizeparam5.varx = "topo"
	cachesizeparam5.varset = ["1221.txt", "1755.txt", "3257.txt", "2914.txt", "3356.txt", "7018.txt",]
	cachesizeparam5.consperc = 100000 #cachesize	
	cachesizeparam5.stratset = ["ConsumerERouterCache3", "RouterCache3",]
	cachesizeparam5.rate = 100
	cachesizeparam5.ttime = 501
	cachesizeexperiment5 = Topo1(parameters = cachesizeparam5)
	cachesizeexperiment5.run()
	
	#*****TUNNEL start
	
	cachesizeparam6 = copy.deepcopy(cachesizeparam)
	cachesizeparam6.name = "cache-size3-rate20-tunnel"
	cachesizeparam6.stratset = ["ConsumerERouterCache3-tunnel", "RouterCache3-tunnel",]
	cachesizeparam6.ttime = 501
	cachesizeparam6.rate = 20
	cachesizeexperiment6 = CacheSize(parameters = cachesizeparam6)
	cachesizeexperiment6.run()
	
	cachesizeparam6b = copy.deepcopy(cachesizeparam6)
	cachesizeparam6b.name = "cache-size3-rate100-tunnel"
	cachesizeparam6b.rate = 100
	cachesizeexperiment6b = CacheSize(parameters = cachesizeparam6b)
	cachesizeexperiment6b.run()
	
	cachesizeparam7 = copy.deepcopy(cachesizeparam)
	cachesizeparam7.name = "cache-size10-consumer-rate-tunnel"
	cachesizeparam7.varx = "rate"
	cachesizeparam7.varset = [2, 20, 200]
	cachesizeparam7.consperc = 10 #cachesize	
	cachesizeparam7.stratset = ["ConsumerERouterCache3-tunnel", "RouterCache3-tunnel",]
	cachesizeparam7.ttime = 501
	cachesizeexperiment7 = ConsumerRate3(parameters = cachesizeparam7)
	cachesizeexperiment7.run()
	
	cachesizeparam7b = copy.deepcopy(cachesizeparam7)
	cachesizeparam7b.name = "cache-size100000-consumer-rate-tunnel"
	cachesizeparam7b.consperc = 100000 #cachesize	
	cachesizeexperiment7b = ConsumerRate3(parameters = cachesizeparam7b)
	cachesizeexperiment7b.run()	
	
	cachesizeparam8 = copy.deepcopy(cachesizeparam)
	cachesizeparam8.name = "cache-size10-topo-tunnel"
	cachesizeparam8.varx = "topo"
	cachesizeparam8.varset = ["1221.txt", "1755.txt", "3257.txt", "2914.txt", "3356.txt", "7018.txt",]
	cachesizeparam8.consperc = 10 #cachesize	
	cachesizeparam8.stratset = ["ConsumerERouterCache3-tunnel", "RouterCache3-tunnel",]
	cachesizeparam8.rate = 100
	cachesizeparam8.ttime = 501
	cachesizeexperiment8 = Topo1(parameters = cachesizeparam8)
	cachesizeexperiment8.run()	
	
	cachesizeparam8b = copy.deepcopy(cachesizeparam8)
	cachesizeparam8b.name = "cache-size100000-topo-tunnel"
	cachesizeparam8b.consperc = 100000 #cachesize	
	cachesizeexperiment8b = Topo1(parameters = cachesizeparam8b)
	cachesizeexperiment8b.run()		

	#*****TUNNEL end
	
	mobilityparam  = copy.deepcopy(nproducerscacheparam)
	mobilityparam.name = "mobility"
	mobilityparam.script = "revista2b-mobility"
	mobilityparam.varx = "prodxint"
	mobilityparam.varset = [5, 25, 125,]	
	mobilityparam.rate = 20
	mobilityparam.consprefs = 1
	mobilityparam.ttime = 501
	mobilityparam.constime = 0
	mobilityparam.runs = 30
	mobilityparam.stratset = stratset2
	mobilityparam.consxint = 3
	mobilityparam.fibsize = 15
	mobilityparam.nprefix = 1
	mobilityexperiment = NProducers(parameters = mobilityparam)
	mobilityexperiment.run()
	
	mobilityparam1  = copy.deepcopy(mobilityparam)
	mobilityparam1.name = "mobility1"
	mobilityparam1.consprefs = 10
	mobilityparam1.nprefix = 10
	mobilityexperiment1 = NProducers(parameters = mobilityparam1)
	mobilityexperiment1.run()
	
	mobilityparam2 = copy.deepcopy(mobilityparam)
	mobilityparam2.name = "mobility-rev1"
	mobilityparam2.topo = "1755.txt"
	mobilityparam2.stratset = ["CRoS-NDN",]
	mobilityexperiment2 = NProducers(parameters = mobilityparam2)
	mobilityexperiment2.run()
	
	mobilityparam3 = copy.deepcopy(mobilityparam)
	mobilityparam3.name = "mobility-rev1-1"
	mobilityparam3.topo = "1221.txt"
	mobilityparam3.stratset = ["CRoS-NDN",]
	mobilityexperiment3 = NProducers(parameters = mobilityparam3)
	mobilityexperiment3.run()

	mobilityparam4 = copy.deepcopy(mobilityparam)
	mobilityparam4.name = "mobility-rev1-2"
	mobilityparam4.topo = "3257.txt"
	mobilityparam4.stratset = ["CRoS-NDN",]
	mobilityexperiment4 = NProducers(parameters = mobilityparam4)
	mobilityexperiment4.run()	
	
	mobilityparam5 = copy.deepcopy(mobilityparam)
	mobilityparam5.name = "mobility-rev1-30consumers"
	mobilityparam5.topo = "1755.txt"
	mobilityparam5.rate = 2
	mobilityparam5.consxint = 30
	mobilityparam5.stratset = ["CRoS-NDN",]
	mobilityexperiment5 = NProducers(parameters = mobilityparam5)
	mobilityexperiment5.run()
	
	mobilityparam6 = copy.deepcopy(mobilityparam)
	mobilityparam6.name = "mobility-rev1-1-30consumers"
	mobilityparam6.topo = "1221.txt"
	mobilityparam6.rate = 2
	mobilityparam6.consxint = 30
	mobilityparam6.stratset = ["CRoS-NDN",]
	mobilityexperiment6 = NProducers(parameters = mobilityparam6)
	mobilityexperiment6.run()	
	
	mandelbrotparam = copy.deepcopy(mobilityparam)
	mandelbrotparam.name = "mandelbrot-nconsumer-zipf07-fib100"
	#mandelbrotparam.script = "mandelbrot-r2"
	#mandelbrotparam.script = "mandelbrot-r2-ctl-fix"
	mandelbrotparam.script = "mandelbrot-r2-zipf07-regcontentset"
	mandelbrotparam.varx = "consxint"
	#mandelbrotparam.varset = [1, 5, 15, 20, 25, 30]
	#mandelbrotparam.varset = [1, 15, 25,]
	mandelbrotparam.varset = [1, 2, 4, 8,]
	#mandelbrotparam.varset = [5,]
	mandelbrotparam.rate = 50
	mandelbrotparam.nprefix = 3000
	mandelbrotparam.consprefs = 3000
	mandelbrotparam.fibsize = 100
	mandelbrotparam.prodxint = 1
	mandelbrotparam.regrate = 200
	mandelbrotparam.stratset = ["CRoS-NDN",]
	mandelbrotparam.runs = 30
	mandelbrotparam.ttime = 501
	mandelbrotparam.constime = 100
	mandelbrotparam.topo = "1755.txt"
	mandelbrotexperimet = NConsumers(mandelbrotparam)
	mandelbrotexperimet.run()
	
	mandelbrotparam1 = copy.deepcopy(mandelbrotparam)
	mandelbrotparam1.name = "mandelbrot-nconsumer-zipf07-fib1k"
	mandelbrotparam1.fibsize = 1000
	mandelbrotexperimet1 = NConsumers(mandelbrotparam1)
	mandelbrotexperimet1.run()
	
	mandelbrotparam2 = copy.deepcopy(mandelbrotparam)
	mandelbrotparam2.name = "mandelbrot-nconsumer-zipf07-fib3k"
	mandelbrotparam2.fibsize = 3000
	mandelbrotexperimet2 = NConsumers(mandelbrotparam2)
	mandelbrotexperimet2.run()
	
	mandelbrotparam3 = copy.deepcopy(mandelbrotparam)
	mandelbrotparam3.name = "mandelbrot-nconsumer-zipf14-fib100"
	mandelbrotparam3.script = "mandelbrot-r2-zipf14-regcontentset"
	mandelbrotparam3.fibsize = 100
	mandelbrotexperimet3 = NConsumers(mandelbrotparam3)
	mandelbrotexperimet3.run()
	
	#*****TUNNEL start
	mandelbrotparam4 = copy.deepcopy(mandelbrotparam)
	mandelbrotparam4.name = "mandelbrot-nconsumer-zipf07-fib100-tunnel"
	mandelbrotparam4.script = "mandelbrot-r2-zipf07-regcontentset-tunnel"
	mandelbrotparam4.fibsize = 100
	mandelbrotexperimet4 = NConsumers(mandelbrotparam4)
	mandelbrotexperimet4.run()
	
	mandelbrotparam5 = copy.deepcopy(mandelbrotparam)
	mandelbrotparam5.name = "mandelbrot-nconsumer-zipf07-fib1k-tunnel"
	mandelbrotparam5.script = "mandelbrot-r2-zipf07-regcontentset-tunnel"
	mandelbrotparam5.fibsize = 1000
	mandelbrotexperimet5 = NConsumers(mandelbrotparam5)
	mandelbrotexperimet5.run()
	
	mandelbrotparam6 = copy.deepcopy(mandelbrotparam)
	mandelbrotparam6.name = "mandelbrot-nconsumer-zipf07-fib3k-tunnel"
	mandelbrotparam6.script = "mandelbrot-r2-zipf07-regcontentset-tunnel"
	mandelbrotparam6.fibsize = 3000
	mandelbrotexperimet6 = NConsumers(mandelbrotparam6)
	mandelbrotexperimet6.run()
	
	mandelbrotparam7 = copy.deepcopy(mandelbrotparam)
	mandelbrotparam7.name = "mandelbrot-nconsumer-zipf14-fib100-tunnel"
	mandelbrotparam7.script = "mandelbrot-r2-zipf14-regcontentset-tunnel"
	mandelbrotparam7.fibsize = 100
	mandelbrotexperimet7 = NConsumers(mandelbrotparam7)
	mandelbrotexperimet7.run()		
		
	#*****TUNNEL end
	
	mandelbrotcrparam = copy.deepcopy(mandelbrotparam)
	mandelbrotcrparam.name = "mandelbrot-crate-zipf07-fib100"
	mandelbrotcrparam.script = "mandelbrot-r2-zipf07-regcontentset"
	mandelbrotcrparam.consxint = 1
	mandelbrotcrparam.fibsize = 100
	mandelbrotcrparam.varx = "rate"
	#mandelbrotcrparam.varset = [20, 40, 60, 80, 100,]
	#mandelbrotcrparam.varset = [60, 120, 180,]
	mandelbrotcrparam.varset = [50, 100, 200, 400,]
	mandelbrotcrexperiment = ConsumerRate2(mandelbrotcrparam)
	mandelbrotcrexperiment.run()
	
	mandelbrotcrparam1 = copy.deepcopy(mandelbrotcrparam)
	mandelbrotcrparam1.name = "mandelbrot-crate-zipf07-fib1k"
	mandelbrotcrparam1.script = "mandelbrot-r2-zipf07-regcontentset"
	mandelbrotcrparam1.fibsize = 1000
	mandelbrotcrexperiment1 = ConsumerRate2(mandelbrotcrparam1)
	mandelbrotcrexperiment1.run()
	
	mandelbrotcrparam2 = copy.deepcopy(mandelbrotcrparam)
	mandelbrotcrparam2.name = "mandelbrot-crate-zipf07-fib3k"
	mandelbrotcrparam2.script = "mandelbrot-r2-zipf07-regcontentset"
	mandelbrotcrparam2.fibsize = 3000
	mandelbrotcrexperiment2 = ConsumerRate2(mandelbrotcrparam2)
	mandelbrotcrexperiment2.run()
	
	mandelbrotcrparam3 = copy.deepcopy(mandelbrotcrparam)
	mandelbrotcrparam3.name = "mandelbrot-crate-zipf14-fib100"
	mandelbrotcrparam3.script = "mandelbrot-r2-zipf14-regcontentset"
	mandelbrotcrexperiment3 = ConsumerRate2(mandelbrotcrparam3)
	mandelbrotcrexperiment3.run()
	
	fibparam = copy.deepcopy(baseparameters)
	fibparam.name = "fibsize-rev1"
	fibparam.stratset = ["CRoS-NDN",]
	fibparam.varx = "fibsize"
	fibparam.varset = [10, 50, 100,]
	fibparam.script = "revista1c"
	fibparam.failtime = 0
	fibparam.ttime = 1001
	fibparam.runs = 31
	fibparam.nprefix = 200
	fibparam.consprefs = 200
	fibparam.samplei = 5
	fibparam.rate = 5
	fibparam.topo = "1221.txt"
	fibparamexperiment = FIBSize2 (parameters = fibparam)
	fibparamexperiment.run()
	
	fibparam1 = copy.deepcopy(fibparam)
	fibparam1.name = "fibsize-rev1-1"
	fibparam1.topo = "1755.txt"
	fibparamexperiment1 = FIBSize2 (parameters = fibparam1)
	fibparamexperiment1.run()	
	
	fibparam2 = copy.deepcopy(fibparam)
	fibparam2.name = "fibsize-rev1-2"
	fibparam2.topo = "3257.txt"
	fibparamexperiment2 = FIBSize2 (parameters = fibparam2)
	fibparamexperiment2.run()

	fibparam3 = copy.deepcopy(fibparam)
	fibparam3.name = "fibsize-rev1-3"
	fibparam3.topo="topo-3-way.txt"
	fibparamexperiment3 = FIBSize2 (parameters = fibparam3)
	fibparamexperiment3.run()	

	mandelbrotfrateparam = copy.deepcopy(mobilityparam)
	mandelbrotfrateparam.name = "mandelbrot-nconsumer-fixedrate-zipf07-fib100"
	mandelbrotfrateparam.script = "mandelbrot-r2-zipf07-regcontentset"
	mandelbrotfrateparam.varx = "consxint"
	mandelbrotfrateparam.varset = [1, 2, 4, 8,]
	mandelbrotfrateparam.rate = 100
	mandelbrotfrateparam.nprefix = 3000
	mandelbrotfrateparam.consprefs = 3000
	mandelbrotfrateparam.fibsize = 100
	mandelbrotfrateparam.prodxint = 1
	mandelbrotfrateparam.regrate = 200
	mandelbrotfrateparam.stratset = ["CRoS-NDN",]
	mandelbrotfrateparam.runs = 30
	#mandelbrotfrateparam.ttime = 501 alterado em09maio2016
	mandelbrotfrateparam.ttime = 5001
	mandelbrotfrateparam.constime = 100
	mandelbrotfrateparam.topo = "1755.txt"
	mandelbrotfrateexperiment = NConsumersFixedRate(mandelbrotfrateparam)
	mandelbrotfrateexperiment.run()
	
	mandelbrotfrateparam1 = copy.deepcopy(mandelbrotfrateparam)
	mandelbrotfrateparam1.name = "mandelbrot-nconsumer-fixedrate-zipf07-fib1k"
	mandelbrotfrateparam1.fibsize = 1000
	mandelbrotfrateexperiment1 = NConsumersFixedRate(mandelbrotfrateparam1)
	mandelbrotfrateexperiment1.run()
	
	mandelbrotfrateparam2 = copy.deepcopy(mandelbrotfrateparam)
	mandelbrotfrateparam2.name = "mandelbrot-nconsumer-fixedrate-zipf07-fib3k"
	mandelbrotfrateparam2.fibsize = 3000
	mandelbrotfrateexperiment2 = NConsumersFixedRate(mandelbrotfrateparam2)
	mandelbrotfrateexperiment2.run()
	
	mandelbrotfrateparam3 = copy.deepcopy(mandelbrotfrateparam)
	mandelbrotfrateparam3.name = "mandelbrot-nconsumer-fixedrate-zipf14-fib100"
	mandelbrotfrateparam3.script = "mandelbrot-r2-zipf14-regcontentset"
	mandelbrotfrateparam3.fibsize = 100
	mandelbrotfrateexperiment3 = NConsumersFixedRate(mandelbrotfrateparam3)
	mandelbrotfrateexperiment3.run()

	#*****TUNNEL start
	
	mandelbrotfrateparam4 = copy.deepcopy(mandelbrotfrateparam)
	mandelbrotfrateparam4.name = "mandelbrot-nconsumer-fixedrate-zipf07-fib100-tunnel"
	mandelbrotfrateparam4.script = "mandelbrot-r2-zipf07-regcontentset-tunnel"
	mandelbrotfrateparam4.fibsize = 100
	mandelbrotfrateexperiment4 = NConsumersFixedRate(mandelbrotfrateparam4)
	mandelbrotfrateexperiment4.run()
	
	mandelbrotfrateparam5 = copy.deepcopy(mandelbrotfrateparam)
	mandelbrotfrateparam5.name = "mandelbrot-nconsumer-fixedrate-zipf07-fib1k-tunnel"
	mandelbrotfrateparam5.script = "mandelbrot-r2-zipf07-regcontentset-tunnel"
	mandelbrotfrateparam5.fibsize = 1000
	mandelbrotfrateexperiment5 = NConsumersFixedRate(mandelbrotfrateparam5)
	mandelbrotfrateexperiment5.run()
	
	mandelbrotfrateparam6 = copy.deepcopy(mandelbrotfrateparam)
	mandelbrotfrateparam6.name = "mandelbrot-nconsumer-fixedrate-zipf07-fib3k-tunnel"
	mandelbrotfrateparam6.script = "mandelbrot-r2-zipf07-regcontentset-tunnel"
	mandelbrotfrateparam6.fibsize = 3000
	mandelbrotfrateexperiment6 = NConsumersFixedRate(mandelbrotfrateparam6)
	mandelbrotfrateexperiment6.run()
	
	mandelbrotfrateparam7 = copy.deepcopy(mandelbrotfrateparam)
	mandelbrotfrateparam7.name = "mandelbrot-nconsumer-fixedrate-zipf14-fib100-tunnel"
	mandelbrotfrateparam7.script = "mandelbrot-r2-zipf14-regcontentset-tunnel"
	mandelbrotfrateparam7.fibsize = 100
	mandelbrotfrateexperiment7 = NConsumersFixedRate(mandelbrotfrateparam7)
	mandelbrotfrateexperiment7.run()
	
	#*****TUNNEL end
	
	mandelbrotfibprefparam = copy.deepcopy(mandelbrotfrateparam)
	mandelbrotfibprefparam.name = "mandelbrot-fib-pref-fixedratio"
	mandelbrotfibprefparam.script = "mandelbrot-r2-zipf07-regcontentset"
	#mandelbrotfibprefparam.script = "mandelbrot-r2-zipf14-regcontentset"
	mandelbrotfibprefparam.consxint = 1
	mandelbrotfibprefparam.varx = "fibsize"
	mandelbrotfibprefparam.rate = 100
	mandelbrotfibprefparam.varset = [100, 1000, 10000, 100000]
	mandelbrotfibprefparam.ttime = 501
	mandelbrotfibprefparam.constime = 100
	mandelbrotfibprefparam.prodxint = 1
	mandelbrotfibprefparam.regrate = 200
	mandelbrotfibprefparam.stratset = ["CRoS-NDN",]
	mandelbrotfibprefparam.runs = 30
	mandelbrotfibprefparam.topo = "1755.txt"
	mandelbrotfibprefparam.samplei = 20
	mandelbrotfibprefexperiment = FIBSize3 (parameters = mandelbrotfibprefparam)
	mandelbrotfibprefexperiment.run()
	
	mandelbrotfibsizeparam = copy.deepcopy(mandelbrotfibprefparam)
	mandelbrotfibsizeparam.name = "mandelbrot-fibsize"
	mandelbrotfibsizeparam.nprefix = 100000
	mandelbrotfibsizeexperiment = FIBSize2 (parameters = mandelbrotfibsizeparam)
	mandelbrotfibsizeexperiment.run()
	
	mandelbrotfibsizenprefixfactorparam = copy.deepcopy(mandelbrotfibsizeparam)
	mandelbrotfibsizenprefixfactorparam.name = "mandelbrot-fibsize-nprefix-zipf07-factor1"
	mandelbrotfibsizenprefixfactorexperiment = FIBSize4 (parameters = mandelbrotfibsizenprefixfactorparam, factor = 1)
	mandelbrotfibsizenprefixfactorexperiment.run()
	
	mandelbrotfibsizenprefixfactorparam1 = copy.deepcopy(mandelbrotfibsizeparam)
	mandelbrotfibsizenprefixfactorparam1.name = "mandelbrot-fibsize-nprefix-zipf07-factor10"
	mandelbrotfibsizenprefixfactorexperiment1 = FIBSize4 (parameters = mandelbrotfibsizenprefixfactorparam1, factor = 10)
	mandelbrotfibsizenprefixfactorexperiment1.run()
	
	mandelbrotfibsizenprefixfactorparam2 = copy.deepcopy(mandelbrotfibsizeparam)
	mandelbrotfibsizenprefixfactorparam2.name = "mandelbrot-fibsize-nprefix-zipf07-factor100"
	mandelbrotfibsizenprefixfactorexperiment2 = FIBSize4 (parameters = mandelbrotfibsizenprefixfactorparam2, factor = 100)
	mandelbrotfibsizenprefixfactorexperiment2.run()
	
	#This requires too much memory!!!
	# mandelbrotfibsizenprefixfactorparam3 = copy.deepcopy(mandelbrotfibsizeparam)
	# mandelbrotfibsizenprefixfactorparam3.name = "mandelbrot-fibsize-nprefix-factor1000"
	# mandelbrotfibsizenprefixfactorexperiment3 = FIBSize4 (parameters = mandelbrotfibsizenprefixfactorparam3, factor = 1000)
	# mandelbrotfibsizenprefixfactorexperiment3.run()	
	
	# mandelbrotfibsizenprefixfactorparam4 = copy.deepcopy(mandelbrotfibsizeparam)
	# mandelbrotfibsizenprefixfactorparam4.name = "mandelbrot-fibsize-nprefix-factor10000"
	# mandelbrotfibsizenprefixfactorexperiment4 = FIBSize4 (parameters = mandelbrotfibsizenprefixfactorparam4, factor = 10000)
	# mandelbrotfibsizenprefixfactorexperiment4.run()
	
	mandelbrotfibsizenprefixfactorparam5 = copy.deepcopy(mandelbrotfibsizeparam)
	mandelbrotfibsizenprefixfactorparam5.name = "mandelbrot-fibsize-nprefix-zipf14-factor1"
	mandelbrotfibsizenprefixfactorparam5.script = "mandelbrot-r2-zipf14-regcontentset"
	mandelbrotfibsizenprefixfactorexperiment5 = FIBSize4 (parameters = mandelbrotfibsizenprefixfactorparam5, factor = 1)
	mandelbrotfibsizenprefixfactorexperiment5.run()
	
	mandelbrotfibsizenprefixfactorparam6 = copy.deepcopy(mandelbrotfibsizenprefixfactorparam5)
	mandelbrotfibsizenprefixfactorparam6.name = "mandelbrot-fibsize-nprefix-zipf14-factor10"
	mandelbrotfibsizenprefixfactorexperiment6 = FIBSize4 (parameters = mandelbrotfibsizenprefixfactorparam6, factor = 10)
	mandelbrotfibsizenprefixfactorexperiment6.run()
	
	mandelbrotfibsizenprefixfactorparam7 = copy.deepcopy(mandelbrotfibsizenprefixfactorparam5)
	mandelbrotfibsizenprefixfactorparam7.name = "mandelbrot-fibsize-nprefix-zipf14-factor100"
	mandelbrotfibsizenprefixfactorexperiment7 = FIBSize4 (parameters = mandelbrotfibsizenprefixfactorparam7, factor = 100)
	mandelbrotfibsizenprefixfactorexperiment7.run()	
	
	#*****TUNNEL start
	
	mandelbrotfibsizenprefixfactorparam8 = copy.deepcopy(mandelbrotfibsizeparam)
	mandelbrotfibsizenprefixfactorparam8.name = "mandelbrot-fibsize-nprefix-zipf14-factor1-tunnel"
	mandelbrotfibsizenprefixfactorparam8.script = "mandelbrot-r2-zipf14-regcontentset-tunnel"
	mandelbrotfibsizenprefixfactorexperiment8 = FIBSize4 (parameters = mandelbrotfibsizenprefixfactorparam8, factor = 1)
	mandelbrotfibsizenprefixfactorexperiment8.run()
	
	mandelbrotfibsizenprefixfactorparam9 = copy.deepcopy(mandelbrotfibsizenprefixfactorparam8)
	mandelbrotfibsizenprefixfactorparam9.name = "mandelbrot-fibsize-nprefix-zipf14-factor10-tunnel"
	mandelbrotfibsizenprefixfactorexperiment9 = FIBSize4 (parameters = mandelbrotfibsizenprefixfactorparam9, factor = 10)
	mandelbrotfibsizenprefixfactorexperiment9.run()
	
	mandelbrotfibsizenprefixfactorparam10 = copy.deepcopy(mandelbrotfibsizenprefixfactorparam8)
	mandelbrotfibsizenprefixfactorparam10.name = "mandelbrot-fibsize-nprefix-zipf14-factor100-tunnel"
	mandelbrotfibsizenprefixfactorexperiment10 = FIBSize4 (parameters = mandelbrotfibsizenprefixfactorparam10, factor = 100)
	mandelbrotfibsizenprefixfactorexperiment10.run()	
	
	#*****TUNNEL end 
	
except:
	print "Error"
	print '-'*60
	traceback.print_exc(file=sys.stdout)
	print '-'*60	
	
	
finally:
    pool.join ()
    pool.shutdown ()


