
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

#include "model/modelimporter.h"
#include <string.h>


#define JC_TEST_IMPLEMENTATION
#include <jc_test/jc_test.h>

static dmModelImporter::Scene* LoadScene(const char* path, dmModelImporter::Options& options)
{
    uint32_t file_size = 0;
    void* mem = dmModelImporter::ReadFile(path, &file_size);
    if (!mem)
        return 0;

    const char* suffix = strrchr(path, '.') + 1;
    dmModelImporter::Scene* scene = dmModelImporter::LoadFromBuffer(&options, suffix, mem, file_size);

    free(mem);

    return scene;
}

TEST(ModelGLTF, Load)
{
    const char* path = "/Users/mawe/work/projects/users/mawe/ModelTestKenneyFbx/assets/models/kenney/characters2/Animations/idle_test.glb";
    uint32_t file_size = 0;
    void* mem = dmModelImporter::ReadFile(path, &file_size);

    const char* suffix = strrchr(path, '.') + 1;

    dmModelImporter::Options options;
    dmModelImporter::Scene* scene = dmModelImporter::LoadFromBuffer(&options, suffix, mem, file_size);

    ASSERT_NE((void*)0, scene);

    dmModelImporter::DebugScene(scene);

    dmModelImporter::DestroyScene(scene);

    free(mem);
}

static int TestStandalone(const char* path)
{
    dmModelImporter::Options options;
    dmModelImporter::Scene* scene = LoadScene(path, options);

    dmModelImporter::DebugScene(scene);

    dmModelImporter::DestroyScene(scene);
}

int main(int argc, char **argv)
{
    if (argc > 1 && strstr(argv[1], ".gltf") != 0)
    {
        return TestStandalone(argv[1]);
    }

    jc_test_init(&argc, argv);
    return jc_test_run_all();
}

