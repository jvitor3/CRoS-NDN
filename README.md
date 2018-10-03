CRoS-NDN

This is the simulation code developed in the CRoS-NDN research.
CRoS-NDN - Controller-based Routing Scheme for Named-Data Networking

References:
Torres, J.V., Boutaba, R., and Duarte, O.C.M.B. - "An Autonomous and Efficient Controller-based Routing Scheme for Networking Named-Data Mobility", in Computer Communications 2016. 
English, A4 size, 12 p.
http://www.gta.ufrj.br/ftp/gta/TechReports/TBD16.pdf

Torres, J. V., Alvarenga, I. D., Boutaba, R., and Duarte, O. C. M. B. - "Evaluating CRoS-NDN: A comparative performance analysis of the Controller-based Routing Scheme for Named-Data Networking", Technical Report, Electrical Engineering Program, COPPE/UFRJ, May 2016. 
English, A4 size, 14 p.
http://www.gta.ufrj.br/ftp/gta/TechReports/TBD16a.pdf

Doctor of Science Thesis
Author - João Vitor Torres
Title - "An Autonomous and Efficient Controller-based Routing Scheme for Networking Named-data Mobility"
Advisor -Otto Carlos Muniz Bandeira Duarte
COPPE/PEE/UFRJ - June 2016
English - A4 size - 79 p.
http://www.gta.ufrj.br/ftp/gta/TechReports/JoaoVitor16.pdf

Torres, J. V., Alvarenga, I. D., Pedroza, A. C. P. and Duarte, O. C. M. B - "Proposing, Specifying, and Validating a Controller-based Routing Protocol for a Clean-Slate Named-Data Networking", in 7th International Conference Network of the Future - NoF'2016, Buzios-RJ, Brazil, November 2016.
English, A4 size, 5 p.
http://www.gta.ufrj.br/ftp/gta/TechReports/TAPD16.pdf

Torres, J.V., and Duarte, O. C. M. B. - "CRoS-NDN: Controller-based Routing Strategy for Named Data Networking", Technical Report, Electrical Engineering Program, COPPE/UFRJ, April 2014. English, A4 size, 7 p.
http://www.gta.ufrj.br/ftp/gta/TechReports/ToDu14a.pdf

Torres, J. V., Ferraz, L. H. G., and Duarte, O. C. M. B. - "Controller-based Routing Scheme for Named Data Network", Technical Report, Electrical Engineering Program, COPPE/UFRJ, December 2012. 
English, A4 size, 6 p.
http://www.gta.ufrj.br/ftp/gta/TechReports/TFD12.pdf


INSTALATION

The code is based on ndnsim v1.0
http://ndnsim.net/1.0/getting-started.html

apt-get update
apt-get install libboost-all-dev
apt-get install git
mkdir ndnSIM
cd ndnSIM
git clone -b ndnSIM-v1 git://github.com/cawka/ns-3-dev-ndnSIM ns-3
git clone git://github.com/cawka/pybindgen.git pybindgen
git clone -b master-v1 git://github.com/named-data/ndnSIM.git ns-3/src/ndnSIM

cd ns-3
./waf configure -d optimized
./waf
./waf install
cd ..

apt-get install pkg-config python-pip r-base
pip install workerpool
git clone https://github.com/jvitor3/CRoS-NDN.git crosndn
cd crosndn
./recompile.sh
mkdir temp
mkdir results
mkdir localresults
Rscript RLibsInstall.r

SIMULATION

Use the python scripts to run simulatinons.