# Notes
This is notes that lists some of important design decision that I choose for this software.
This will be useful in case, I revisit this project again in the future.

## 1 BufferView and Buffer
Buffer and Buffer API is the core data structure of the entire editor. Buffer can be constructed
as empty buffer or by reading a file. Buffer contains not only the text data but also list of
lines, list of tokens, and undo related data like deleted text and operations stack (insertion 
and deletion). Buffer also have a cursor which BufferView needs to control in order to modify the 
buffer. BufferView is the modification state of a buffer. It basically have it's cursor, selection,
current line pointer, etc.

## 2. 

