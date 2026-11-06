default: all 

all: cpp_delay_channel cy_delay_channel

cpp_delay_channel:
	@echo "*** Creating example: $@"
	cd ./$@/ && $(MAKE)  
	mv ./$@/run_$@ .

cy_delay_channel:
	@echo "*** Creating example: $@"
	cd ./$@/ && python setup.py build_ext --inplace

clean:
	cd ./cpp_delay_channel/ && $(MAKE) clean
	rm run_cpp_delay_channel
	rm ./cy_delay_channel/*.so

.PHONY: default all cpp_delay_channel cy_delay_channel clean
