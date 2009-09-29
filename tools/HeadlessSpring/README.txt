What is this?
-------------

It's a headless version of spring.

You can run it on headless servers that have neither X nor a graphics card.

How to use?
-----------

instead of launching 'spring' launch instead 'spring-headlessstubs'.

Since it has no interface, you need to specify a script file, eg:

./spring-headerlessstubs myscript.txt

You can create an appropriate script file using springlobby or equivalent, which will
create a file  'script.txt' for you in your spring game directory, eg in ~/.spring.

Once springlobby has created the script.txt, simply copy it to another name, so that running
springlobby again won't overwrite the old file, and pass in the name of that file on the 
spring-headlessstubs commmand-line.

Who wrote it?
-------------

Hugh Perkins, hughperkins@gmail.com

What's the license?
-------------------

GPLv2, as for the rest of Spring.

