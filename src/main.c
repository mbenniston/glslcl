#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>
#include <unistd.h>
#include "glad.h"
#include <GLFW/glfw3.h>

void createContext();
char* readFile(const char* path);
int compileShader(const char* source, int type, char** log);
void destroyContext();
int getType(const char* filename);
bool isArgument(const char* arg);
int processFlag(const char* arg);
void processStdinInput();
int processFile(const char* filename);

//arguments / flags
#define ARG_QUIET "-q"
#define ARG_VERBOSE "-v"
#define ARG_ALL_VERTS "-vert"
#define ARG_ALL_FRAGS "-frag"
#define ARG_FILTER "-f"
#define ARG_HELP "-h"

static const char* USAGE_STR = ""
"Usage: \n\t"
"[flag] [filename] [filename] [flag] [flag] [filename] ... \n\n"
"Example: glslcl my_shader.vert -q\n\n"
"Flags:\n"
"\t-q:\tSets logging mode to quiet, only errors are shown (errors and info messages are shown by default)\n"
"\t-v:\tSets logging mode to verbose, all errors, info and debugging messages are shown\n"
"\t-vert:\tSets the shader type to vertex for all given files\n"
"\t-frag:\tSets the shader type to fragment for all given files\n"
"\t-f:\tIgnores files that don't end in .vert or .frag\n"
"\t-h:\tShows help description\n\n";

static const char* HELP_STR = ""
"glslcl is a simple command line tool to compile-test glsl shaders without a game engine or context\n"
"it either takes filenames as input through argv, or a raw shader source through stdin \n"
"filenames given to glslcl are checked if the end in either .vert or .frag to select the correct shader type\n"
"if a filename does not end in .vert or .frag and none of the the override or filter flags an error is thown\n"
"if the -vert flag is set it will compile all shaders as vertex shaders\n"
"if the -frag flag is set it will compile all shaders as fragment shaders\n"
"program exits with exit code 0 if no compile issues are found otherwises it exits with exit code 1 \n";

enum LogType {
    LT_QUIET = 0, LT_DEFAULT = 1, LT_VERBOSE = 2
};

enum LogType logType = LT_DEFAULT; 

//overrides any inferred shader types
int typeOverride = -1;

//doesnt process any files that do not fit the interred shader type filename pattern
bool filterFilenames = false;

//used to disable / enable statements depending on the current log type
#define LOG_DEBUG(x) if(logType == LT_VERBOSE) x;
#define LOG_INFO(x) if(logType > LT_QUIET) x;
#define LOG_ERROR(x) x;

int main(int argc, const char**  argv) {
    int numFilenames = 0;
    int retValue = 0;

    //process arguments first to set up program 
    if(argc > 1) {
        for(int i = 0; i < argc - 1; i++) {
            if(isArgument(argv[i+1])){
                processFlag(argv[i+1]);
            } else {
                numFilenames ++;
            }
        }
    }

    createContext();

    //iterate over filenames
    for(int i = 0; i < argc - 1; i++){
        //skip arguments
        if(isArgument(argv[i+1])){
            continue;
        }

        const char* filename = argv[i+1];

        //process file and handle error code
        int result = processFile(filename);
        if(result == 2) {
            retValue = 1;
        } else if(result == 1) {
            destroyContext();
            return 1;
        }
        
    }
    
    //check if the program is not in a terminal
    if(!isatty(STDIN_FILENO)) {
        processStdinInput();
    } else if(numFilenames < 1) {
        printf("%s", USAGE_STR);
        return 1;
    }

    destroyContext();

    return retValue;
}

static struct GLFWwindow* window; 

//create OpenGL context without a window 
void createContext() {
    glfwInit();

    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);

    window = glfwCreateWindow(320, 180, "", NULL, NULL);
    glfwMakeContextCurrent(window);
    gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
} 

//reads a file in as a string, returns NULL if the file is not found
char* readFile(const char* path) {
    FILE* file = fopen(path, "r");
    if(!file) {
        return NULL;
    }

    fseek(file, 0L, SEEK_END);
    int sz = ftell(file);
    rewind(file);

    char* source = malloc(sz + 1);
    fread(source, 1, sz, file);
    source[sz] = 0;

    assert(sz == strlen(source));
    
    return source;
} 

//compiles the shader and returns an error code along with an option log string
int compileShader(const char* source, int type, char** log) {
    *log = NULL;

    if(typeOverride != -1) type = typeOverride;
    LOG_DEBUG(printf("Compiling using type 0x%X\n", type));

    GLuint shader = glCreateShader(type);

    //make sure the string is null terminated
    glShaderSource(shader, 1, (const char * const*)&source, NULL);
    glCompileShader(shader);

    int result;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &result);
	if (result == GL_FALSE)
	{
        int length;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);
		*log = malloc(length);
		glGetShaderInfoLog(shader, length, NULL, *log);
		return 1;
	}
    glDeleteShader(shader);
    return 0;
} 

void destroyContext() {
    glfwTerminate();
} 

//tries to infer shader type from filename, returns -1 if no match and override is not set
int getType(const char* filename) {
    char* dotPostfix = strrchr(filename, '.');

    if(dotPostfix && strcmp(dotPostfix, ".vert") == 0) {
        return GL_VERTEX_SHADER;
    }

    if(dotPostfix && strcmp(dotPostfix, ".frag") == 0) {
        return GL_FRAGMENT_SHADER;
    }

    if(typeOverride != -1) {
        return typeOverride;
    }

    return -1;
}

//returns whether a argument passed to the program is an option or a filename
bool isArgument(const char* arg) 
{
    char* minusPostfix = strrchr(arg, '-');
    char* dotPostfix = strrchr(arg, '.');

    return minusPostfix && !dotPostfix;
}

//updates state depending on a given argument 
int processFlag(const char* arg)
{
    if(strcmp(arg, ARG_QUIET) == 0) {
        logType = LT_QUIET;
    }
    else if(strcmp(arg, ARG_VERBOSE) == 0) {
        logType = LT_VERBOSE;
    }
    else if(strcmp(arg, ARG_ALL_VERTS) == 0) {
        typeOverride = GL_VERTEX_SHADER;
    }
    else if(strcmp(arg, ARG_ALL_FRAGS) == 0) {
        typeOverride = GL_FRAGMENT_SHADER;
    }
    else if(strcmp(arg, ARG_FILTER) == 0) {
        filterFilenames = true;
    } else if(strcmp(arg, ARG_HELP) == 0) {
        printf("%s", USAGE_STR);
        printf("%s", HELP_STR);
        exit(1);
    } else {
        return 1;
    }

    return 0;
}

const int INITIAL_SIZE = 1024;

//reads input from stdin, dynamically allocating space for input
void processStdinInput()
{
    char* source = malloc(INITIAL_SIZE);
    int maxSize = INITIAL_SIZE;
    int length = 0;
    int c;

    while(!feof(stdin)){
        c = fgetc(stdin);
        if(c == EOF) break;

        if(length + 1 >= maxSize){
            source = realloc(source, maxSize + INITIAL_SIZE);
            maxSize += INITIAL_SIZE;
        }

        source[length++] = c;
    }

    if(length + 1 >= maxSize){
        source = realloc(source, maxSize + 1);
        maxSize += 1;
    }

    source[length++] = 0;

    if(length <= 1) {
        return;
    }

    LOG_DEBUG(printf("Allocated for %d bytes, actual size: %d \n", maxSize, length))

    LOG_DEBUG(printf("COMPILING: stdin\n"))
    int type = GL_VERTEX_SHADER;
    if(typeOverride != -1) type = typeOverride; 

    char* log;
    int ret = compileShader(source, type, &log);
    free(source);

    switch (ret)
    {
    case 0:
        LOG_INFO(printf("stdin,PASSED\n"))
        break;
    case 1:
        LOG_ERROR(printf("stdin,FAILED\n"))
        printf("\t%s", log);
        destroyContext();
        exit(1);
        break;

    default:
        break;
    }

    if(log != NULL) {
        free(log);
    }

    destroyContext();
    exit(0);
}

//opens a file, compiles it and displays any error messages
int processFile(const char* filename)
{
    LOG_DEBUG(printf("TESTING: %s\n", filename))
    int type = getType(filename);

    if(type == -1 && filterFilenames) {
        return 0;
    } else if(type == -1) {
        LOG_ERROR(printf("ERROR: Could not determine shader type for file: %s, please postfix with .vert for vertex shaders and .frag for fragment shaders\n", filename))
        return 1;
    }

    char* source = readFile(filename);
    if(!source) {
        LOG_ERROR(printf("ERROR: Could not open file %s\n", filename))
        return 1;
    }

    LOG_DEBUG(printf("COMPILING: %s\n", filename))
    char* log;
    int ret = compileShader(source, type, &log);
    free(source);

    switch (ret)
    {
    case 0:
        LOG_INFO(printf("%s,PASSED\n", filename))
        break;
    case 1:
        LOG_ERROR(printf("%s,FAILED\n", filename))
        printf("\t%s", log);
        if(log != NULL) {
            free(log);
        }
        return 2;
        break;
    default:
        break;
    }

    return 0;
}