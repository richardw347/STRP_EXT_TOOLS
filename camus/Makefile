
.PHONY : all mcs mus clean test dist

all: mcs mus

mcs:
	$(MAKE) -C mcs

mus:
	$(MAKE) -C mus

clean:
	$(MAKE) -C mcs clean
	$(MAKE) -C mus clean
	rm -f camus-dist.tar.gz

test: mcs mus
	@tests/run_tests.sh

dist: clean
	tar czvhf camus-dist.tar.gz -C .. --exclude=.git* --exclude=camus*.tar.gz camus

