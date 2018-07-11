.PHONY: default

default: so-commons-library our-commonsRule parsi readline coordinadorRule planificadorRule instanciaRule esiRule

so-commons-library:
	cd ~; git clone https://github.com/sisoputnfrba/so-commons-library; cd so-commons-library; sudo make install

our-commonsRule:
	cd our-commons/Debug; make

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
	sudo rm -rf ~/so-commons-library
	sudo rm -rf ~/parsi
	cd our-commons/Debug; make clean
	cd coordinador/Debug; make clean
	cd planificador/Debug; make clean
	cd instancia/Debug; make clean
	cd esi/Debug; make clean
