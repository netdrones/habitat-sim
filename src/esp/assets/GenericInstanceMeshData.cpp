// Copyright (c) Facebook, Inc. and its affiliates.
// This source code is licensed under the MIT license found in the
// LICENSE file in the root directory of this source tree.

#include "GenericInstanceMeshData.h"

#include <Corrade/Containers/Array.h>
#include <Corrade/Containers/ArrayView.h>
#include <Corrade/Containers/ArrayViewStl.h>
#include <Corrade/Utility/Algorithms.h>
#include <Corrade/Utility/FormatStl.h>
#include <Magnum/Image.h>
#include <Magnum/ImageView.h>
#include <Magnum/Math/Functions.h>
#include <Magnum/Math/FunctionsBatch.h>
#include <Magnum/Math/PackingBatch.h>
#include <Magnum/MeshTools/Interleave.h>
#include <Magnum/MeshTools/RemoveDuplicates.h>
#include <Magnum/PixelFormat.h>
#include <Magnum/Shaders/GenericGL.h>
#include <Magnum/Trade/AbstractImporter.h>

#include "esp/core/Esp.h"
#include "esp/geo/Geo.h"
#include "esp/io/Json.h"

namespace Cr = Corrade;
namespace Mn = Magnum;

namespace esp {
namespace assets {

namespace {

// TODO: this could instead use Mn::Trade::MeshData directly
struct InstancePlyData {
  std::vector<vec3f> cpu_vbo;
  std::vector<vec3uc> cpu_cbo;
  std::vector<uint32_t> cpu_ibo;
  std::vector<uint16_t> objectIds;
  /**
   * @brief Whether or not the objectIds were provided by the source .ply file.
   * If so then we assume they can be used to provide islands to split the
   * semantic mesh for better frustum culling.
   */
  bool objIdsFromPly = false;
};

Cr::Containers::Optional<InstancePlyData> parsePly(
    Mn::Trade::AbstractImporter& importer,
    const std::string& plyFile) {
  /* Open the file. On error the importer already prints a diagnostic message,
     so no need to do that here. The importer implicitly converts per-face
     attributes to per-vertex, so nothing extra needs to be done. */
  Cr::Containers::Optional<Mn::Trade::MeshData> meshData;
  if (!importer.openFile(plyFile) || !(meshData = importer.mesh(0)))
    return Cr::Containers::NullOpt;

  /* Copy attributes to the vectors. Positions and indices can be copied
     directly using the convenience APIs as we store them in the full type.
     The importer always provides an indexed mesh with positions, so no need
     for extra error checking. */
  InstancePlyData data;
  data.cpu_vbo.resize(meshData->vertexCount());
  data.cpu_ibo.resize(meshData->indexCount());
  meshData->positions3DInto(Cr::Containers::arrayCast<Mn::Vector3>(
      Cr::Containers::arrayView(data.cpu_vbo)));
  meshData->indicesInto(data.cpu_ibo);
  const std::string msgPrefix =
      Cr::Utility::formatString("PLY File {} :", plyFile);
  /* Assuming colors are 8-bit RGB to avoid expanding them to float and then
     packing back */
  ESP_CHECK(meshData->hasAttribute(Mn::Trade::MeshAttribute::Color),
            msgPrefix << "has no vertex colors defined, which are required.");

  data.cpu_cbo.resize(meshData->vertexCount());
  const Mn::VertexFormat colorFormat =
      meshData->attributeFormat(Mn::Trade::MeshAttribute::Color);
  Cr::Containers::StridedArrayView1D<const Magnum::Color3ub> meshColors;
  if (colorFormat == Mn::VertexFormat::Vector3ubNormalized) {
    meshColors =
        meshData->attribute<Mn::Color3ub>(Mn::Trade::MeshAttribute::Color);

  } else if (colorFormat == Mn::VertexFormat::Vector4ubNormalized) {
    // retrieve RGB view of RGBA color data and copy into data
    meshColors = Cr::Containers::arrayCast<const Mn::Color3ub>(
        meshData->attribute<Mn::Color4ub>(Mn::Trade::MeshAttribute::Color));

  } else {
    ESP_CHECK(false,
              msgPrefix << "Unexpected vertex color type " << colorFormat);
  }

  Cr::Utility::copy(meshColors, Cr::Containers::arrayCast<Mn::Color3ub>(
                                    Cr::Containers::arrayView(data.cpu_cbo)));

  /* Check we actually have object IDs before copying them, and that those are
     in a range we expect them to be */
  data.objectIds.resize(meshData->vertexCount());
  Cr::Containers::Array<Mn::UnsignedInt> objectIds;
  if (meshData->hasAttribute(Mn::Trade::MeshAttribute::ObjectId)) {
    objectIds = meshData->objectIdsAsArray();
    data.objIdsFromPly = true;
    const int maxVal = Mn::Math::max(objectIds);
    ESP_CHECK(
        maxVal <= 65535,
        Cr::Utility::formatString(
            "{}Object IDs can't be stored into 16 bits : Max ID Value : {}",
            msgPrefix, maxVal));
  } else {
    // convert color data array to int (idx) for object ID
    // removing duplicates returns array of unique ids for colors
    // These ids should not be used to split the mesh with our current mesh
    // process
    data.objIdsFromPly = false;
    auto out = Mn::MeshTools::removeDuplicatesInPlace(
        meshData->mutableAttribute(Mn::Trade::MeshAttribute::Color));
    objectIds = std::move(out.first);
  }
  Mn::Math::castInto(Cr::Containers::arrayCast<2, Mn::UnsignedInt>(
                         Cr::Containers::stridedArrayView(objectIds)),
                     Cr::Containers::arrayCast<2, Mn::UnsignedShort>(
                         Cr::Containers::stridedArrayView(data.objectIds)));

  // Generic Semantic PLY meshes have -Z gravity
  const quatf T_esp_scene =
      quatf::FromTwoVectors(-vec3f::UnitZ(), geo::ESP_GRAVITY);

  for (auto& xyz : data.cpu_vbo) {
    xyz = T_esp_scene * xyz;
  }
  return data;
}

}  // namespace

std::vector<std::unique_ptr<GenericInstanceMeshData>>
GenericInstanceMeshData::fromPLY(Mn::Trade::AbstractImporter& importer,
                                 const std::string& plyFile,
                                 const bool splitMesh) {
  Cr::Containers::Optional<InstancePlyData> parseResult =
      parsePly(importer, plyFile);
  if (!parseResult) {
    return {};
  }

  std::vector<GenericInstanceMeshData::uptr> splitMeshData;
  if (splitMesh && parseResult->objIdsFromPly) {
    const InstancePlyData& data = *parseResult;
    std::unordered_map<uint16_t, PerObjectIdMeshBuilder> objectIdToObjectData;
    for (size_t i = 0; i < data.cpu_ibo.size(); ++i) {
      const uint32_t globalIndex = data.cpu_ibo[i];
      const uint16_t objectId = data.objectIds[globalIndex];
      if (objectIdToObjectData.find(objectId) == objectIdToObjectData.end()) {
        auto instanceMesh = GenericInstanceMeshData::create_unique();
        objectIdToObjectData.emplace(
            objectId, PerObjectIdMeshBuilder{*instanceMesh, objectId});
        splitMeshData.emplace_back(std::move(instanceMesh));
      }
      objectIdToObjectData.at(objectId).addVertex(
          globalIndex, data.cpu_vbo[globalIndex], data.cpu_cbo[globalIndex]);
    }
    // Update collision mesh data for each mesh
    for (size_t i = 0; i < splitMeshData.size(); ++i) {
      splitMeshData[i]->updateCollisionMeshData();
    }
  } else {
    // ply should not be split - ids were synthesized
    auto meshData = GenericInstanceMeshData::create_unique();
    meshData->cpu_vbo_ = std::move(parseResult->cpu_vbo);
    meshData->cpu_cbo_ = std::move(parseResult->cpu_cbo);
    meshData->cpu_ibo_ = std::move(parseResult->cpu_ibo);
    meshData->objectIds_ = std::move(parseResult->objectIds);
    // Construct vertices for collsion meshData
    // Store indices, facd_ids in Magnum MeshData3D format such that
    // later they can be accessed.
    // Note that normal and texture data are not stored
    meshData->collisionMeshData_.primitive = Magnum::MeshPrimitive::Triangles;
    meshData->updateCollisionMeshData();
    splitMeshData.emplace_back(std::move(meshData));
  }
  return splitMeshData;

}  // GenericInstanceMeshData::fromPLY

void GenericInstanceMeshData::uploadBuffersToGPU(bool forceReload) {
  if (forceReload) {
    buffersOnGPU_ = false;
  }
  if (buffersOnGPU_) {
    return;
  }

  Mn::GL::Buffer vertices, indices;
  indices.setTargetHint(Mn::GL::Buffer::TargetHint::ElementArray);
  indices.setData(cpu_ibo_, Mn::GL::BufferUsage::StaticDraw);

  vertices.setData(
      Mn::MeshTools::interleave(cpu_vbo_, cpu_cbo_, 1, objectIds_, 2),
      Mn::GL::BufferUsage::StaticDraw);

  renderingBuffer_ =
      std::make_unique<GenericInstanceMeshData::RenderingBuffer>();
  renderingBuffer_->mesh.setPrimitive(Magnum::GL::MeshPrimitive::Triangles)
      .setCount(cpu_ibo_.size())
      .addVertexBuffer(
          std::move(vertices), 0, Mn::Shaders::GenericGL3D::Position{},
          Mn::Shaders::GenericGL3D::Color3{
              Mn::Shaders::GenericGL3D::Color3::DataType::UnsignedByte,
              Mn::Shaders::GenericGL3D::Color3::DataOption::Normalized},
          1,
          Mn::Shaders::GenericGL3D::ObjectId{
              Mn::Shaders::GenericGL3D::ObjectId::DataType::UnsignedShort},
          2)
      .setIndexBuffer(std::move(indices), 0,
                      Mn::GL::MeshIndexType::UnsignedInt);

  updateCollisionMeshData();

  buffersOnGPU_ = true;
}

Magnum::GL::Mesh* GenericInstanceMeshData::getMagnumGLMesh() {
  if (renderingBuffer_ == nullptr) {
    return nullptr;
  }
  return &(renderingBuffer_->mesh);
}

void GenericInstanceMeshData::updateCollisionMeshData() {
  collisionMeshData_.positions = Cr::Containers::arrayCast<Mn::Vector3>(
      Cr::Containers::arrayView(cpu_vbo_));
  collisionMeshData_.indices = Cr::Containers::arrayCast<Mn::UnsignedInt>(
      Cr::Containers::arrayView(cpu_ibo_));
}

void GenericInstanceMeshData::PerObjectIdMeshBuilder::addVertex(
    uint32_t vertexId,
    const vec3f& position,
    const vec3uc& color) {
  // if we haven't seen this vertex, add it to the local vertex/color buffer
  auto result = vertexIdToVertexIndex_.emplace(vertexId, data_.cpu_vbo_.size());
  if (result.second) {
    data_.cpu_vbo_.emplace_back(position);
    data_.cpu_cbo_.emplace_back(color);
    data_.objectIds_.emplace_back(objectId_);
  }

  // update index buffers with local index of vertex/color
  data_.cpu_ibo_.emplace_back(result.first->second);
}

}  // namespace assets
}  // namespace esp
