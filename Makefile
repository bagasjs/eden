CC := clang
CFLAGS := -Wall -Wextra -pedantic -g -fsanitize=address
LFLAGS := -lX11 -lXrandr -lm -lGL

build/eden: src/tp/glad.c src/eden.c src/ren.c src/buffer.c src/buffer_impl_array.c src/fontatlas.c
	$(CC) $(CFLAGS) -o $@ $^ $(LFLAGS)
