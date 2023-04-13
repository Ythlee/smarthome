all:
	-make -C live/
	-make -C src/
	cp src/server_media ./

clean:
	-make clean -C live/
	-make clean -C src/
	rm server_media