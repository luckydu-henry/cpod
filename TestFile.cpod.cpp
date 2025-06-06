/* ***********************************
* This is a multiline comment!
* This is another multiline comment!
* And third line of comments!
* ************************************/

char   YourAge      = 102;
char   WhatsUp[4]   = {1,2,3,4};
float  q[4]         = {1,2,3,4};
char   MyAge        = 0x11;

struct BiggerStruct {
    char* shader_code = 
    "layout(location = 0) vec3 position;\n"
    "char* str = \" This is a string sequence! \"\n"
    "void main() {\n"
    "    gl_Position = vec4(position, 1.0);\n"
    "}"
    ;
    // I believe there are still other comments!
    int   a   = 10;
    int   b   = 20;
    struct vec4 {
        float x = 1.F;
        float y = 2.F;
        float z = 3.F;
    } p;
} q;