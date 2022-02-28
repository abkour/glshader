# glshader

A OpenGL shader wrapper used for generic graphics applications. If you are looking for a minimal shader wrapper that deals with error handling while 
providing RAII consider using the library.

## Dependencies

The only dependency is GLAD to load OpenGL function pointers into your program. You may have to modify the include directive at the top of the file 
to point to the correct installation path.

## Usage

Very simply, you only need to write the following code to write your shader:

    	Shader shaderProgram =
		{
			{ GL_VERTEX_SHADER, "mypath/myshader.glsl.vs" },
			{ GL_FRAGMENT_SHADER, "mypath/myshader.glsl.fs"} //,
            // potentially more shaders here
		};
		shaderProgram.bind();

The constructor automatically creates the list of shaders you specified by reading the files at the associated paths. It reports errors
by throwing exceptions that you need to catch. If you wish to use the shader simply call the bind method.