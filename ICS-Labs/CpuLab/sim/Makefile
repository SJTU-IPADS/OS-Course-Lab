##################################################
# You shouldn't need to modify anything below here
##################################################

# Use this rule (make all) to build the Y86-64 tools.
all:
	(cd misc; make all)
	(cd pipe; make all)
	(cd seq; make all)
	(cd y86-code; make all)

clean:
	rm -f *~ core
	(cd misc; make clean)
	(cd pipe; make clean)
	(cd seq; make clean)
	(cd y86-code; make clean)
	(cd ptest; make clean)
