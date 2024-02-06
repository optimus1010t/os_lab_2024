
# delete the file a.out if it exists

run: main.c
	rm -f CSE
	gcc main.c -o CSE
	./CSE

runv: main.c
	rm -f CSE
	gcc -DVERBOSE main.c -o CSE
	./CSE

runs: main.c
	rm -f CSE
	gcc -DSLEEP main.c -o CSE
	./CSE

runvs: main.c
	rm -f CSE
	gcc -DVERBOSE -DSLEEP main.c -o CSE
	./CSE

save: ease.sh
	./ease.sh

clean: 
	rm -f CSE

