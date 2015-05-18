all:
	@echo "making external tools"
	cd camus && make
	cd mtminer-dist && make
	cd shd31 && make


clean:
	@echo "cleaning up tool folders"
	cd camus && make clean  
	cd mtminer-dist && make clean 
	cd shd31 && make clean
