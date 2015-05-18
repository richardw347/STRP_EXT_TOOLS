INSTALL_DIR=/usr/local/bin

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


install:
	@echo "installing"
	cp camus/mus/camus_mus $(INSTALL_DIR)
	cp camus/mcs/camus_mcs $(INSTALL_DIR) 
	cp mtminer-dist/mtminer $(INSTALL_DIR) 
	cp shd31/shd $(INSTALL_DIR) 


uninstall:
	@echo "uninstalling"
	rm $(INSTALL_DIR)/camus_mus
	rm $(INSTALL_DIR)/camus_mcs
	rm $(INSTALL_DIR)/mtminer
	rm $(INSTALL_DIR)/shd
