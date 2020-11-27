#version 450

layout (location = 0) out vec2 outUV;

out gl_PerVertex
{
    vec4 gl_Position;
};

void main()
{
//    xy(-1.000000,-1.000000)  uv(0.000000,0.000000)
//    xy(3.000000,-1.000000)  uv(2.000000,0.000000)
//    xy(-1.000000,3.000000)  uv(0.000000,2.000000)

//    switch (gl_VertexIndex) {
//        case 0:
//            outUV = vec2(0, 0);
//            gl_Position = vec4(-1, -1, 0, 1);
//            break;
//        case 1:
//            outUV = vec2(1, 0);
//            gl_Position = vec4(1, -1, 0, 1);
//        break;
//        case 2:
//            outUV = vec2(0, 1);
//            gl_Position = vec4(-1, 1, 0, 1);
//        break;
//
//        case 3:
//        outUV = vec2(0, 1);
//        gl_Position = vec4(-1, 1, 0, 1);
//        break;
//        case 4:
//        outUV = vec2(1, 0);
//        gl_Position = vec4(1, -1, 0, 1);
//        break;
//        case 5:
//        outUV = vec2(1, 1);
//        gl_Position = vec4(1, 1, 0, 1);
//        break;
//    }
    outUV = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);
    gl_Position = vec4(outUV * 2.0f - 1.0f, 0.0f, 1.0f);
}
