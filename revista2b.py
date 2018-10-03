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
		os.system("cp ./temp/*.png ./results/" + fdname)		
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
			
	def writecmdline (self):
		self.seed = random.randint(1,10000)
		os.mkdir("./temp/%s" % self.suffix)
		f = open("./graphs/varx.r","w")
		f.write("varx1='%s'" % self.counter)
		f.write("\nsuffix='%s'" % self.suffix)
		f.close()
		for strat in self.stratset:
			cmdline = ["LD_LIBRARY_PATH=/usr/local/lib", "./ptimized/%s" % self.script,"--strategy=%s" % strat, "--nprefix=%s" % 
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
			cmdline = ["LD_LIBRARY_PATH=/usr/local/lib", "./ptimized/%s" % strat,"--strategy=%s" % strat, "--nprefix=%s" % 
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
		f = open("temp/summaryXaxis.txt","w")
		f.write("%s Strategy ise\n" % self.varx)
		f.close()
		f = open("temp/summarytime.txt","w")
		f.write("%s Strategy TimeSec ise\n" % self.varx)
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
				subprocess.call ("./graphs/strategy1.r", shell=True)
				run = run + 1
			subprocess.call ("./graphs/strategy-95v1.r", shell=True)
			os.rename("temp/summary.txt","temp/summary-%s.txt" % self.suffix)
			os.rename("temp/Error.png","temp/Error-%s.png" % self.suffix)
        	subprocess.call ("./graphs/strategy-xaxisv1.r", shell=True)
		subprocess.call ("./graphs/strategy-time-r2.r", shell=True)
		
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
	nprefixparam.runs = 40
	nprefixparam.samplei = 20
	nprefixparam.ttime = 501
	nprefixparam.fibsize = 300
	nprefixparam.failtime = 0
	nprefixparam.stratset = stratset2
	nprefixexperiment = NumberofPrefixes2 (parameters = nprefixparam)
	nprefixexperiment.run()
	
	nfibentries = [5, 10, 20,]
	fiblimparam = copy.deepcopy(baseparameters)
	fiblimparam.name = "fiblimit-randposition"
	fiblimparam.script = "revista2b"
	fiblimparam.varx = "fibsize"
	fiblimparam.varset = nfibentries	
	fiblimparam.nprefix = 15
	fiblimparam.consprefs = 15
	fiblimparam.topo = "1755.txt"
	fiblimparam.rate = 10	
	fiblimparam.runs = 40
	fiblimparam.samplei = 20
	fiblimparam.ttime = 501
	fiblimparam.failtime = 0
	fiblimparam.stratset = stratset2
	fibexperiment = FIBSize2 (parameters = fiblimparam)
	fibexperiment.run()
	
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
	cachesizeparam4.name = "cache-consumer-rate"
	cachesizeparam4.varx = "rate"
	cachesizeparam4.varset = [20, 100, 500]
	cachesizeparam4.consperc = 10 #cachesize	
	cachesizeparam4.stratset = ["ConsumerERouterCache3", "RouterCache3",]
	cachesizeparam4.ttime = 501
	cachesizeexperiment4 = ConsumerRate3(parameters = cachesizeparam4)
	cachesizeexperiment4.run()
	
	cachesizeparam5 = copy.deepcopy(cachesizeparam)
	cachesizeparam5.name = "cache-topo"
	cachesizeparam5.varx = "topo"
	cachesizeparam5.varset = ["1221.txt", "1755.txt", "3257.txt", "2914.txt", "3356.txt", "7018.txt",]
	cachesizeparam5.consperc = 100000 #cachesize	
	cachesizeparam5.stratset = ["ConsumerERouterCache3", "RouterCache3",]
	cachesizeparam5.rate = 100
	cachesizeparam5.ttime = 501
	cachesizeexperiment5 = Topo1(parameters = cachesizeparam5)
	cachesizeexperiment5.run()
	
	mobilityparam  = copy.deepcopy(nproducerscacheparam)
	mobilityparam.name = "mobility"
	mobilityparam.script = "revista2b-mobility"
	mobilityparam.varx = "prodxint"
	mobilityparam.varset = [5, 25, 125,]	
	mobilityparam.rate = 20
	mobilityparam.consprefs = 10
	mobilityparam.ttime = 501
	mobilityparam.constime = 0
	mobilityparam.runs = 30
	mobilityparam.stratset = stratset2
	mobilityparam.consxint = 3
	mobilityparam.fibsize = 15
	mobilityparam.nprefix = 10
	mobilityexperiment = NProducers(parameters = mobilityparam)
	mobilityexperiment.run()
	
	mandelbrotparam = copy.deepcopy(mobilityparam)
	mandelbrotparam.name = "mandelbrot-consumer"
	#mandelbrotparam.script = "mandelbrot-r2"
	#mandelbrotparam.script = "mandelbrot-r2-ctl-fix"
	mandelbrotparam.script = "mandelbrot-r2-b"
	mandelbrotparam.varx = "consxint"
	#mandelbrotparam.varset = [1, 5, 15, 20, 25, 30]
	#mandelbrotparam.varset = [1, 15, 25,]
	mandelbrotparam.varset = [1, 2, 3, 4, 5,]
	mandelbrotparam.rate = 20
	mandelbrotparam.nprefix = 3000
	mandelbrotparam.consprefs = 3000
	mandelbrotparam.fibsize = 1000
	mandelbrotparam.prodxint = 1
	mandelbrotparam.regrate = 1000
	mandelbrotparam.stratset = ["CRoS-NDN",]
	mandelbrotparam.runs = 30
	mandelbrotparam.ttime = 1001
	mandelbrotparam.topo = "1755.txt"
	mandelbrotexperimet = NConsumers(mandelbrotparam)
	mandelbrotexperimet.run()
	
	mandelbrotcrparam = copy.deepcopy(mandelbrotparam)
	mandelbrotcrparam.name = "mandelbrot-crate"
	mandelbrotcrparam.consxint = 1
	mandelbrotcrparam.varx = "rate"
	mandelbrotcrparam.varset = [20, 40, 60, 80, 100,]
	mandelbrotcrexperiment = ConsumerRate2(mandelbrotcrparam)
	mandelbrotcrexperiment.run()
	
	
except:
	print "Error"
	
	
finally:
    pool.join ()
    pool.shutdown ()


