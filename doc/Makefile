PICS = powermand.jpeg powerman.jpeg

all : $(PICS)

%.jpeg : %.fig
	fig2dev -L jpeg  $< $@

clean :
	rm -f *~ *.o .#* parse_* *.jpeg
