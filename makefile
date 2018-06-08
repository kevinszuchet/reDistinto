.PHONY: all

all: so-commons-library our-commonsRule parsi readline coordinadorRule planificadorRule instanciaRule esiRule

so-commons-library:
	cd ~; git clone https://github.com/sisoputnfrba/so-commons-library; cd so-commons-library; sudo make clean; sudo make install

our-commonsRule:
	cd our-commons/Debug; make clean; make
	our-commons/Debug/makeOurCommonsUsable.sh

parsi:
	cd ~; git clone https://github.com/sisoputnfrba/parsi; cd parsi; sudo make clean; sudo make install

readline:
	sudo apt-get install libreadline6 libreadline6-dev

coordinadorRule:
	cd coordinador/Debug; make clean; make

planificadorRule:
	cd planificador/Debug; make clean; make

instanciaRule:
	cd instancia/Debug; make clean; make

esiRule:
	cd esi/Debug; make clean; make
