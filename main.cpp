/* Copyright (c) 2019, NVIDIA CORPORATION. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *  * Neither the name of NVIDIA CORPORATION nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

//--------------------------------------------------------------------------------------------------
// This example is creating a scene with many similar objects and a plane. There are a few materials
// and a light direction.
// More details in simple.cpp
//

#include <array>
#include <chrono>
#include <iostream>


#include "nvh/inputparser.h"
#include "nvvkpp/context_vkpp.hpp"

#include "example.hpp"
#include "nvh/fileoperations.hpp"
#include "nvvk/extensions_vk.hpp"

int const SAMPLE_SIZE_WIDTH  = 800;
int const SAMPLE_SIZE_HEIGHT = 600;

// Default search path for shaders
std::vector<std::string> defaultSearchPaths{
    "./",
    "../",
    std::string(PROJECT_NAME),
    std::string("SPV_" PROJECT_NAME),
    PROJECT_ABSDIRECTORY,
    NVPSystem::exePath() + std::string(PROJECT_RELDIRECTORY),
};


//--------------------------------------------------------------------------------------------------
//
//
int main(int argc, char** argv)
{
  // Parsing the command line: mandatory '-f' for the filename of the scene
  InputParser parser(argc, argv);
  std::string filename = parser.getString("-f");
  if(parser.exist("-f"))
  {
    filename = parser.getString("-f");
  }
  else if(argc == 2 && nvh::endsWith(argv[1], ".gltf"))  // Drag&Drop
  {
    filename = argv[1];
  }
  else
  {
    filename = nvh::findFile("data/robot.gltf", defaultSearchPaths);
  }

  std::string hdrFilename = parser.getString("-e");
  if(hdrFilename.empty())
  {
    hdrFilename = nvh::findFile(R"(/data/daytime.hdr)", defaultSearchPaths);
  }


  // setup some basic things for the sample, logging file for example
  NVPSystem system(argv[0], PROJECT_NAME);

  // Enabling the extension
  vk::PhysicalDeviceDescriptorIndexingFeaturesEXT feature;

  nvvkpp::ContextCreateInfo contextInfo;
  contextInfo.addInstanceLayer("VK_LAYER_LUNARG_monitor", true);
  contextInfo.addInstanceExtension(VK_KHR_SURFACE_EXTENSION_NAME);
  contextInfo.addInstanceExtension(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
  contextInfo.addInstanceExtension(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);

  contextInfo.addDeviceExtension(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
  contextInfo.addDeviceExtension(VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME, false, &feature);
  contextInfo.addDeviceExtension(VK_KHR_MAINTENANCE3_EXTENSION_NAME);
  contextInfo.addDeviceExtension(VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME);
  contextInfo.addDeviceExtension(VK_KHR_BIND_MEMORY_2_EXTENSION_NAME);
  contextInfo.addDeviceExtension(VK_NV_RAY_TRACING_EXTENSION_NAME);


  // Creating the Vulkan instance and device
  nvvkpp::Context vkctx;
  vkctx.initInstance(contextInfo);

  // Find all compatible devices
  auto compatibleDevices = vkctx.getCompatibleDevices(contextInfo);
  assert(!compatibleDevices.empty());

  // Use first compatible device
  vkctx.initDevice(compatibleDevices[0], contextInfo);

  // Loading function pointers for Vulkan extensions
  load_VK_EXTENSION_SUBSET(vkctx.m_instance, vkGetInstanceProcAddr, vkctx.m_device, vkGetDeviceProcAddr);

  VkRtExample example;
  example.setScene(filename);
  example.setEnvironmentHdr(hdrFilename);

  // Creating the window
  example.open(0, 0, SAMPLE_SIZE_WIDTH, SAMPLE_SIZE_HEIGHT, PROJECT_NAME);

  // Window need to be opened to get the surface on which to draw
  vk::SurfaceKHR surface = example.getVkSurface(vkctx.m_instance);
  vkctx.setGCTQueueWithPresent(surface);

  LOGI("Using %s \n", vkctx.m_physicalDevice.getProperties().deviceName);

  example.setup(vkctx.m_device, vkctx.m_physicalDevice, vkctx.m_queueGCT.familyIndex);
  example.createSurface(surface, SAMPLE_SIZE_WIDTH, SAMPLE_SIZE_HEIGHT);
  example.createDepthBuffer();
  example.createRenderPass();
  example.createFrameBuffers();
  example.initExample();  // Now build the example
  example.initGUI(0);     // Using sub-pass 0


  // Window system loop
  while(example.pollEvents() && !example.isClosing())
  {
    if(example.isOpen())  // Not minimized
    {
      CameraManip.updateAnim();
      example.display();  // infinitely drawing
    }
  }
  example.destroy();
  vkctx.m_instance.destroySurfaceKHR(surface);
  vkctx.deinit();
}