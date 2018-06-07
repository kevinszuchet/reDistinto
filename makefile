all: so-commons-library our-commons parsi readline coordinador planificador esi instancia

so-commons-library:
	cd ~
	git clone https://github.com/sisoputnfrba/so-commons-library
	cd so-commons-library
	make clean
	sudo make install

our-commons:
	cd ~
	cd workspace/tp-2018-1c-youKnowNothing/our-commons/Debug
	make clean
	make
	export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:~/workspace/tp-2018-1c-youKnowNothing/our-commons/Debug
  
parsi:
	cd ~
	git clone https://github.com/sisoputnfrba/parsi
	cd parsi
	sudo make install
  
readline:
	sudo apt-get install libreadline6 libreadline6-dev
  
coordinador:
	cd ~/workspace/tp-2018-1c-youKnowNothing/coordinador/Debug
	make clean
	make
  
planificador:
	cd ~/workspace/tp-2018-1c-youKnowNothing/planificador/Debug
	make clean
	make
  
esi:
	cd ~/workspace/tp-2018-1c-youKnowNothing/esi/Debug
	make clean
	make
  
instancia:
	cd ~/workspace/tp-2018-1c-youKnowNothing/instancia/Debug
	make clean
	make

clean:	
	rm -rf ../so-commons-librar
