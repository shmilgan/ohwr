include ../../Makedefs

all:
			 cd wr_vic && ./build.sh && cd ..
			 cd wr_minic && ./build.sh && cd ..
			 cp wr_minic/*.ko wr_vic/*.ko bin
			 
clean:
			 cd wr_vic && ./build.sh clean && cd ..
			 cd wr_minic && ./build.sh clean && cd ..

deploy: all
				mkdir -p $(WR_INSTALL_ROOT)/lib
				mkdir -p $(WR_INSTALL_ROOT)/lib/modules
				cp bin/*.ko $(WR_INSTALL_ROOT)/lib/modules

run: 		all
				scp bin/*.ko root@$(T):/wr/lib/modules