# shaderdirect

A OpenGL shader wrapper used for generic graphics applications. If you are looking for a minimal shader wrapper that deals with error handling while 
providing RAII consider using this library.

## Dependencies

The only dependency is GLAD to load OpenGL function pointers into your program. You may have to modify the include directive at the top of the file 
to point to the correct installation path.

## Usage

Very simply, you only need to write the following code to write your shader:
```c++
ShaderWrapper shaderProgram
(
	true,
	shader_pair(GL_VERTEX_SHADER, "mypath/myshader.glsl.vs"),
	shader_pair(GL_FRAGMENT_SHADER, "mypath/myshader.glsl.fs")
	// potentially more shaders here
);
shaderProgram.bind();
```

The first parameter `enableExtendedGLSL` is a boolean that tells the shader class to parse the shader source code for #include directives.

The second parameter is a parameter pack of type `std::pair<GLenum, std::string>` that specifies the shader type and their associated 
file locations. Make sure to use the `shader_pair` type alias to allow the compiler to deduce the correct type. Alternatively, you would 
have to use `std::pair<GLenum, std::string>` which is a little bit more cumbersome. 

Finally, the construction of the shader program has finished. If any errors occured, you will need to catch them. If no errors occured,
you can call the member function `bind(void)` to bind this shader program as the active shader program.

#### Moving shaders

You can move shaders by using the following methods:

```c++
auto shaderA(std::move(shaderB))
```
```c++
auto shaderA = std::move(shaderB)
```
```c++
ShaderWrapper shaderA;
shaderA = std::move(shaderA)
```

There is no way to construct a copy of a shader and I don't see a reason why one would want to do that.