# glshader

A OpenGL shader wrapper used for generic graphics applications. If you are looking for a minimal shader wrapper that deals with error handling while 
providing RAII consider using the library.

## Dependencies

The only dependency is GLAD to load OpenGL function pointers into your program. You may have to modify the include directive at the top of the file 
to point to the correct installation path.