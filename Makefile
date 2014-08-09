default:
	gcc userspace.c macro.c parser.c -lusb-1.0 -o setup-cataclysm-mouse

debug:
	gcc -g userspace.c macro.c parser.c -lusb-1.0

clean:
	rm setup-cataclysm-mouse
