
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

#define CGLTF_IMPLEMENTATION
#include <cgltf/cgltf.h>

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <dmsdk/dlib/vmath.h>
#include <dmsdk/dlib/hash.h>
#include <dmsdk/dlib/hashtable.h>
#include <dmsdk/dlib/transform.h>

namespace dmModelImporter
{

static const char* getPrimitiveTypeStr(cgltf_primitive_type type)
{
    switch(type)
    {
    case cgltf_primitive_type_points: return "cgltf_primitive_type_points";
    case cgltf_primitive_type_lines: return "cgltf_primitive_type_lines";
    case cgltf_primitive_type_line_loop: return "cgltf_primitive_type_line_loop";
    case cgltf_primitive_type_line_strip: return "cgltf_primitive_type_line_strip";
    case cgltf_primitive_type_triangles: return "cgltf_primitive_type_triangles";
    case cgltf_primitive_type_triangle_strip: return "cgltf_primitive_type_triangle_strip";
    case cgltf_primitive_type_triangle_fan: return "cgltf_primitive_type_triangle_fan";
    default: return "unknown";
    }
}

static const char* GetAttributeTypeStr(cgltf_attribute_type type)
{
    switch(type)
    {
    case cgltf_attribute_type_invalid: return "cgltf_attribute_type_invalid";
    case cgltf_attribute_type_position: return "cgltf_attribute_type_position";
    case cgltf_attribute_type_normal: return "cgltf_attribute_type_normal";
    case cgltf_attribute_type_tangent: return "cgltf_attribute_type_tangent";
    case cgltf_attribute_type_texcoord: return "cgltf_attribute_type_texcoord";
    case cgltf_attribute_type_color: return "cgltf_attribute_type_color";
    case cgltf_attribute_type_joints: return "cgltf_attribute_type_joints";
    case cgltf_attribute_type_weights: return "cgltf_attribute_type_weights";
    default: return "unknown";
    }
}

static const char* GetTypeStr(cgltf_type type)
{
    switch(type)
    {
    case cgltf_type_invalid: return "cgltf_type_invalid";
    case cgltf_type_scalar: return "cgltf_type_scalar";
    case cgltf_type_vec2: return "cgltf_type_vec2";
    case cgltf_type_vec3: return "cgltf_type_vec3";
    case cgltf_type_vec4: return "cgltf_type_vec4";
    case cgltf_type_mat2: return "cgltf_type_mat2";
    case cgltf_type_mat3: return "cgltf_type_mat3";
    case cgltf_type_mat4: return "cgltf_type_mat4";
    default: return "unknown";
    }
}

static const char* GetResultStr(cgltf_result result)
{
    switch(result)
    {
    case cgltf_result_success: return "cgltf_result_success";
    case cgltf_result_data_too_short: return "cgltf_result_data_too_short";
    case cgltf_result_unknown_format: return "cgltf_result_unknown_format";
    case cgltf_result_invalid_json: return "cgltf_result_invalid_json";
    case cgltf_result_invalid_gltf: return "cgltf_result_invalid_gltf";
    case cgltf_result_invalid_options: return "cgltf_result_invalid_options";
    case cgltf_result_file_not_found: return "cgltf_result_file_not_found";
    case cgltf_result_io_error: return "cgltf_result_io_error";
    case cgltf_result_out_of_memory: return "cgltf_result_out_of_memory";
    case cgltf_result_legacy_gltf: return "cgltf_result_legacy_gltf";
    default: return "unknown";
    }
}

static const char* GetAnimationPathTypeStr(cgltf_animation_path_type type)
{
    switch(type)
    {
    case cgltf_animation_path_type_invalid: return "cgltf_animation_path_type_invalid";
    case cgltf_animation_path_type_translation: return "cgltf_animation_path_type_translation";
    case cgltf_animation_path_type_rotation: return "cgltf_animation_path_type_rotation";
    case cgltf_animation_path_type_scale: return "cgltf_animation_path_type_scale";
    case cgltf_animation_path_type_weights: return "cgltf_animation_path_type_weights";
    default: return "unknown";
    }
}

static const char* GetInterpolationTypeStr(cgltf_interpolation_type type)
{
 switch(type)
 {
 case cgltf_interpolation_type_linear: return "cgltf_interpolation_type_linear";
 case cgltf_interpolation_type_step: return "cgltf_interpolation_type_step";
 case cgltf_interpolation_type_cubic_spline: return "cgltf_interpolation_type_cubic_spline";
 default: return "unknown";
 }
}


static float* ReadAccessorFloat(cgltf_accessor* accessor, uint32_t desired_num_components)
{
    uint32_t num_components = (uint32_t)cgltf_num_components(accessor->type);

    if (desired_num_components == 0)
        desired_num_components = num_components;

    float* out = new float[accessor->count * num_components];
    float* writeptr = out;

    for (uint32_t i = 0; i < accessor->count; ++i)
    {
        bool result = cgltf_accessor_read_float(accessor, i, writeptr, num_components);

        if (!result)
        {
            printf("Couldn't read floats!\n");
            delete[] out;
            return 0;
        }

        writeptr += desired_num_components;
    }

    return out;
}

static uint32_t* ReadAccessorUint32(cgltf_accessor* accessor, uint32_t desired_num_components)
{
    uint32_t num_components = (uint32_t)cgltf_num_components(accessor->type);

    if (desired_num_components == 0)
        desired_num_components = num_components;

    uint32_t* out = new uint32_t[accessor->count * desired_num_components];
    uint32_t* writeptr = out;

    for (uint32_t i = 0; i < accessor->count; ++i)
    {
        bool result = cgltf_accessor_read_uint(accessor, i, writeptr, num_components);

        if (!result)
        {
            printf("couldnt read floats!\n");
            delete[] out;
            return 0;;
        }

        writeptr += desired_num_components;
    }

    return out;
}

static float* ReadAccessorMatrix4(cgltf_accessor* accessor, uint32_t index, float* out)
{
    uint32_t num_components = (uint32_t)cgltf_num_components(accessor->type);
    assert(num_components == 16);

    bool result = cgltf_accessor_read_float(accessor, index, out, 16);

    if (!result)
    {
        printf("Couldn't read floats!\n");
        return 0;
    }

    return out;
}



static void DestroyGltf(void* opaque_scene_data);


static uint32_t FindNodeIndex(cgltf_node* node, uint32_t nodes_count, cgltf_node* nodes)
{
    for (uint32_t i = 0; i < nodes_count; ++i)
    {
        if (&nodes[i] == node)
            return i;
    }
    assert(false && "Failed to find node in list of nodes");
    return 0xFFFFFFFF;
}

static Node* TranslateNode(cgltf_node* node, cgltf_data* gltf_data, Scene* scene)
{
    uint32_t index = FindNodeIndex(node, gltf_data->nodes_count, gltf_data->nodes);
    return &scene->m_Nodes[index];
}

static dmTransform::Transform ToTransform(const float* m)
{
    dmVMath::Matrix4 mat = dmVMath::Matrix4(dmVMath::Vector4(m[0], m[1], m[2], m[3]),
                                            dmVMath::Vector4(m[4], m[5], m[6], m[7]),
                                            dmVMath::Vector4(m[8], m[9], m[10], m[11]),
                                            dmVMath::Vector4(m[12], m[13], m[14], m[15]));
    return dmTransform::ToTransform(mat);
}

static Skin* FindSkin(Scene* scene, const char* name)
{
    for (uint32_t i = 0; i < scene->m_SkinsCount; ++i)
    {
        Skin* skin = &scene->m_Skins[i];
        if (strcmp(name, skin->m_Name) == 0)
            return skin;
    }
    return 0;
}

static void LoadNodes(Scene* scene, cgltf_data* gltf_data)
{
    scene->m_NodesCount = gltf_data->nodes_count;
    scene->m_Nodes = new Node[scene->m_NodesCount];
    memset(scene->m_Nodes, 0, sizeof(Node)*scene->m_NodesCount);

    for (size_t i = 0; i < gltf_data->nodes_count; ++i)
    {
        cgltf_node* gltf_node = &gltf_data->nodes[i];

        Node* node = &scene->m_Nodes[i];
        node->m_Name = strdup(gltf_node->name);

        //printf("node %s:  ", gltf_node->name);

        dmVMath::Vector3 scale = dmVMath::Vector3(1,1,1);
        dmVMath::Vector3 translation;
        dmVMath::Quat rotation;

        node->m_Skin = 0;
        if (gltf_node->skin)
        {
            node->m_Skin = FindSkin(scene, gltf_node->skin->name);
        }

        // if (gltf_node->weights)
        //     printf("  Weights!");

        //Transform(dmVMath::Vector3 translation, dmVMath::Quat rotation, dmVMath::Vector3 scale)

        if (gltf_node->has_translation)
            translation = dmVMath::Vector3(gltf_node->translation[0], gltf_node->translation[1], gltf_node->translation[2]);
        if (gltf_node->has_scale)
            scale = dmVMath::Vector3(gltf_node->scale[0], gltf_node->scale[1], gltf_node->scale[2]);
        if (gltf_node->has_rotation)
            rotation = dmVMath::Quat(gltf_node->rotation[0], gltf_node->rotation[1], gltf_node->rotation[2], gltf_node->rotation[3]);

        if (gltf_node->has_matrix)
        {
            node->m_Transform = ToTransform(gltf_node->matrix);
        }
        else {
            node->m_Transform = dmTransform::Transform(translation, rotation, scale);
        }

        //printf("\n");

        //printf("    Node: %20s  mesh: %s  skin: %s\n", gltf_node->name, gltf_node->mesh?gltf_node->mesh->name:"-", gltf_node->skin?gltf_node->skin->name:"-");
    }

    // find all the parents and all the children

    for (size_t i = 0; i < gltf_data->nodes_count; ++i)
    {
        cgltf_node* gltf_node = &gltf_data->nodes[i];
        Node* node = &scene->m_Nodes[i];

        node->m_Parent = gltf_node->parent ? TranslateNode(gltf_node->parent, gltf_data, scene) : 0;

        node->m_ChildrenCount = gltf_node->children_count;
        node->m_Children = new Node*[node->m_ChildrenCount];

        for (uint32_t c = 0; c < gltf_node->children_count; ++c)
            node->m_Children[c] = TranslateNode(gltf_node->children[c], gltf_data, scene);
    }

    // Find root nodes
    scene->m_RootNodesCount = 0;
    for (uint32_t i = 0; i < scene->m_NodesCount; ++i)
    {
        Node* node = &scene->m_Nodes[i];
        if (node->m_Parent == 0)
            scene->m_RootNodesCount++;
    }
    scene->m_RootNodes = new Node*[scene->m_RootNodesCount];

    scene->m_RootNodesCount = 0;
    for (uint32_t i = 0; i < scene->m_NodesCount; ++i)
    {
        Node* node = &scene->m_Nodes[i];
        if (node->m_Parent == 0)
        {
            scene->m_RootNodes[scene->m_RootNodesCount++] = node;
        }
    }
}

static void LoadPrimitives(Model* model, cgltf_mesh* gltf_mesh)
{
    model->m_MeshesCount = gltf_mesh->primitives_count;
    model->m_Meshes = new Mesh[model->m_MeshesCount];
    memset(model->m_Meshes, 0, sizeof(Mesh)*model->m_MeshesCount);

    for (size_t i = 0; i < gltf_mesh->primitives_count; ++i)
    {
        cgltf_primitive* prim = &gltf_mesh->primitives[i];
        Mesh* mesh = &model->m_Meshes[i];
        mesh->m_Name = strdup(gltf_mesh->name);

        mesh->m_Material = strdup(prim->material->name);
        mesh->m_VertexCount = 0;

        //printf("primitive_type: %s\n", getPrimitiveTypeStr(prim->type));

        for (uint32_t a = 0; a < prim->attributes_count; ++a)
        {
            cgltf_attribute* attribute = &prim->attributes[a];
            cgltf_accessor* accessor = attribute->data;
            //printf("  attributes: %s   index: %u   type: %s  count: %u\n", attribute->name, attribute->index, GetAttributeTypeStr(attribute->type), (uint32_t)accessor->count);

            mesh->m_VertexCount = accessor->count;

            uint32_t num_components = (uint32_t)cgltf_num_components(accessor->type);
            uint32_t desired_num_components = num_components;

            if (attribute->type == cgltf_attribute_type_tangent)
            {
                desired_num_components = 3; // for some reason it give 4 elements
            }

            float* fdata = 0;
            uint32_t* udata = 0;

            if (attribute->type == cgltf_attribute_type_joints)
            {
                udata = ReadAccessorUint32(accessor, desired_num_components);
            }
            else
            {
                fdata = ReadAccessorFloat(accessor, desired_num_components);
            }

            if (fdata || udata)
            {
                if (attribute->type == cgltf_attribute_type_position)
                    mesh->m_Positions = fdata;

                else if (attribute->type == cgltf_attribute_type_normal)
                    mesh->m_Normals = fdata;

                else if (attribute->type == cgltf_attribute_type_tangent)
                    mesh->m_Tangents = fdata;

                else if (attribute->type == cgltf_attribute_type_texcoord)
                {
                    if (attribute->index == 0)
                    {
                        mesh->m_TexCoord0 = fdata;
                        mesh->m_TexCoord0NumComponents = num_components;
                    }
                    else if (attribute->index == 1)
                    {
                        mesh->m_TexCoord1 = fdata;
                        mesh->m_TexCoord1NumComponents = num_components;
                    }
                }

                else if (attribute->type == cgltf_attribute_type_color)
                    mesh->m_Color = fdata;

                else if (attribute->type == cgltf_attribute_type_joints)
                    mesh->m_Bones = udata;

                else if (attribute->type == cgltf_attribute_type_weights)
                    mesh->m_Weights = fdata;
            }
        }
    }
}

static void LoadMeshes(Scene* scene, cgltf_data* gltf_data)
{
    scene->m_ModelsCount = gltf_data->meshes_count;
    scene->m_Models = new Model[scene->m_ModelsCount];
    memset(scene->m_Models, 0, sizeof(Model)*scene->m_ModelsCount);

    for (uint32_t i = 0; i < gltf_data->meshes_count; ++i)
    {
        cgltf_mesh* gltf_mesh = &gltf_data->meshes[i]; // our "Model"
        Model* model = &scene->m_Models[i];
        model->m_Name = strdup(gltf_mesh->name);

        LoadPrimitives(model, gltf_mesh);
    }
}

static void LoadSkins(Scene* scene, cgltf_data* gltf_data)
{
    scene->m_SkinsCount = gltf_data->skins_count;
    scene->m_Skins = new Skin[scene->m_SkinsCount];
    memset(scene->m_Skins, 0, sizeof(Skin)*scene->m_SkinsCount);

    for (uint32_t i = 0; i < gltf_data->skins_count; ++i)
    {
        cgltf_skin* gltf_skin = &gltf_data->skins[i];

        Skin* skin = &scene->m_Skins[i];
        skin->m_Name = strdup(gltf_skin->name);

        skin->m_BonesCount = gltf_skin->joints_count;
        skin->m_Bones = new Bone[skin->m_BonesCount];
        memset(skin->m_Bones, 0, sizeof(Bone)*skin->m_BonesCount);

        cgltf_accessor* accessor = gltf_skin->inverse_bind_matrices;
        for (uint32_t j = 0; j < gltf_skin->joints_count; ++j)
        {
            cgltf_node* gltf_joint = gltf_skin->joints[j];
            Bone* bone = &skin->m_Bones[j];
            bone->m_Name = strdup(gltf_joint->name);
            // Cannot translate the bones here, since they're not created yet
            // bone->m_Node = ...

            float matrix[16];
            if (ReadAccessorMatrix4(accessor, j, matrix))
            {
                bone->m_InvBindPose = ToTransform(matrix);
            } else
            {
                assert(false);
            }
        }
    }
}

static void LinkBonesWithNodes(Scene* scene, cgltf_data* gltf_data)
{
    for (uint32_t i = 0; i < gltf_data->skins_count; ++i)
    {
        cgltf_skin* gltf_skin = &gltf_data->skins[i];
        Skin* skin = &scene->m_Skins[i];

        for (uint32_t j = 0; j < gltf_skin->joints_count; ++j)
        {
            cgltf_node* gltf_joint = gltf_skin->joints[j];
            Bone* bone = &skin->m_Bones[j];
            bone->m_Node = TranslateNode(gltf_joint, gltf_data, scene);
        }
    }
}

static bool AreEqual(const float* a, const float* b, uint32_t num_components, float epsilon)
{
    for (uint32_t i = 0; i < num_components; ++i)
    {
        if (fabsf(a[i] - b[i]) > epsilon)
            return false;
    }
    return true;
}

static void LoadChannel(NodeAnimation* node_animation, cgltf_animation_channel* channel)
{
    cgltf_accessor* accessor_times = channel->sampler->input;
    cgltf_accessor* accessor = channel->sampler->output;

    uint32_t num_components = (uint32_t)cgltf_num_components(accessor->type);

    bool all_identical = true;

    KeyFrame* key_frames = new KeyFrame[accessor_times->count];
    for (uint32_t i = 0; i < accessor->count; ++i)
    {
        cgltf_accessor_read_float(accessor_times, i, &key_frames[i].m_Time, 1);
        cgltf_accessor_read_float(accessor, i, key_frames[i].m_Value, num_components);

        if (i > 0 && !AreEqual(key_frames[0].m_Value, key_frames[i].m_Value, num_components, 0.0001f))
        {
            all_identical = false;
        }
    }

    uint32_t key_count = accessor->count;
    if (all_identical)
    {
        key_count = 1;
    }

    if (channel->target_path == cgltf_animation_path_type_translation)
    {
        node_animation->m_TranslationKeys = key_frames;
        node_animation->m_TranslationKeysCount = key_count;
    }
    else if(channel->target_path == cgltf_animation_path_type_rotation)
    {
        node_animation->m_RotationKeys = key_frames;
        node_animation->m_RotationKeysCount = key_count;
    }
    else if(channel->target_path == cgltf_animation_path_type_scale)
    {
        node_animation->m_ScaleKeys = key_frames;
        node_animation->m_ScaleKeysCount = key_count;
    } else
    {
        // Unsupported type
        delete[] key_frames;
    }
}

static uint32_t CountAnimatedNodes(cgltf_animation* animation, dmHashTable64<uint32_t>& node_to_index)
{
    node_to_index.SetCapacity((32*2)/3, 32);

    for (size_t i = 0; i < animation->channels_count; ++i)
    {
        if (node_to_index.Full())
        {
            uint32_t new_capacity = node_to_index.Capacity() + 32;
            node_to_index.SetCapacity((new_capacity*2)/3, new_capacity);
        }

        const char* node_name = animation->channels[i].target_node->name;
        dmhash_t node_name_hash = dmHashString64(node_name);

        uint32_t* prev_index = node_to_index.Get(node_name_hash);
        if (prev_index == 0)
        {
            node_to_index.Put(node_name_hash, node_to_index.Size());
        }
    }

    return node_to_index.Size();
}

static void LoadAnimations(Scene* scene, cgltf_data* gltf_data)
{
    scene->m_AnimationsCount = gltf_data->animations_count;
    scene->m_Animations = new Animation[scene->m_AnimationsCount];

    // first, count number of animated nodes we have
    for (uint32_t i = 0; i < gltf_data->animations_count; ++i)
    {
        cgltf_animation* gltf_animation = &gltf_data->animations[i];
        Animation* animation = &scene->m_Animations[i];

        animation->m_Name = strdup(gltf_animation->name);

        dmHashTable64<uint32_t> node_name_to_index;
        animation->m_NodeAnimationsCount = CountAnimatedNodes(gltf_animation, node_name_to_index);
        animation->m_NodeAnimations = new NodeAnimation[animation->m_NodeAnimationsCount];
        memset(animation->m_NodeAnimations, 0, sizeof(NodeAnimation) * animation->m_NodeAnimationsCount);

        for (size_t i = 0; i < gltf_animation->channels_count; ++i)
        {
            cgltf_animation_channel* channel = &gltf_animation->channels[i];

            const char* node_name = channel->target_node->name;
            dmhash_t node_name_hash = dmHashString64(node_name);

            uint32_t* node_index = node_name_to_index.Get(node_name_hash);
            assert(node_index != 0);

            NodeAnimation* node_animation = &animation->m_NodeAnimations[*node_index];
            if (node_animation->m_Node == 0)
                node_animation->m_Node = TranslateNode(channel->target_node, gltf_data, scene);

            LoadChannel(node_animation, channel);
        }
    }
}


Scene* LoadGltfFromBuffer(Options* importeroptions, void* mem, uint32_t file_size)
{
    cgltf_options options;
    memset(&options, 0, sizeof(cgltf_options));

    cgltf_data* data = NULL;
    cgltf_result result = cgltf_parse(&options, (uint8_t*)mem, file_size, &data);

    if (result == cgltf_result_success)
        result = cgltf_load_buffers(&options, data, 0);

    if (result == cgltf_result_success)
        result = cgltf_validate(data);

    if (result != cgltf_result_success)
    {
        printf("Failed to load gltf file: %s (%d)\n", GetResultStr(result), result);
        return 0;
    }

    Scene* scene = new Scene;
    scene->m_OpaqueSceneData = (void*)data;
    scene->m_DestroyFn = DestroyGltf;

    LoadSkins(scene, data);
    LoadNodes(scene, data);
    LinkBonesWithNodes(scene, data);
    LoadMeshes(scene, data);
    LoadAnimations(scene, data);

    return scene;
}

static void DestroyGltf(void* opaque_scene_data)
{
    cgltf_free((cgltf_data*)opaque_scene_data);
}

}
