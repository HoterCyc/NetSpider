all: spider
OBJ = spider.o http.o web.o 
CPPFLAGS = -O -Wall -lpthread
spider: $(OBJ)
	g++ -o spider $(OBJ) $(CPPFLAGS)
.cpp.o:
	g++ -c $<
.PHONY: clean cleanall
clean:
	-rm spider *.o
cleanall:
	-make clean && rm ./Pages/*.html
