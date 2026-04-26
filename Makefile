CC := clang
CFLAGS := -Wall -Wextra -pedantic -g -fsanitize=address
LFLAGS := -lX11 -lXrandr -lm -lGL

TPOBJS := build/glad.o build/stb_image.o build/stb_truetype.o

build/eden: $(TPOBJS) src/common.c src/eden.c src/ren.c src/buffer.c src/buffer_impl_array.c src/fontatlas.c
	$(CC) $(CFLAGS) -o $@ $^ $(LFLAGS)

build/glad.o: src/tp/glad.c
	$(CC) $(CFLAGS) -O3 -c -o $@ $^
build/stb_image.o: src/tp/stb_image.h
	$(CC) $(CFLAGS) -DSTB_IMAGE_IMPLEMENTATION -O3 -x c -c -o $@ $^
build/stb_truetype.o: src/tp/stb_truetype.h
	$(CC) $(CFLAGS) -DSTB_TRUETYPE_IMPLEMENTATION -O3 -x c -c -o $@ $^
