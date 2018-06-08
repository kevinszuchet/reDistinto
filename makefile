.PHONY: all

all: so-commons-library our-commonsRule parsi readline coordinadorRule planificadorRule instanciaRule esiRule

so-commons-library:
	cd ~; git clone https://github.com/sisoputnfrba/so-commons-library; cd so-commons-library; sudo make install

our-commonsRule:
	cd our-commons/Debug; make; chmod 777 makeOurCommonsUsable.sh
	. our-commons/Debug/makeOurCommonsUsable.sh

parsi:
	cd ~; git clone https://github.com/sisoputnfrba/parsi; cd parsi; sudo make install

readline:
	sudo apt-get install libreadline6 libreadline6-dev

coordinadorRule:
	cd coordinador/Debug; make

planificadorRule:
	cd planificador/Debug; make

instanciaRule:
	cd instancia/Debug; make

esiRule:
	cd esi/Debug; make

clean:
	cd ~/so-commons-library; sudo make clean
	cd our-commons/Debug; make clean
	cd ~/parsi; sudo make clean
	cd coordinador/Debug; make clean
	cd planificador/Debug; make clean
	cd instancia/Debug; make clean
	cd esi/Debug; make clean
