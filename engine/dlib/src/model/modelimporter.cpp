
// Copyright 2020-2022 The Defold Foundation
// Copyright 2014-2020 King
// Copyright 2009-2014 Ragnar Svensson, Christian Murray
// Licensed under the Defold License version 1.0 (the "License"); you may not use
// this file except in compliance with the License.
//
// You may obtain a copy of the License, together with FAQs at
// https://www.defold.com/license
//
// Unless required by applicable law or agreed to in writing, software distributed
// under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
// CONDITIONS OF ANY KIND, either express or implied. See the License for the
// specific language governing permissions and limitations under the License.

#include "modelimporter.h"
#include <dmsdk/dlib/dstrings.h>
#include <stdio.h>
#include <stdlib.h>

namespace dmModelImporter
{

Options::Options()
{
}

static void DestroyMesh(Mesh* mesh)
{
    delete[] mesh->m_Positions;
    delete[] mesh->m_Normals;
    delete[] mesh->m_Tangents;
    delete[] mesh->m_Color;
    delete[] mesh->m_Weights;
    delete[] mesh->m_Bones;
    delete[] mesh->m_TexCoord0;
    delete[] mesh->m_TexCoord1;
    free((void*)mesh->m_Material);
}

static void DestroyModel(Model* model)
{
    free((void*)model->m_Name);
    for (uint32_t i = 0; i < model->m_MeshesCount; ++i)
        DestroyMesh(&model->m_Meshes[i]);
    delete[] model->m_Meshes;
}

static void DestroyNode(Node* node)
{
    free((void*)node->m_Name);
}

static void DestroyBone(Bone* bone)
{
    free((void*)bone->m_Name);
}

static void DestroySkin(Skin* skin)
{
    free((void*)skin->m_Name);
    for (uint32_t i = 0; i < skin->m_BonesCount; ++i)
        DestroyBone(&skin->m_Bones[i]);
    delete[] skin->m_Bones;
}

static void DestroyNodeAnimation(NodeAnimation* node_animation)
{
    delete[] node_animation->m_TranslationKeys;
    delete[] node_animation->m_RotationKeys;
    delete[] node_animation->m_ScaleKeys;
}

static void DestroyAnimation(Animation* animation)
{
    free((void*)animation->m_Name);
    for (uint32_t i = 0; i < animation->m_NodeAnimationsCount; ++i)
        DestroyNodeAnimation(&animation->m_NodeAnimations[i]);
    delete[] animation->m_NodeAnimations;
}

void DestroyScene(Scene* scene)
{
    scene->m_DestroyFn(scene->m_OpaqueSceneData);

    for (uint32_t i = 0; i < scene->m_NodesCount; ++i)
        DestroyNode(&scene->m_Nodes[i]);

    delete[] scene->m_Nodes;
    delete[] scene->m_RootNodes;

    for (uint32_t i = 0; i < scene->m_ModelsCount; ++i)
        DestroyModel(&scene->m_Models[i]);
    delete[] scene->m_Models;

    for (uint32_t i = 0; i < scene->m_SkinsCount; ++i)
        DestroySkin(&scene->m_Skins[i]);
    delete[] scene->m_Skins;

    for (uint32_t i = 0; i < scene->m_AnimationsCount; ++i)
        DestroyAnimation(&scene->m_Animations[i]);
    delete[] scene->m_Animations;
}

Scene* Load(Options* options, const char* suffix, void* data, uint32_t file_size)
{
    if (dmStrCaseCmp(suffix, "gltf") == 0 || dmStrCaseCmp(suffix, "glb") == 0)
        return LoadGltf(options, data, file_size);

    printf("ModelImpoerter: File type not supported: %s\n", suffix);
    return 0;
}

}
