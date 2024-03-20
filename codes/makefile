
# delete the file a.out if it exists

run: main.c
	rm -f main
	gcc main.c -o main
	./main

runv: main.c
	rm -f main
	gcc -DVERBOSE main.c -o main
	./main

runs: main.c
	rm -f main
	gcc -DSLEEP main.c -o main
	./main

runvs: main.c
	rm -f main
	gcc -DVERBOSE -DSLEEP main.c -o main
	./main

save: ease.sh
	./ease.sh

clean: 
	rm -f main

