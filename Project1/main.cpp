#include<iostream>
#include<glad/glad.h>
#include<GLFW/glfw3.h>


// Vertex Shader source code
const char* vertexShaderSource = "#version 140\n"
"attribute vec3 aPos;\n"
"void main()\n"
"{\n"
" gl_Position = vec4(aPos, 1.0);\n"
"}";

const char* fragmentShaderSource = "#version 140\n"
"uniform vec4 ourColor;\n"
"void main()\n"
"{\n"
" gl_FragColor = ourColor;\n"
"}";



int main()
{
	// Initialize GLFW
	glfwInit();

	// Tell GLFW what version of OpenGL we are using 
	// In this case we are using OpenGL 3.1
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);


	// Create a GLFWwindow object of 800 by 800 pixels, naming it "YoutubeOpenGL"
	GLFWwindow* window = glfwCreateWindow(800, 800, "YoutubeOpenGL", NULL, NULL);
	// Error check if the window fails to create
	if (window == NULL)
	{
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return -1;
	}
	// Introduce the window into the current context
	glfwMakeContextCurrent(window);

	//Load GLAD so it configures OpenGL
	gladLoadGL();
	// Specify the viewport of OpenGL in the Window
	// In this case the viewport goes from x = 0, y = 0, to x = 800, y = 800
	glViewport(0, 0, 800, 800);



	// Create Vertex Shader Object and get its reference
	GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
	// Attach Vertex Shader source to the Vertex Shader Object
	glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
	// Compile the Vertex Shader into machine code
	glCompileShader(vertexShader);

	// Create Fragment Shader Object and get its reference
	GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	// Attach Fragment Shader source to the Fragment Shader Object
	glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
	// Compile the Vertex Shader into machine code
	glCompileShader(fragmentShader);

	// Create Shader Program Object and get its reference
	GLuint shaderProgram = glCreateProgram();
	// Attach the Vertex and Fragment Shaders to the Shader Program
	glAttachShader(shaderProgram, vertexShader);
	glAttachShader(shaderProgram, fragmentShader);
	// Wrap-up/Link all the shaders together into the Shader Program
	glLinkProgram(shaderProgram);

	GLuint colorLocation =
				glGetUniformLocation(shaderProgram, "ourColor");
	std::cout << "colorLocation = " << colorLocation << std::endl;
	
	// Delete the now useless Vertex and Fragment Shader objects
	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);

	GLfloat pointVertices[] =
	{
		-0.90f,  0.00f, 0.0f, // rojo
		-0.80f,  0.15f, 0.0f, // verde
		-0.70f,  0.00f, 0.0f  // azul
	};

	GLfloat triangleVertices[] =
	{
		-0.20f,  0.40f, 0.0f,  // arriba
		-0.35f,  0.00f, 0.0f,  // abajo izquierda
		-0.05f,  0.00f, 0.0f   // abajo derecha
	};

	// Vertices coordinates
	GLfloat vertices[] =
	{
			0.30f, -0.20f, 0.0f,
			
			0.80f, -0.20f, 0.0f,
			
			0.80f, 0.30f, 0.0f,
			
			0.30f, 0.30f, 0.0f
			
	};
	

	GLuint indices[] =
	{
		0, 1, 2, // lower left triangle 
		2, 3, 0, // lower right triangle

	};

	GLfloat gridVertices[] =
	{
		// Líneas verticales
		-1.0f, -1.0f, 0.0f,
		-1.0f,  1.0f, 0.0f,

		-0.5f, -1.0f, 0.0f,
		-0.5f,  1.0f, 0.0f,

		 0.0f, -1.0f, 0.0f,
		 0.0f,  1.0f, 0.0f,

		 0.5f, -1.0f, 0.0f,
		 0.5f,  1.0f, 0.0f,

		 1.0f, -1.0f, 0.0f,
		 1.0f,  1.0f, 0.0f,

		 // Líneas horizontales
		 -1.0f, -1.0f, 0.0f,
		  1.0f, -1.0f, 0.0f,

		 -1.0f, -0.5f, 0.0f,
		  1.0f, -0.5f, 0.0f,

		 -1.0f,  0.0f, 0.0f,
		  1.0f,  0.0f, 0.0f,

		 -1.0f,  0.5f, 0.0f,
		  1.0f,  0.5f, 0.0f,

		 -1.0f,  1.0f, 0.0f,
		  1.0f,  1.0f, 0.0f
	};


	// Create reference containers for the Vartex Array Object and the Vertex Buffer Object
	GLuint VAO, VBO, EBO;

	// Generate the VAO and VBO with only 1 object each
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	glGenBuffers(1, &EBO);

	// Make the VAO the current Vertex Array Object by binding it
	glBindVertexArray(VAO);

	// Bind the VBO specifying it's a GL_ARRAY_BUFFER
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	// Introduce the vertices into the VBO
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

	// Configure the Vertex Attribute so that OpenGL knows how to read the VBO
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
	// Enable the Vertex Attribute so that OpenGL knows to use it
	glEnableVertexAttribArray(0);

	// Bind both the VBO and VAO to 0 so that we don't accidentally modify the VAO and VBO we created
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);


	GLuint gridVAO, gridVBO;

	glGenVertexArrays(1, &gridVAO);
	glGenBuffers(1, &gridVBO);

	glBindVertexArray(gridVAO);

	glBindBuffer(GL_ARRAY_BUFFER, gridVBO);
	glBufferData(
		GL_ARRAY_BUFFER,
		sizeof(gridVertices),
		gridVertices,
		GL_STATIC_DRAW
	);

	glVertexAttribPointer(
		0,
		3,
		GL_FLOAT,
		GL_FALSE,
		3 * sizeof(float),
		(void*)0
	);

	glEnableVertexAttribArray(0);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	GLuint pointVAO, pointVBO;

	glGenVertexArrays(1, &pointVAO);
	glGenBuffers(1, &pointVBO);

	glBindVertexArray(pointVAO);

	glBindBuffer(GL_ARRAY_BUFFER, pointVBO);
	glBufferData(
		GL_ARRAY_BUFFER,
		sizeof(pointVertices),
		pointVertices,
		GL_STATIC_DRAW
	);

	glVertexAttribPointer(
		0,
		3,
		GL_FLOAT,
		GL_FALSE,
		3 * sizeof(float),
		(void*)0
	);

	glEnableVertexAttribArray(0);

	glBindVertexArray(0);

	GLuint triangleVAO, triangleVBO;

	glGenVertexArrays(1, &triangleVAO);
	glGenBuffers(1, &triangleVBO);

	glBindVertexArray(triangleVAO);

	glBindBuffer(GL_ARRAY_BUFFER, triangleVBO);
	glBufferData(
		GL_ARRAY_BUFFER,
		sizeof(triangleVertices),
		triangleVertices,
		GL_STATIC_DRAW
	);

	glVertexAttribPointer(
		0,
		3,
		GL_FLOAT,
		GL_FALSE,
		3 * sizeof(float),
		(void*)0
	);

	glEnableVertexAttribArray(0);

	glBindVertexArray(0);

	glPointSize(15.0f);


	// Main while loop
	while (!glfwWindowShouldClose(window))
	{
		
		// Specify the color of the background
		glClearColor(0.0f, 0.1f, 0.17f, 1.0f);
		// Clean the back buffer and assign the new color to it
		glClear(GL_COLOR_BUFFER_BIT);
		// Tell OpenGL which Shader Program we want to use
		glUseProgram(shaderProgram);
		
		//vertices

		glUniform4f(colorLocation, 1.0f, 0.0f, 0.0f, 1.0f);
		glBindVertexArray(pointVAO);
		glDrawArrays(GL_POINTS, 0, 1);
		glUniform4f(colorLocation, 0.0f, 1.0f, 0.0f, 1.0f);
		glDrawArrays(GL_POINTS, 1, 1);
		glUniform4f(colorLocation, 0.0f, 0.0f, 1.0f, 1.0f);
		glDrawArrays(GL_POINTS, 2, 1);

		//triangulo
		glUniform4f(colorLocation, 1.0f, 1.0f, 0.0f, 1.0f);

		glBindVertexArray(triangleVAO);

		glDrawArrays(GL_TRIANGLES, 0, 3);
		
		// Bind the VAO so OpenGL knows to use it
		glBindVertexArray(VAO);
		//primer triangulo
		glUniform4f(colorLocation, 1.0f, 0.0f, 0.0f, 1.0f);
		glDrawElements(GL_TRIANGLES, 3, GL_UNSIGNED_INT, 0);
		//segundo triangulo
		glUniform4f(colorLocation, 0.0f, 0.0f, 1.0f, 1.0f);
		glDrawElements(GL_TRIANGLES, 3, GL_UNSIGNED_INT, (void*)(3 * sizeof(GLuint)));

		//GRILLA
		glUseProgram(shaderProgram);
		glUniform4f(colorLocation, 0.0f, 1.0f, 0.0f, 1.0f);

		glBindVertexArray(gridVAO);

		glDrawArrays(GL_LINES, 0, 20);
		// Swap the back buffer with the front buffer
		glfwSwapBuffers(window);
		// Take care of all GLFW events
		glfwPollEvents();
	}



	// Delete all the objects we've created
	glDeleteVertexArrays(1, &VAO);
	glDeleteBuffers(1, &VBO);
	glDeleteBuffers(1, &EBO);
	glDeleteProgram(shaderProgram);
	// Delete window before ending the program
	glfwDestroyWindow(window);
	// Terminate GLFW before ending the program
	glfwTerminate();
	return 0;
}

