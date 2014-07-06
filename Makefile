default:
	gcc userspace.c macro.c parser.c -lusb-1.0

debug:
	gcc -g userspace.c macro.c parser.c -lusb-1.0
