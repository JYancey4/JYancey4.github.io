/*
 * Shader Loading Function Declaration
 * Author: Joshua Yancey
 * Contact: joshua.yancey@snhu.edu
 * Date: 02/01/2024
 * Version: 1.0
 *
 * Description:
 * This header file declares the LoadShaders function used for loading, compiling,
 * and linking vertex and fragment shaders for OpenGL.
 */

#ifndef SHADER_HPP
#define SHADER_HPP

GLuint LoadShaders(const char* vertex_file_path, const char* fragment_file_path);

#endif