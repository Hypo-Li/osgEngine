#version 430 core
layout (triangles, invocations = 32) in;
layout (triangle_strip, max_vertices = 3) out;

in vec2 uv[3];
out vec3 uvw;

void main()
{
    for (int i = 0; i < 3; ++i)
    {
        gl_Layer = gl_InvocationID;
        gl_Position = gl_in[i].gl_Position;
        uvw = vec3(uv[i], (gl_InvocationID + 0.5) / 32.0);
        EmitVertex();
    }
    EndPrimitive();
}