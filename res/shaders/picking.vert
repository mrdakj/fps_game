#version 460 core

layout (location = 0) in vec3 aPos;
layout (location = 4) in ivec4 BoneIDs;
layout (location = 5) in vec4 Weights;

uniform mat4 camMatrix;

uniform mat4 model;
uniform mat4 transformation;

const int MAX_BONES = 100;
uniform mat4 gBones[MAX_BONES];

void main()
{
    mat4 BoneTransform= mat4(0.0);
    BoneTransform = gBones[BoneIDs[0]] * Weights[0];
    BoneTransform += gBones[BoneIDs[1]] * Weights[1];
    BoneTransform += gBones[BoneIDs[2]] * Weights[2];
    BoneTransform += gBones[BoneIDs[3]] * Weights[3];
    if (BoneTransform == mat4(0.0))
    {
            BoneTransform = mat4(1.0);
    }

	gl_Position = camMatrix * transformation * model * BoneTransform * vec4(aPos, 1.0f);
}
