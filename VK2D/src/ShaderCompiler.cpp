/// This file is C++ whereas the rest of Vulkan2D's source code is C. This is because
/// Slang only provides a C++ interface for compiling shaders, so we wrap the C++ code
/// in an extern C interface the rest of the renderer can use.
#ifndef DISABLE_SHADER_COMPILATION
#include <slang-com-ptr.h>
#endif
#include <string>
#include "VK2D/ShaderCompiler.h"
#include "VK2D/Validation.h"
#include "VK2D/Logger.h"

#ifndef DISABLE_SHADER_COMPILATION
using namespace slang;
static Slang::ComPtr<IGlobalSession> gGlobalSession;
#endif // DISABLE_SHADER_COMPILATION

void _vk2dInitShaderCompiler() {
#ifndef DISABLE_SHADER_COMPILATION
    // Create the global session
    SlangGlobalSessionDesc globalSessionDesc = {0};
    SlangResult result = createGlobalSession(&globalSessionDesc, gGlobalSession.writeRef());
    if (!SLANG_SUCCEEDED(result)) {
        vk2dRaise(VK2D_STATUS_VULKAN_ERROR, "Failed to initialize global Slang session, %s", slang_getLastInternalErrorMessage());
    }
#endif // DISABLE_SHADER_COMPILATION
}

void _vk2dQuitShaderCompiler() {
    // nothing yet
}

bool _vk2dShaderCompile(const char *shader, uint32_t shaderSize, VK2DCompiledShaders *compiledShaders) {
#ifndef DISABLE_SHADER_COMPILATION
    memset(compiledShaders, 0, sizeof(VK2DCompiledShaders));

    // Create the local session
    TargetDesc target = {
            .format = SLANG_SPIRV,
            .profile = gGlobalSession->findProfile("glsl_450"),
    };
    SessionDesc sessionDesc = {
            .targets = &target,
            .targetCount = 1,
    };
    Slang::ComPtr<ISession> session;
    SlangResult result = gGlobalSession->createSession(sessionDesc, session.writeRef());
    if (!SLANG_SUCCEEDED(result)) {
        vk2dRaise(VK2D_STATUS_VULKAN_ERROR, "Failed to initialize Slang session, %s", slang_getLastInternalErrorMessage());
    }

    // Load user shader module
    Slang::ComPtr<IBlob> diagnostics;
    Slang::ComPtr<ISlangBlob> blob(Slang::INIT_ATTACH, slang_createBlob(shader, shaderSize));
    Slang::ComPtr<IModule> module(session->loadModuleFromSource("user_shader", "user_shader", blob, diagnostics.writeRef()));
    if (diagnostics) {
        vk2dLogInfo("%s", (const char*) diagnostics->getBufferPointer());
    }
    if (module.get() == nullptr) {
        return false;
    }

    // Find frag entry point
    Slang::ComPtr<IEntryPoint> fragEntryPoint;
    result = module->findEntryPointByName("PixelShader", fragEntryPoint.writeRef());

    if (!SLANG_SUCCEEDED(result)) {
        vk2dLogInfo("Failed to get entrypoint for pixel shader, %s", slang_getLastInternalErrorMessage());
        return false;
    }


    IComponentType* components[] = { module, fragEntryPoint };
    Slang::ComPtr<IComponentType> program;
    result = session->createCompositeComponentType(components, 2, program.writeRef());

    if (!SLANG_SUCCEEDED(result)) {
        vk2dLogInfo("Failed to compose Slang components, %s", slang_getLastInternalErrorMessage());
        return false;
    }

    // Detect a user buffer automatically and make sure the inputs are structured properly
    auto programLayout = program->getLayout();
    auto globalVarLayout = programLayout->getGlobalParamsVarLayout();
    auto globalTypeLayout = globalVarLayout->getTypeLayout();
    size_t userDataSize = 0;
    bool matchingPushConstants = false;
    bool matchingSampler = false;
    bool matchingTextures = false;
    bool extraGarbage = false;

    int fieldCount = globalTypeLayout->getFieldCount();
    for (int i = 0; i < fieldCount; i++) {
        // Get information about this field
        auto fieldVarLayout = globalTypeLayout->getFieldByIndex(i);
        auto fieldTypeLayout = fieldVarLayout->getTypeLayout();
        int32_t descriptorBinding = fieldVarLayout->getBindingIndex();
        int32_t descriptorSet = fieldVarLayout->getBindingSpace();
        int32_t size = userDataSize = fieldTypeLayout->getElementTypeLayout()->getSize();
        Slang::ComPtr<IBlob> tempBlob;
        fieldTypeLayout->getType()->getFullName(tempBlob.writeRef());
        const char *typeFullName = static_cast<const char *>(tempBlob->getBufferPointer());

        // Check for extra user-provided data to the shader
        if (descriptorBinding == 3 && descriptorSet == 3) {
            if (strcmp("ConstantBuffer", fieldTypeLayout->getName()) != 0) {
                vk2dLogInfo("User-provided data to shaders must be a ConstantBuffer, not \"%s\".", fieldTypeLayout->getName());
                return false;
            }
            userDataSize = size;
            continue;
        }

        // Validate push constants range
        if (strcmp(fieldVarLayout->getName(), "push") == 0) {
            if (strcmp("ConstantBuffer<Push, DefaultPushConstantDataLayout>", typeFullName) != 0) {
                vk2dLogInfo("Push constant is of incorrect type \"%s\", it should be \"[[vk::push_constant]] Push push;\"", typeFullName);
                return false;
            }
            if (size != 112) {
                vk2dLogInfo("Push constant range is of incorrect size \"%i\", it should be 112 bytes. Please check https://paolomazzon.github.io/Vulkan2D/md_docs_2Shaders.html for more information.", size);
                return false;
            }
            matchingPushConstants = true;
            continue;
        }

        // Validate matching sampler input
        if (strcmp(fieldVarLayout->getName(), "sampler") == 0) {
            // "SamplerState" 1/1
            if (strcmp("SamplerState", typeFullName) != 0) {
                vk2dLogInfo("Sampler is of incorrect type \"%s\", it should be \"[[vk::binding(1,1)]] SamplerState sampler;\"", typeFullName);
                return false;
            }
            if (descriptorSet != 1 || descriptorBinding != 1) {
                vk2dLogInfo("Sampler is in the incorrect binding/set [%i/%i], it should be \"[[vk::binding(1,1)]] SamplerState sampler;\"", descriptorBinding, descriptorSet);
                return false;
            }

            matchingSampler = true;
            continue;
        }

        // Validate matching texture input
        if (strcmp(fieldVarLayout->getName(), "textures") == 0) {
            if (strcmp("Texture2D<vector<float,4>>[]", typeFullName) != 0) {
                vk2dLogInfo("Texture array is of incorrect type \"%s\", it should be \"[[vk::binding(2,2)]] Texture2D<float4> textures[];\"", typeFullName);
                return false;
            }
            if (descriptorSet != 2 || descriptorBinding != 2) {
                vk2dLogInfo("Texture array is in the incorrect binding/set [%i/%i]. It should be \"[[vk::binding(2,2)]] Texture2D<float4> textures[];\"", descriptorBinding, descriptorSet);
                return false;
            }

            matchingTextures = true;
            continue;
        }

        // Getting here means this field is invalid
        vk2dLogInfo("Invalid field \"%s\" of type \"%s\" found in shader. Please check https://paolomazzon.github.io/Vulkan2D/md_docs_2Shaders.html for more information.", fieldVarLayout->getName(), typeFullName);
        return false;
    }

    // Make sure everything was found
    if (!matchingTextures || !matchingSampler || !matchingPushConstants) {
        if (!matchingSampler)
            vk2dLogInfo("Sampler state missing in shader. Please check https://paolomazzon.github.io/Vulkan2D/md_docs_2Shaders.html for more information.");
        if (!matchingTextures)
            vk2dLogInfo("Texture array missing in shader. Please check https://paolomazzon.github.io/Vulkan2D/md_docs_2Shaders.html for more information.");
        if (!matchingPushConstants)
            vk2dLogInfo("Push constants missing in shader. Please check https://paolomazzon.github.io/Vulkan2D/md_docs_2Shaders.html for more information.");

        return false;
    }

    // Link the shader
    Slang::ComPtr<IComponentType> linkedProgram;
    Slang::ComPtr<ISlangBlob> diagnosticBlob;
    result = program->link(linkedProgram.writeRef(), diagnosticBlob.writeRef());

    if (!SLANG_SUCCEEDED(result)) {
        vk2dLogInfo("Failed to link Slang components, %s", slang_getLastInternalErrorMessage());
        return false;
    }

    // Get final spir-v
    Slang::ComPtr<IBlob> fragKernelBlob;
    result = linkedProgram->getEntryPointCode(
            0, // frag
            0,
            fragKernelBlob.writeRef(),
            diagnostics.writeRef());

    if (!SLANG_SUCCEEDED(result)) {
        vk2dLogInfo("Failed to get fragment SPIR-V, %s", (const char*) diagnostics->getBufferPointer());
        return false;
    }

    // Copy over the new spir-v
    compiledShaders->fragmentSpirvSize = fragKernelBlob->getBufferSize();
    compiledShaders->fragmentSpirv = static_cast<uint32_t*>(malloc(fragKernelBlob->getBufferSize()));
    memcpy(compiledShaders->fragmentSpirv, fragKernelBlob->getBufferPointer(), compiledShaders->fragmentSpirvSize);
    compiledShaders->userDataSize = userDataSize;

    return true;
#endif // DISABLE_SHADER_COMPILATION
    vk2dLogInfo("Shaders not enabled.");
    return false;
}