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
                    help='Move gross results to local folder and upload processed results to svn')                    

args = parser.parse_args()

if not args.list and not args.rmresults and not args.upload and len(args.scenarios)==0:
    print "ERROR: at least one scenario need to be specified"
    parser.print_help()
    exit (1)

if args.list:
    print "Available scenarios: "
else:
    if args.simulate:
        print "Simulating the following scenarios: " + ",".join (args.scenarios)

    if args.graph:
        print "Building graphs for the following scenarios: " + ",".join (args.scenarios)
        
    if args.rmresults:
        print "Cleaning all files in -temp- folder!"
        print "Erasing directory: " + os.getcwd() + "/temp/*"
        os.system("rm -r ./temp/*")
        exit(0)        

    if args.upload:
        print "Move gross results to local folder and upload processed results to svn"
        now = datetime.now()
        nowstr = now.strftime("%Y%m%d-%H%M%S")
        os.mkdir("./localresults/%s" % nowstr)
        os.mkdir("./results/%s" % nowstr)
        os.system("mv ./temp/var* ./localresults/%s/" % nowstr)
        os.system("mv ./temp/* ./results/%s/" % nowstr)        
        #os.system("svn add results/*")
        #os.system("svn commit -m 'Novos resultados:  %s'" % nowstr)
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
        #subprocess.call (self.cmdline, shell=False)
        subprocess.check_call(" ".join (self.cmdline), shell=True)
        subprocess.call (" ".join (self.cmdline), shell=True)

pool = workerpool.WorkerPool(size = multiprocessing.cpu_count())

class Processor:
    def run (self):
        if args.list:
            print "    " + self.name
            return
           
        if "all" not in args.scenarios and self.name not in args.scenarios:
            return

        if args.list:
            pass
        else:
            if args.simulate:
                self.simulate ()
                pool.join ()
                #self.postprocess ()
            if args.graph:
                self.graph ()

    def graph (self):
        os.system("rm ./temp/*.png")
        os.system("rm ./temp/*.txt")        
        self.postprocess ()
        #print ("Doing nothing!")
        #subprocess.call ("./graphs/strategy.r", shell=True)
        #subprocess.call ("./graphs/%s.R" % self.name, shell=True)

class Base (Processor):
    def __init__ (self, name, script, stratset, nprefix, topo, controller, rate, hellorate, consxint, cons, prodxint, prod, consprefs, consperc, ttime, failtime, constime, regrate, fibsize, samplei, lang,  varx, maxXaxis, suffix):
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
        self.ctime = constime
        self.regrate = regrate
        self.fibsize = fibsize
        self.samplei = samplei
        self.lang = lang
        self.varx = varx
        self.maxXaxis = maxXaxis
        self.suffix = suffix
        f = open("graphs/variables.r","w")
        f.write("constime=%s + 1" % self.ctime)
        f.write("\nlang='%s'" % self.lang)        
        f.write("\nstratset=c(")
        for strat in self.stratset:
                f.write("'%s'," % strat)       
        f.write("'null')")
        f.write("\nvarx='%s'" % self.varx)
        f.write("\nvarx2=quote(%s)" % self.varx)        
        f.write("\nttime=%s" % self.ttime)
        f.write("\nmaxXaxis=%s" % self.maxXaxis)
        f.close()    
        # other initialization, if any

    def simulate (self):
        self.seed = random.randint(1,10000)
        #self.suffix = "var-%s-single-run" % (self.varx)
        os.mkdir("./temp/%s" % self.suffix)
        f = open("./graphs/varx.r","w")
        f.write("varx1='1'")
        f.write("\nsuffix='%s'" % self.suffix)
        f.close()
        for strat in self.stratset:           
                cmdline = ["LD_LIBRARY_PATH=/usr/local/lib",
                        "./ptimized/%s" % self.script,
                        "--strategy=%s" % strat,
                        "--nprefix=%s" % self.nprefix,
                        "--topo=%s" % self.topo,
                        "--consumer=%s" % self.cons,
                        "--producer=%s" % self.prod,
                        "--controller=%s" % self.controller,
                        "--rate=%s" % self.rate,
                        "--hellorate=%s" % self.hellorate,
                        "--consxint=%s" % self.consxint,
                        "--prodxint=%s" % self.prodxint,
                        "--consprefs=%s" % self.consprefs,
                        "--consperc=%s" % self.consperc,
                        "--time=%s" % self.ttime,
                        "--failtime=%s" % self.failtime,
                        "--constime=%s" % self.ctime,
                        "--regrate=%s" % self.regrate,
                        "--fibsize=%s" % self.fibsize,
                        "--seed=%s" % self.seed,
                        "--samplei=%s" % self.samplei,
                        "--suffix=%s" % self.suffix,
                        ]
                f = open("./temp/cmdline.log","a")
                f.write("\nCMDLINE: "+ " ".join (cmdline))
                f.close()        
                job = SimulationJob (cmdline)
                pool.put (job)

    def postprocess (self):
        subprocess.call ("./graphs/strategy1.r", shell=True)
        # any postprocessing, if any
        pass


class SingleRun (Processor):
    def __init__ (self, name, script, stratset, nprefix, topo, controller, rate, hellorate, consxint, prodxint, consprefs, consperc, ttime, failtime, constime, regrate, fibsize, samplei, lang,  varx, maxXaxis, suffix):
        self.name = name
        self.script = script
        self.stratset = stratset
        self.nprefix = nprefix
        self.topo = topo
        self.controller = controller
        self.rate = rate
        self.hellorate = hellorate
        self.consxint = consxint
        self.prodxint = prodxint
        self.consprefs = consprefs
        self.consperc = consperc
        self.ttime = ttime
        self.failtime = failtime
        self.ctime = constime
        self.regrate = regrate
        self.fibsize = fibsize
        self.samplei = samplei
        self.lang = lang
        self.varx = varx
        self.maxXaxis = maxXaxis
        self.suffix = suffix
        f = open("graphs/variables.r","w")
        f.write("constime=%s + 1" % self.ctime)
        f.write("\nlang='%s'" % self.lang)        
        f.write("\nstratset=c(")
        for strat in self.stratset:
                f.write("'%s'," % strat)       
        f.write("'null')")
        f.write("\nvarx='%s'" % self.varx)
        f.write("\nvarx2=quote(%s)" % self.varx)        
        f.write("\nttime=%s" % self.ttime)
        f.write("\nmaxXaxis=%s" % self.maxXaxis)
        f.close()    
        # other initialization, if any

    def simulate (self):
        self.seed = random.randint(1,10000)
        self.cons = random.randint(1,10000)
        self.prod = random.randint(1,10000)
        #self.suffix = "var-%s-single-run" % (self.varx)
        os.mkdir("./temp/%s" % self.suffix)
        f = open("./graphs/varx.r","w")
        f.write("varx1='1'")
        f.write("\nsuffix='%s'" % self.suffix)
        f.close()
        for strat in self.stratset:           
                cmdline = ["LD_LIBRARY_PATH=/usr/local/lib",
                        "./ptimized/%s" % self.script,
                        "--strategy=%s" % strat,
                        "--nprefix=%s" % self.nprefix,
                        "--topo=%s" % self.topo,
                        "--consumer=%s" % self.cons,
                        "--producer=%s" % self.prod,
                        "--controller=%s" % self.controller,
                        "--rate=%s" % self.rate,
                        "--hellorate=%s" % self.hellorate,
                        "--consxint=%s" % self.consxint,
                        "--prodxint=%s" % self.prodxint,
                        "--consprefs=%s" % self.consprefs,
                        "--consperc=%s" % self.consperc,
                        "--time=%s" % self.ttime,
                        "--failtime=%s" % self.failtime,
                        "--constime=%s" % self.ctime,
                        "--regrate=%s" % self.regrate,
                        "--fibsize=%s" % self.fibsize,
                        "--seed=%s" % self.seed,
                        "--samplei=%s" % self.samplei,
                        "--suffix=%s" % self.suffix,
                        ]
                f = open("./temp/cmdline.log","a")
                f.write("\nCMDLINE: "+ " ".join (cmdline))
                f.close()        
                job = SimulationJob (cmdline)
                pool.put (job)

    def postprocess (self):
        subprocess.call ("./graphs/strategy.r", shell=True)
        # any postprocessing, if any
        pass

class Nruns (Processor):
    def __init__ (self, name, script, stratset, nprefix, topo, controller, rate, hellorate, consxint, prodxint, consprefs, consperc, ttime, failtime, constime, regrate, fibsize, samplei, lang,  varx, maxXaxis, suffix, runs):
        self.name = name
        self.script = script
        self.stratset = stratset
        self.nprefix = nprefix
        self.topo = topo
        self.controller = controller
        self.rate = rate
        self.hellorate = hellorate
        self.consxint = consxint
        self.prodxint = prodxint
        self.consprefs = consprefs
        self.consperc = consperc
        self.ttime = ttime
        self.failtime = failtime
        self.ctime = constime
        self.regrate = regrate
        self.fibsize = fibsize
        self.samplei = samplei
        self.lang = lang
        self.varx = varx
        self.maxXaxis = maxXaxis
        self.suffix = suffix
        self.runs = runs
        f = open("graphs/variables.r","w")
        f.write("constime=%s + 1" % self.ctime)
        f.write("\nlang='%s'" % self.lang)        
        f.write("\nstratset=c(")
        for strat in self.stratset:
                f.write("'%s'," % strat)       
        f.write("'null')")
        f.write("\nvarx='%s'" % self.varx)
        f.write("\nvarx2=quote(%s)" % self.varx)        
        f.write("\nttime=%s" % self.ttime)
        f.write("\nmaxXaxis=%s" % self.maxXaxis)
        f.close()    
        # other initialization, if any

    def simulate (self):
            run = 0
            while (run < self.runs):
                self.seed = random.randint(1,10000)
                self.cons = random.randint(1,10000)
                self.prod = random.randint(1,10000)
                self.suffix = "var-%s-run-%s" % (self.varx, run)
                #self.suffix = "var-%s-single-run" % (self.varx)
                os.mkdir("./temp/%s" % self.suffix)
                f = open("./graphs/varx.r","w")
                f.write("varx1='1'")
                f.write("\nsuffix='%s'" % self.suffix)
                f.close()
                for strat in self.stratset:           
                        cmdline = ["LD_LIBRARY_PATH=/usr/local/lib",
                                "./ptimized/%s" % self.script,
                                "--strategy=%s" % strat,
                                "--nprefix=%s" % self.nprefix,
                                "--topo=%s" % self.topo,
                                "--consumer=%s" % self.cons,
                                "--producer=%s" % self.prod,
                                "--controller=%s" % self.controller,
                                "--rate=%s" % self.rate,
                                "--hellorate=%s" % self.hellorate,
                                "--consxint=%s" % self.consxint,
                                "--prodxint=%s" % self.prodxint,
                                "--consprefs=%s" % self.consprefs,
                                "--consperc=%s" % self.consperc,
                                "--time=%s" % self.ttime,
                                "--failtime=%s" % self.failtime,
                                "--constime=%s" % self.ctime,
                                "--regrate=%s" % self.regrate,
                                "--fibsize=%s" % self.fibsize,
                                "--seed=%s" % self.seed,
                                "--samplei=%s" % self.samplei,
                                "--suffix=%s" % self.suffix,
                                ]
                        f = open("./temp/cmdline.log","a")
                        f.write("\nCMDLINE: " + " ".join (cmdline))
                        f.close()
                        job = SimulationJob (cmdline)
                        pool.put (job)
                run = run + 1

    def postprocess (self):
        # any postprocessing, if any
        f = open("temp/summary.txt","w")
        f.write("Strategy TimeSec ise\n")
        f.close()
        run = 0
        while (run < self.runs):
                self.suffix = "var-%s-run-%s" % (self.varx, run)
                f = open("./graphs/varx.r","w")
                f.write("varx1='1'")
                f.write("\nsuffix='%s'" % self.suffix)
                f.close()               
                subprocess.call ("./graphs/strategy.r", shell=True)
                os.rename("temp/SignallingEfficiency-Time.png","temp/SignallingEfficiency-Time-%s.png" % self.suffix)        
                run = run + 1
        subprocess.call ("./graphs/strategy-95.r", shell=True)

class NPrefix (Processor):
    def __init__ (self, name, script, stratset, nprefix, topo, controller, rate, hellorate, consxint, prodxint, consprefs, consperc, ttime, failtime, constime, regrate, fibsize, samplei, lang,  varx, maxXaxis, suffix, runs, step):
        self.name = name
        self.script = script
        self.stratset = stratset
        self.nprefix = nprefix
        self.topo = topo
        self.controller = controller
        self.rate = rate
        self.hellorate = hellorate
        self.consxint = consxint
        self.prodxint = prodxint
        self.consprefs = consprefs
        self.consperc = consperc
        self.ttime = ttime
        self.failtime = failtime
        self.ctime = constime
        self.regrate = regrate
        self.fibsize = fibsize
        self.samplei = samplei
        self.lang = lang
        self.varx = varx
        self.maxXaxis = maxXaxis
        self.suffix = suffix
        self.runs = runs
        self.step = step
        f = open("graphs/variables.r","w")
        f.write("constime=%s + 1" % self.ctime)
        f.write("\nlang='%s'" % self.lang)        
        f.write("\nstratset=c(")
        for strat in self.stratset:
                f.write("'%s'," % strat)       
        f.write("'null')")
        f.write("\nvarx='%s'" % self.varx)
        f.write("\nvarx2=quote(%s)" % self.varx)        
        f.write("\nttime=%s" % self.ttime)
        f.write("\nmaxXaxis=%s" % self.maxXaxis)
        f.close()    
        # other initialization, if any

    def simulate (self):
        #counter = self.nprefix
        #while (counter < self.maxXaxis):
        for counter in self.nprefix:
            run = 0
            while (run < self.runs):
                self.seed = random.randint(1,10000)
                self.cons = random.randint(1,10000)
                self.prod = random.randint(1,10000)
                self.suffix = "var-%s-value-%s-run-%s" % (self.varx, counter, run)
                os.mkdir("./temp/%s" % self.suffix)
                f = open("./graphs/varx.r","w")
                f.write("varx1='%s'" % counter)
                f.write("\nsuffix='%s'" % self.suffix)
                f.close()
                for strat in self.stratset:           
                        cmdline = ["LD_LIBRARY_PATH=/usr/local/lib",
                                "./ptimized/%s" % self.script,
                                "--strategy=%s" % strat,
                                "--nprefix=%s" % counter,
                                "--topo=%s" % self.topo,
                                "--consumer=%s" % self.cons,
                                "--producer=%s" % self.prod,
                                "--controller=%s" % self.controller,
                                "--rate=%s" % self.rate,
                                "--hellorate=%s" % self.hellorate,
                                "--consxint=%s" % self.consxint,
                                "--prodxint=%s" % self.prodxint,
                                "--consprefs=%s" % counter,
                                "--consperc=%s" % self.consperc,
                                "--time=%s" % self.ttime,
                                "--failtime=%s" % self.failtime,
                                "--constime=%s" % self.ctime,
                                "--regrate=%s" % self.regrate,
                                "--fibsize=%s" % self.fibsize,
                                "--seed=%s" % self.seed,
                                "--samplei=%s" % self.samplei,
                                "--suffix=%s" % self.suffix,
                                ]
                        f = open("./temp/cmdline.log","a")
                        f.write("\nCMDLINE: "+" ".join (cmdline))
                        f.close()                               
                        job = SimulationJob (cmdline)
                        pool.put (job)
                run = run + 1
            #counter = counter + self.step

    def postprocess (self):
        # any postprocessing, if any
        #counter = self.nprefix
        f = open("temp/summaryXaxis.txt","w")
        f.write("%s Strategy ise\n" % self.varx)
        f.close()
        #while (counter < self.maxXaxis):
        for counter in self.nprefix:
            f = open("temp/summary.txt","w")
            f.write("Strategy TimeSec ise\n")
            f.close()
            run = 0
            while (run < self.runs):
                self.suffix = "var-%s-value-%s-run-%s" % (self.varx, counter, run)
                f = open("./graphs/varx.r","w")
                f.write("varx1='%s'" % counter)
                f.write("\nsuffix='%s'" % self.suffix)
                f.close()               
                subprocess.call ("./graphs/strategy.r", shell=True)
                os.rename("temp/SignallingEfficiency-Time.png","temp/SignallingEfficiency-Time-%s.png" % self.suffix)        
                run = run + 1
            subprocess.call ("./graphs/strategy-95.r", shell=True)
            #counter = counter + self.step
            os.rename("temp/summary.txt","temp/summary-%s.txt" % self.suffix)
            os.rename("temp/Error.png","temp/Error-%s.png" % self.suffix)
        subprocess.call ("./graphs/strategy-xaxis.r", shell=True)            


class FibMFail (Processor):
    def __init__ (self, name, script, stratset, nprefix, topo, controller, rate, hellorate, consxint, prodxint, consprefs, consperc, ttime, failtime, constime, regrate, fibsize, samplei, lang,  varx, maxXaxis, suffix, runs, step):
        self.name = name
        self.script = script
        self.stratset = stratset
        self.nprefix = nprefix
        self.topo = topo
        self.controller = controller
        self.rate = rate
        self.hellorate = hellorate
        self.consxint = consxint
        self.prodxint = prodxint
        self.consprefs = consprefs
        self.consperc = consperc
        self.ttime = ttime
        self.failtime = failtime
        self.ctime = constime
        self.regrate = regrate
        self.fibsize = fibsize
        self.samplei = samplei
        self.lang = lang
        self.varx = varx
        self.maxXaxis = maxXaxis
        self.suffix = suffix
        self.runs = runs
        self.step = step
        f = open("graphs/variables.r","w")
        f.write("constime=%s + 1" % self.ctime)
        f.write("\nlang='%s'" % self.lang)        
        f.write("\nstratset=c(")
        for strat in self.stratset:
                f.write("'%s'," % strat)       
        f.write("'null')")
        f.write("\nvarx='%s'" % self.varx)
        f.write("\nvarx2=quote(%s)" % self.varx)        
        f.write("\nttime=%s" % self.ttime)
        f.write("\nmaxXaxis=%s" % self.maxXaxis)
        f.close()    
        # other initialization, if any

    def simulate (self):
        #counter = self.consperc
        #while (counter < self.maxXaxis):
        for counter in self.consperc:
            run = 0
            while (run < self.runs):
                self.seed = random.randint(1,10000)
                self.cons = random.randint(1,10000)
                self.prod = random.randint(1,10000)
                self.suffix = "var-%s-value-%s-run-%s" % (self.varx, counter, run)
                os.mkdir("./temp/%s" % self.suffix)
                f = open("./graphs/varx.r","w")
                f.write("varx1='%s'" % counter)
                f.write("\nsuffix='%s'" % self.suffix)
                f.close()
                for strat in self.stratset:           
                        cmdline = ["LD_LIBRARY_PATH=/usr/local/lib",
                                "./ptimized/%s" % self.script,
                                "--strategy=%s" % strat,
                                "--nprefix=%s" % self.nprefix,
                                "--topo=%s" % self.topo,
                                "--consumer=%s" % self.cons,
                                "--producer=%s" % self.prod,
                                "--controller=%s" % self.controller,
                                "--rate=%s" % self.rate,
                                "--hellorate=%s" % self.hellorate,
                                "--consxint=%s" % self.consxint,
                                "--prodxint=%s" % self.prodxint,
                                "--consprefs=%s" % self.consprefs,
                                "--consperc=%s" % counter,
                                "--time=%s" % self.ttime,
                                "--failtime=%s" % self.failtime,
                                "--constime=%s" % self.ctime,
                                "--regrate=%s" % self.regrate,
                                "--fibsize=%s" % self.fibsize,
                                "--seed=%s" % self.seed,
                                "--samplei=%s" % self.samplei,
                                "--suffix=%s" % self.suffix,
                                ]
                        f = open("./temp/cmdline.log","a")
                        f.write("\nCMDLINE: "+" ".join (cmdline))
                        f.close()                               
                        job = SimulationJob (cmdline)
                        pool.put (job)
                run = run + 1
            #counter = counter + self.step

    def postprocess (self):
        # any postprocessing, if any
        #counter = self.consperc
        f = open("temp/summaryXaxis.txt","w")
        f.write("%s Strategy ise\n" % self.varx)
        f.close()
        #while (counter < self.maxXaxis):
        for counter in self.consperc:
            f = open("temp/summary.txt","w")
            f.write("Strategy TimeSec ise\n")
            f.close()
            run = 0
            while (run < self.runs):
                self.suffix = "var-%s-value-%s-run-%s" % (self.varx, counter, run)
                f = open("./graphs/varx.r","w")
                f.write("varx1='%s'" % counter)
                f.write("\nsuffix='%s'" % self.suffix)
                f.close()               
                subprocess.call ("./graphs/strategy.r", shell=True)
                os.rename("temp/SignallingEfficiency-Time.png","temp/SignallingEfficiency-Time-%s.png" % self.suffix)        
                run = run + 1
            subprocess.call ("./graphs/strategy-95.r", shell=True)
            #counter = counter + self.step
            os.rename("temp/summary.txt","temp/summary-%s.txt" % self.suffix)
            os.rename("temp/Error.png","temp/Error-%s.png" % self.suffix)
        subprocess.call ("./graphs/strategy-xaxis.r", shell=True)            

class RRate (Processor):
    def __init__ (self, name, script, stratset, nprefix, topo, controller, rate, hellorate, consxint, prodxint, consprefs, consperc, ttime, failtime, constime, regrate, fibsize, samplei, lang,  varx, maxXaxis, suffix, runs, step):
        self.name = name
        self.script = script
        self.stratset = stratset
        self.nprefix = nprefix
        self.topo = topo
        self.controller = controller
        self.rate = rate
        self.hellorate = hellorate
        self.consxint = consxint
        self.prodxint = prodxint
        self.consprefs = consprefs
        self.consperc = consperc
        self.ttime = ttime
        self.failtime = failtime
        self.ctime = constime
        self.regrate = regrate
        self.fibsize = fibsize
        self.samplei = samplei
        self.lang = lang
        self.varx = varx
        self.maxXaxis = maxXaxis
        self.suffix = suffix
        self.runs = runs
        self.step = step
        f = open("graphs/variables.r","w")
        f.write("constime=%s + 1" % self.ctime)
        f.write("\nlang='%s'" % self.lang)        
        f.write("\nstratset=c(")
        for strat in self.stratset:
                f.write("'%s'," % strat)       
        f.write("'null')")
        f.write("\nvarx='%s'" % self.varx)
        f.write("\nvarx2=quote(%s)" % self.varx)        
        f.write("\nttime=%s" % self.ttime)
        f.write("\nmaxXaxis=%s" % self.maxXaxis)
        f.close()    
        # other initialization, if any

    def simulate (self):
        #counter = self.rate
        #while (counter < self.maxXaxis):
        for counter in self.rate:
            run = 0
            while (run < self.runs):
                self.seed = random.randint(1,10000)
                self.cons = random.randint(1,10000)
                self.prod = random.randint(1,10000)
                self.suffix = "var-%s-value-%s-run-%s" % (self.varx, counter, run)
                os.mkdir("./temp/%s" % self.suffix)
                f = open("./graphs/varx.r","w")
                f.write("varx1='%s'" % counter)
                f.write("\nsuffix='%s'" % self.suffix)
                f.close()
                for strat in self.stratset:           
                        cmdline = ["LD_LIBRARY_PATH=/usr/local/lib",
                                "./ptimized/%s" % self.script,
                                "--strategy=%s" % strat,
                                "--nprefix=%s" % self.nprefix,
                                "--topo=%s" % self.topo,
                                "--consumer=%s" % self.cons,
                                "--producer=%s" % self.prod,
                                "--controller=%s" % self.controller,
                                "--rate=%s" % counter,
                                "--hellorate=%s" % self.hellorate,
                                "--consxint=%s" % self.consxint,
                                "--prodxint=%s" % self.prodxint,
                                "--consprefs=%s" % self.consprefs,
                                "--consperc=%s" % self.consperc,
                                "--time=%s" % self.ttime,
                                "--failtime=%s" % self.failtime,
                                "--constime=%s" % self.ctime,
                                "--regrate=%s" % self.regrate,
                                "--fibsize=%s" % self.fibsize,
                                "--seed=%s" % self.seed,
                                "--samplei=%s" % self.samplei,
                                "--suffix=%s" % self.suffix,
                                ]
                        f = open("./temp/cmdline.log","a")
                        f.write("\nCMDLINE: "+" ".join (cmdline))
                        f.close()                               
                        job = SimulationJob (cmdline)
                        pool.put (job)
                run = run + 1
            #counter = counter + self.step

    def postprocess (self):
        # any postprocessing, if any
        #counter = self.rate
        f = open("temp/summaryXaxis.txt","w")
        f.write("%s Strategy ise\n" % self.varx)
        f.close()
        for counter in self.rate:
        #while (counter < self.maxXaxis):
            f = open("temp/summary.txt","w")
            f.write("Strategy TimeSec ise\n")
            f.close()
            run = 0
            while (run < self.runs):
                self.suffix = "var-%s-value-%s-run-%s" % (self.varx, counter, run)
                f = open("./graphs/varx.r","w")
                f.write("varx1='%s'" % counter)
                f.write("\nsuffix='%s'" % self.suffix)
                f.close()               
                subprocess.call ("./graphs/strategy.r", shell=True)
                os.rename("temp/SignallingEfficiency-Time.png","temp/SignallingEfficiency-Time-%s.png" % self.suffix)        
                run = run + 1
            subprocess.call ("./graphs/strategy-95.r", shell=True)
            #counter = counter + self.step
            os.rename("temp/summary.txt","temp/summary-%s.txt" % self.suffix)
            os.rename("temp/Error.png","temp/Error-%s.png" % self.suffix)
        subprocess.call ("./graphs/strategy-xaxis.r", shell=True)            

class Topo (Processor):
    def __init__ (self, name, script, stratset, nprefix, topo, controller, rate, hellorate, consxint, prodxint, consprefs, consperc, ttime, failtime, constime, regrate, fibsize, samplei, lang,  varx, maxXaxis, suffix, runs, step):
        self.name = name
        self.script = script
        self.stratset = stratset
        self.nprefix = nprefix
        self.topo = topo
        self.controller = controller
        self.rate = rate
        self.hellorate = hellorate
        self.consxint = consxint
        self.prodxint = prodxint
        self.consprefs = consprefs
        self.consperc = consperc
        self.ttime = ttime
        self.failtime = failtime
        self.ctime = constime
        self.regrate = regrate
        self.fibsize = fibsize
        self.samplei = samplei
        self.lang = lang
        self.varx = varx
        self.maxXaxis = maxXaxis
        self.suffix = suffix
        self.runs = runs
        self.step = step
        f = open("graphs/variables.r","w")
        f.write("constime=%s + 1" % self.ctime)
        f.write("\nlang='%s'" % self.lang)        
        f.write("\nstratset=c(")
        for strat in self.stratset:
                f.write("'%s'," % strat)       
        f.write("'null')")
        f.write("\nvarx='%s'" % self.varx)
        f.write("\nvarx2=quote(%s)" % self.varx)        
        f.write("\nttime=%s" % self.ttime)
        f.write("\nmaxXaxis=%s" % self.maxXaxis)
        f.close()    
        # other initialization, if any

    def simulate (self):
        for counter in self.topo:
            run = 0
            while (run < self.runs):
                self.seed = random.randint(1,10000)
                self.cons = random.randint(1,10000)
                self.prod = random.randint(1,10000)
                self.suffix = "var-%s-value-%s-run-%s" % (self.varx, counter, run)
                os.mkdir("./temp/%s" % self.suffix)
                f = open("./graphs/varx.r","w")
                f.write("varx1='%s'" % counter)
                f.write("\nsuffix='%s'" % self.suffix)
                f.close()
                for strat in self.stratset:           
                        cmdline = ["LD_LIBRARY_PATH=/usr/local/lib",
                                "./ptimized/%s" % self.script,
                                "--strategy=%s" % strat,
                                "--nprefix=%s" % self.nprefix,
                                "--topo=%s" % counter,
                                "--consumer=%s" % self.cons,
                                "--producer=%s" % self.prod,
                                "--controller=%s" % self.controller,
                                "--rate=%s" % self.rate,
                                "--hellorate=%s" % self.hellorate,
                                "--consxint=%s" % self.consxint,
                                "--prodxint=%s" % self.prodxint,
                                "--consprefs=%s" % self.consprefs,
                                "--consperc=%s" % self.consperc,
                                "--time=%s" % self.ttime,
                                "--failtime=%s" % self.failtime,
                                "--constime=%s" % self.ctime,
                                "--regrate=%s" % self.regrate,
                                "--fibsize=%s" % self.fibsize,
                                "--seed=%s" % self.seed,
                                "--samplei=%s" % self.samplei,
                                "--suffix=%s" % self.suffix,
                                ]
                        f = open("./temp/cmdline.log","a")
                        f.write("\nCMDLINE: "+" ".join (cmdline))
                        f.close()                               
                        job = SimulationJob (cmdline)
                        pool.put (job)
                run = run + 1

    def postprocess (self):
        # any postprocessing, if any
        f = open("temp/summaryXaxis.txt","w")
        f.write("%s Strategy ise\n" % self.varx)
        f.close()
        for counter in self.topo:
            f = open("temp/summary.txt","w")
            f.write("Strategy TimeSec ise\n")
            f.close()
            run = 0
            while (run < self.runs):
                self.suffix = "var-%s-value-%s-run-%s" % (self.varx, counter, run)
                f = open("./graphs/varx.r","w")
                f.write("varx1='%s'" % counter)
                f.write("\nsuffix='%s'" % self.suffix)
                f.close()               
                subprocess.call ("./graphs/strategy.r", shell=True)
                os.rename("temp/SignallingEfficiency-Time.png","temp/SignallingEfficiency-Time-%s.png" % self.suffix)        
                run = run + 1
            subprocess.call ("./graphs/strategy-95.r", shell=True)
            os.rename("temp/summary.txt","temp/summary-%s.txt" % self.suffix)
            os.rename("temp/Error.png","temp/Error-%s.png" % self.suffix)
        subprocess.call ("./graphs/strategy-xaxis.r", shell=True)            

class NConsumers (Processor):
    def __init__ (self, name, script, stratset, nprefix, topo, controller, rate, hellorate, consxint, prodxint, consprefs, consperc, ttime, failtime, constime, regrate, fibsize, samplei, lang,  varx, maxXaxis, suffix, runs, step):
        self.name = name
        self.script = script
        self.stratset = stratset
        self.nprefix = nprefix
        self.topo = topo
        self.controller = controller
        self.rate = rate
        self.hellorate = hellorate
        self.consxint = consxint
        self.prodxint = prodxint
        self.consprefs = consprefs
        self.consperc = consperc
        self.ttime = ttime
        self.failtime = failtime
        self.ctime = constime
        self.regrate = regrate
        self.fibsize = fibsize
        self.samplei = samplei
        self.lang = lang
        self.varx = varx
        self.maxXaxis = maxXaxis
        self.suffix = suffix
        self.runs = runs
        self.step = step
        f = open("graphs/variables.r","w")
        f.write("constime=%s + 1" % self.ctime)
        f.write("\nlang='%s'" % self.lang)        
        f.write("\nstratset=c(")
        for strat in self.stratset:
                f.write("'%s'," % strat)       
        f.write("'null')")
        f.write("\nvarx='%s'" % self.varx)
        f.write("\nvarx2=quote(%s)" % self.varx)        
        f.write("\nttime=%s" % self.ttime)
        f.write("\nmaxXaxis=%s" % self.maxXaxis)
        f.close()    
        # other initialization, if any

    def simulate (self):
        #counter = self.consxint
        #while (counter < self.maxXaxis):
        for counter in self.consxint:
            run = 0
            while (run < self.runs):
                self.seed = random.randint(1,10000)
                self.cons = random.randint(1,10000)
                self.prod = random.randint(1,10000)
                self.suffix = "var-%s-value-%s-run-%s" % (self.varx, counter, run)
                os.mkdir("./temp/%s" % self.suffix)
                f = open("./graphs/varx.r","w")
                f.write("varx1='%s'" % counter)
                f.write("\nsuffix='%s'" % self.suffix)
                f.close()
                for strat in self.stratset:           
                        cmdline = ["LD_LIBRARY_PATH=/usr/local/lib",
                                "./ptimized/%s" % self.script,
                                "--strategy=%s" % strat,
                                "--nprefix=%s" % self.nprefix,
                                "--topo=%s" % self.topo,
                                "--consumer=%s" % self.cons,
                                "--producer=%s" % self.prod,
                                "--controller=%s" % self.controller,
                                "--rate=%s" % self.rate,
                                "--hellorate=%s" % self.hellorate,
                                "--consxint=%s" % counter,
                                "--prodxint=%s" % self.prodxint,
                                "--consprefs=%s" % self.consprefs,
                                "--consperc=%s" % self.consperc,
                                "--time=%s" % self.ttime,
                                "--failtime=%s" % self.failtime,
                                "--constime=%s" % self.ctime,
                                "--regrate=%s" % self.regrate,
                                "--fibsize=%s" % self.fibsize,
                                "--seed=%s" % self.seed,
                                "--samplei=%s" % self.samplei,
                                "--suffix=%s" % self.suffix,
                                ]
                        f = open("./temp/cmdline.log","a")
                        f.write("\nCMDLINE: "+" ".join (cmdline))
                        f.close()                               
                        job = SimulationJob (cmdline)
                        pool.put (job)
                run = run + 1
            #counter = counter + self.step

    def postprocess (self):
        # any postprocessing, if any
        #counter = self.consxint
        f = open("temp/summaryXaxis.txt","w")
        f.write("%s Strategy ise\n" % self.varx)
        f.close()
        #while (counter < self.maxXaxis):
        for counter in self.consxint:
            f = open("temp/summary.txt","w")
            f.write("Strategy TimeSec ise\n")
            f.close()
            run = 0
            while (run < self.runs):
                self.suffix = "var-%s-value-%s-run-%s" % (self.varx, counter, run)
                f = open("./graphs/varx.r","w")
                f.write("varx1='%s'" % counter)
                f.write("\nsuffix='%s'" % self.suffix)
                f.close()               
                subprocess.call ("./graphs/strategy.r", shell=True)
                os.rename("temp/SignallingEfficiency-Time.png","temp/SignallingEfficiency-Time-%s.png" % self.suffix)        
                run = run + 1
            subprocess.call ("./graphs/strategy-95.r", shell=True)
            #counter = counter + self.step
            os.rename("temp/summary.txt","temp/summary-%s.txt" % self.suffix)
            os.rename("temp/Error.png","temp/Error-%s.png" % self.suffix)
        subprocess.call ("./graphs/strategy-xaxis.r", shell=True)            

class NConsumersFixRate (Processor):
    def __init__ (self, name, script, stratset, nprefix, topo, controller, rate, hellorate, consxint, prodxint, consprefs, consperc, ttime, failtime, constime, regrate, fibsize, samplei, lang,  varx, maxXaxis, suffix, runs, step):
        self.name = name
        self.script = script
        self.stratset = stratset
        self.nprefix = nprefix
        self.topo = topo
        self.controller = controller
        self.rate = rate
        self.hellorate = hellorate
        self.consxint = consxint
        self.prodxint = prodxint
        self.consprefs = consprefs
        self.consperc = consperc
        self.ttime = ttime
        self.failtime = failtime
        self.ctime = constime
        self.regrate = regrate
        self.fibsize = fibsize
        self.samplei = samplei
        self.lang = lang
        self.varx = varx
        self.maxXaxis = maxXaxis
        self.suffix = suffix
        self.runs = runs
        self.step = step
        f = open("graphs/variables.r","w")
        f.write("constime=%s + 1" % self.ctime)
        f.write("\nlang='%s'" % self.lang)        
        f.write("\nstratset=c(")
        for strat in self.stratset:
                f.write("'%s'," % strat)       
        f.write("'null')")
        f.write("\nvarx='%s'" % self.varx)
        f.write("\nvarx2=quote(%s)" % self.varx)        
        f.write("\nttime=%s" % self.ttime)
        f.write("\nmaxXaxis=%s" % self.maxXaxis)
        f.close()    
        # other initialization, if any

    def simulate (self):
        #counter = self.consxint
        #localrate = self.rate
        #while (counter < self.maxXaxis):
        for counter in self.consxint:
            localrate = self.rate / counter
            run = 0
            while (run < self.runs):
                self.seed = random.randint(1,10000)
                self.cons = random.randint(1,10000)
                self.prod = random.randint(1,10000)
                self.suffix = "var-%s-value-%s-run-%s" % (self.varx, counter, run)
                os.mkdir("./temp/%s" % self.suffix)
                f = open("./graphs/varx.r","w")
                f.write("varx1='%s'" % counter)
                f.write("\nsuffix='%s'" % self.suffix)
                f.close()
                for strat in self.stratset:           
                        cmdline = ["LD_LIBRARY_PATH=/usr/local/lib",
                                "./ptimized/%s" % self.script,
                                "--strategy=%s" % strat,
                                "--nprefix=%s" % self.nprefix,
                                "--topo=%s" % self.topo,
                                "--consumer=%s" % self.cons,
                                "--producer=%s" % self.prod,
                                "--controller=%s" % self.controller,
                                "--rate=%s" % localrate,
                                "--hellorate=%s" % self.hellorate,
                                "--consxint=%s" % counter,
                                "--prodxint=%s" % self.prodxint,
                                "--consprefs=%s" % self.consprefs,
                                "--consperc=%s" % self.consperc,
                                "--time=%s" % self.ttime,
                                "--failtime=%s" % self.failtime,
                                "--constime=%s" % self.ctime,
                                "--regrate=%s" % self.regrate,
                                "--fibsize=%s" % self.fibsize,
                                "--seed=%s" % self.seed,
                                "--samplei=%s" % self.samplei,
                                "--suffix=%s" % self.suffix,
                                ]
                        f = open("./temp/cmdline.log","a")
                        f.write("\nCMDLINE: "+" ".join (cmdline))
                        f.close()                               
                        job = SimulationJob (cmdline)
                        pool.put (job)
                run = run + 1
            #localrate = self.rate / counter
            #counter = counter + self.step

    def postprocess (self):
        # any postprocessing, if any
        #counter = self.consxint
        f = open("temp/summaryXaxis.txt","w")
        f.write("%s Strategy ise\n" % self.varx)
        f.close()
        #while (counter < self.maxXaxis):
        for counter in self.consxint:
            f = open("temp/summary.txt","w")
            f.write("Strategy TimeSec ise\n")
            f.close()
            run = 0
            while (run < self.runs):
                self.suffix = "var-%s-value-%s-run-%s" % (self.varx, counter, run)
                f = open("./graphs/varx.r","w")
                f.write("varx1='%s'" % counter)
                f.write("\nsuffix='%s'" % self.suffix)
                f.close()               
                subprocess.call ("./graphs/strategy.r", shell=True)
                os.rename("temp/SignallingEfficiency-Time.png","temp/SignallingEfficiency-Time-%s.png" % self.suffix)        
                run = run + 1
            subprocess.call ("./graphs/strategy-95.r", shell=True)
            #counter = counter + self.step
            os.rename("temp/summary.txt","temp/summary-%s.txt" % self.suffix)
            os.rename("temp/Error.png","temp/Error-%s.png" % self.suffix)
        subprocess.call ("./graphs/strategy-xaxis.r", shell=True)            
		

try:
    # Simulation, processing, and graph building
    
    stratset1=["ARPLike",
               "CRoS-NDN",
               "Flooding",
               "NLSRLike",
               "Omniscient",
               "OSPFLike",
               ]
    
    stratset2=["ARPLike",
               "CRoS-NDN",
               "Omniscient",
               ]   
               
    stratset3=["ARPLike",
               "CRoS-NDN",
               "NLSRLike",
               "Omniscient",
               ]                 
    
    stratset4=["CRoS-NDN",
               "NLSRLike",
               "Omniscient",
               ]  

    stratset5=["CRoS-NDN",]
    
    toposet1=["topo-3-way.txt", "1755.txt",]
    
    toposet2=["4755.txt", "topo-3-way.txt", "1221.txt", "1755.txt", "3257.txt",]
    
    toposet3=["4755.txt", "topo-3-way.txt", "1221.txt", "1755.txt", "3257.txt", "3967.txt", "6461.txt",]

    toposet4=["topo-3-way.txt",]
    
    singlerun = SingleRun (name="single-run",
                      script="strategy-comparev4",
                      stratset=stratset4,
                      nprefix=10,
                      topo="4755.txt",
                      controller=0,
                      rate=200,
                      hellorate=0.01,
                      consxint=1,
                      prodxint=0,
                      consprefs=1,
                      consperc=10,
                      ttime=3001,
                      failtime=0,
                      constime=0,
                      regrate=20,
                      fibsize=100,
                      samplei=200,
                      lang="en",
                      varx="nprefix",
                      maxXaxis=11,
                      suffix="single-run"
                      )
    singlerun.run ()
    
    nruns = Nruns (name="nruns",
                      script="strategy-comparev3",
                      stratset=stratset4,
                      nprefix=10,
                      topo="1755.txt",
                      controller=0,
                      rate=20,
                      hellorate=0.01,
                      consxint=1,
                      prodxint=0,
                      consprefs=10,
                      consperc=100,
                      ttime=5001,
                      failtime=0,
                      constime=0,
                      regrate=20,
                      fibsize=100,
                      samplei=200,
                      lang="en",
                      varx="nprefix",
                      maxXaxis=1,
                      suffix="",
                      runs=2
                      )
    nruns.run ()
    
    npref = NPrefix (name="nprefix",
                      script="strategy-comparev3",
                      stratset=stratset1,
                      nprefix=[1, 10, 20],
                      topo="1755.txt",
                      controller=0,
                      rate=20,
                      hellorate=0.01,
                      consxint=1,
                      prodxint=0,
                      consprefs=1,
                      consperc=10,
                      ttime=5001,
                      failtime=0,
                      constime=0,
                      regrate=20,
                      fibsize=100,
                      samplei=200,
                      lang="en",
                      varx="nprefix",
                      maxXaxis=5,
                      suffix="",
                      runs=10,
                      step=2
                      )
    npref.run ()


    nprefsmalltopo = NPrefix (name="nprefsmalltopo",
                      script="strategy-compare",
                      stratset=stratset1,    
                      nprefix=10,
                      topo="topo-3-way.txt",
                      controller=0,
                      rate=20,
                      hellorate=0.01,
                      consxint=1,
                      prodxint=0,
                      consprefs=10,
                      consperc=10,
                      ttime=3001,
                      failtime=0,
                      constime=0,
                      regrate=20,
                      fibsize=20,
                      samplei=200,
                      lang="en",
                      varx="nprefix",
                      maxXaxis=111,
                      suffix="",
                      runs=10,
                      step=100
                      )
    nprefsmalltopo.run ()


    fibmfail = FibMFail (name="fibmfail",
                      script="strategy-comparev3",
                      stratset=stratset1,
                      nprefix=10,
                      topo="1755.txt",
                      controller=0,
                      rate=20,
                      hellorate=0.01,
                      consxint=1,
                      prodxint=0,
                      consprefs=10,
                      consperc=[1, 10, 100],
                      ttime=5001,
                      failtime=0,
                      constime=0,
                      regrate=20,
                      fibsize=100,
                      samplei=200,
                      lang="en",
                      varx="consperc",
                      maxXaxis=101,
                      suffix="",
                      runs=10,
                      step=90
                      )
    fibmfail.run ()

    fibmfailnotimeout = FibMFail (name="fibmfailnotimeout",
                      script="strategy-comparev2",
                      stratset=stratset1,
                      nprefix=3,
                      topo="1755.txt",
                      controller=0,
                      rate=20,
                      hellorate=0.01,
                      consxint=1,
                      prodxint=0,
                      consprefs=3,
                      consperc=[1, 10, 100],
                      ttime=3001,
                      failtime=0,
                      constime=0,
                      regrate=20,
                      fibsize=100,
                      samplei=200,
                      lang="en",
                      varx="consperc",
                      maxXaxis=101,
                      suffix="",
                      runs=10,
                      step=90
                      )
    fibmfailnotimeout.run ()

    fibmfailindcons = FibMFail (name="fibmfailindcons",
                      script="indep-cons",
                      stratset=stratset1,
                      nprefix=3,
                      topo="1755.txt",
                      controller=0,
                      rate=2,
                      hellorate=0.01,
                      consxint=1,
                      prodxint=0,
                      consprefs=3,
                      consperc=[1, 10, 100],
                      ttime=3001,
                      failtime=0,
                      constime=0,
                      regrate=20,
                      fibsize=100,
                      samplei=200,
                      lang="en",
                      varx="consperc",
                      maxXaxis=101,
                      suffix="",
                      runs=10,
                      step=90
                      )
    fibmfailindcons.run ()
	

    rrate = RRate (name="rrate",
                      script="strategy-comparev3",
                      stratset=stratset1,
                      nprefix=3,
                      topo="1755.txt",
                      controller=0,
                      rate=[2, 20, 200],
                      hellorate=0.01,
                      consxint=1,
                      prodxint=0,
                      consprefs=3,
                      consperc=10,
                      ttime=4001,
                      failtime=0,
                      constime=0,
                      regrate=20,
                      fibsize=100,
                      samplei=200,
                      lang="en",
                      varx="rate",
                      maxXaxis=201,
                      suffix="",
                      runs=10,
                      step=180
                      )
    rrate.run ()

    LargeToposet = Topo (name="LargeToposet",
                      script="strategy-comparev3",
                      stratset=stratset1,
                      nprefix=120,
                      topo=toposet2,
                      controller=0,
                      rate=20,
                      hellorate=0.01,
                      consxint=1,
                      prodxint=1,
                      consprefs=120,
                      consperc=10,
                      ttime=3001,
                      failtime=0,
                      constime=0,
                      regrate=20,
                      fibsize=100,
                      samplei=200,
                      lang="en",
                      varx="topo",
                      maxXaxis=101,
                      suffix="",
                      runs=10,
                      step=20
                      )
    LargeToposet.run ()
    
    SmallTopoSet = Topo (name="SmallTopoSet",
                      script="strategy-comparev3",
                      stratset=stratset1,
                      nprefix=3,
                      topo=toposet1,
                      controller=0,
                      rate=20,
                      hellorate=0.01,
                      consxint=1,
                      prodxint=0,
                      consprefs=3,
                      consperc=10,
                      ttime=3001,
                      failtime=0,
                      constime=0,
                      regrate=20,
                      fibsize=100,
                      samplei=200,
                      lang="en",
                      varx="topo",
                      maxXaxis=101,
                      suffix="",
                      runs=10,
                      step=20
                      )
    SmallTopoSet.run ()

    nconsumers = NConsumers (name="nconsumers",
                      script="strategy-comparev3",
                      stratset=stratset1,
                      nprefix=3,
                      topo="1755.txt",
                      controller=0,
                      rate=20,
                      hellorate=0.01,
                      consxint=[1, 10, 20],
                      prodxint=0,
                      consprefs=3,
                      consperc=10,
                      ttime=3001,
                      failtime=0,
                      constime=0,
                      regrate=20,
                      fibsize=100,
                      samplei=200,
                      lang="en",
                      varx="consxint",
                      maxXaxis=12,
                      suffix="",
                      runs=10,
                      step=9
                      )
    nconsumers.run ()

    nconsumersfixrate = NConsumersFixRate (name="nconsumersfixrate",
                      script="strategy-comparev5",
                      stratset=stratset1,
                      nprefix=110,
                      topo="1755.txt",
                      controller=0,
                      rate=20,
                      hellorate=0.01,
                      consxint=[1, 10, 20],
                      prodxint=1,
                      consprefs=110,
                      consperc=10,
                      ttime=3001,
                      failtime=0,
                      constime=0,
                      regrate=20,
                      fibsize=100,
                      samplei=200,
                      lang="en",
                      varx="consxint",
                      maxXaxis=12,
                      suffix="",
                      runs=10,
                      step=5
                      )
    nconsumersfixrate.run ()


    experiment = Base (name="test",
                      script="linkfail",
                      stratset=stratset5,
                      nprefix=10,
                      topo="topo-3-way.txt",
                      controller="Ctr1",
                      rate=20,
                      hellorate=0.1,
                      consxint=1,
		      cons = "Src1",
                      prodxint=1,
		      prod = "Prd1",
                      consprefs=1,
                      consperc=10,
                      ttime=3001,
                      failtime=0,
                      constime=0,
                      regrate=20,
                      fibsize=100,
                      samplei=50,
                      lang="en",
                      varx="nprefix",
                      maxXaxis=11,
                      suffix="single-run"
                      )
    experiment.run ()
    
finally:
    pool.join ()
    pool.shutdown ()
    

