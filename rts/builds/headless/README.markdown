# README

It is a headless version of spring.

You can run it on headless servers that have neither X nor a graphics card.


## How to use?

instead of launching `spring` launch `spring-headless` instead.

Since it has no interactive interface, you need to specify a script file, eg:

	./spring-headless /abs/path/to/my/script.txt

You can create an appropriate script file using SpringLobby or any other
spring lobby client. Usually, this will create a file `script.txt` for you
in your writable spring data directory, eg at `~/.spring/script.txt`.

Once you have a `script.txt`, simply copy it to another name, so that running
SpringLobby again will not overwrite the old file. Now pass in the absolute path
to that file on the `spring-headless` commmand-line.


## What is the license?

GPL v2 or later, as for the rest of Spring.

