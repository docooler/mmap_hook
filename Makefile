hook:
	gcc -shared -fPIC -o libmmap_hook.so mmap_hook.c -ldl

install: hook
    cp libmmap_hook.so /usr/lib/aarch64-linux-gnu/

redirect:
    gcc -o my_redirect my_redirect.c
run:
    ./my_redirect devmem2 0xf0000