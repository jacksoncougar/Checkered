//
// Created by root on 11/1/20.
//

#include "RenderingSystem.h"

namespace Rendering {

void GeometryBatch::draw(Rendering::RenderingSystem &renderingSystem,
                         std::function<bool(const Mesh*)> filter) {

  for (auto [key, detail] : details) {

    if (!filter(key.first.get()))
      continue;

    if (key.second) {
      key.second->bind();
    }

    glDrawElementsInstancedBaseVertexBaseInstance(
        GL_TRIANGLES, detail[1].count, GL_UNSIGNED_INT, nullptr,
        detail[2].count, detail[1].offset, detail[2].offset / 64);

    glBindVertexArray(0);
  }
}

void GeometryBatch::bind(Rendering::RenderingSystem &renderingSystem) {

  Engine::log<module, Engine::low>("Binding shader#", shader);
  glBindVertexArray(vao);
}

GeometryBatch::GeometryBatch(
    std::shared_ptr<Rendering::BatchBuffer> arrayBuffer,
    std::shared_ptr<Rendering::BatchBuffer> elementBuffer,
    std::shared_ptr<Rendering::BatchBuffer> instanceBuffer)
    : arrayBuffer(arrayBuffer), elementBuffer(elementBuffer),
      instanceBuffer(instanceBuffer) {

  Engine::log<module>("Creating batch ", arrayBuffer->id(), " ",
                      elementBuffer->id(), " ", instanceBuffer->id());

  glGenVertexArrays(1, &vao);
  glBindVertexArray(vao);

  glBindBuffer(elementBuffer->type(), elementBuffer->id());

  glBindVertexBuffer(0, arrayBuffer->id(), 0, arrayBuffer->stride());

  glEnableVertexAttribArray(0);
  glVertexAttribFormat(0, 3, GL_FLOAT, false, 0);
  glVertexAttribBinding(0, 0);

  glEnableVertexAttribArray(1);
  glVertexAttribFormat(1, 3, GL_FLOAT, false, 1 * sizeof(glm::vec3));
  glVertexAttribBinding(1, 0);

  glEnableVertexAttribArray(2);
  glVertexAttribFormat(2, 3, GL_FLOAT, false, 2 * sizeof(glm::vec3));
  glVertexAttribBinding(2, 0);

  glBindVertexBuffer(1, instanceBuffer->id(), 0, instanceBuffer->stride());
  glVertexBindingDivisor(1, 1);

  // setup instance matrix attribute buffer

  glEnableVertexAttribArray(4);
  glEnableVertexAttribArray(5);
  glEnableVertexAttribArray(6);
  glEnableVertexAttribArray(7);

  glVertexAttribFormat(4, 4, GL_FLOAT, false, 0);
  glVertexAttribFormat(5, 4, GL_FLOAT, false, 1 * sizeof(glm::vec4));
  glVertexAttribFormat(6, 4, GL_FLOAT, false, 2 * sizeof(glm::vec4));
  glVertexAttribFormat(7, 4, GL_FLOAT, false, 3 * sizeof(glm::vec4));

  glVertexAttribBinding(4, 1);
  glVertexAttribBinding(5, 1);
  glVertexAttribBinding(6, 1);
  glVertexAttribBinding(7, 1);

  glBindVertexArray(0);
  Engine::log<module>("Creating new batch#", vao);
}

void GeometryBatch::push_back(std::shared_ptr<Mesh> mesh,
                              std::shared_ptr<Material> material) {

  const auto key = std::make_pair(mesh, material);

  details[key][ArrayBuffer] = arrayBuffer->push_back(mesh->vertices);
  details[key][ElementBuffer] = elementBuffer->push_back(mesh->indices);
  details[key][InstanceBuffer] =
      instanceBuffer->push_back(std::vector<glm::mat4>{glm::mat4(1)});
}

bool GeometryBatch::contains(std::shared_ptr<Mesh> mesh,
                             std::shared_ptr<Material> material) const {
  return details.count(std::make_pair(mesh, material)) > 0;
}

void GeometryBatch::remove(std::shared_ptr<Mesh> mesh,
                           std::shared_ptr<Material> material) {
  details.erase(std::make_pair(mesh, material));
}

BatchBuffer::BatchBuffer(int bufferMaxSize, int stride, int type)
    : m_size(bufferMaxSize), m_fill(0), m_stride(stride), m_type(type) {

  glGenBuffers(1, &m_id);
  glBindBuffer(m_type, m_id);
  glBufferData(m_type, m_size, nullptr, GL_DYNAMIC_DRAW);
}

GLuint BatchBuffer::id() { return m_id; }

GLuint BatchBuffer::type() { return m_type; }

size_t BatchBuffer::stride() { return m_stride; }

size_t BatchBuffer::count() { return m_fill / m_stride; }
} // namespace Rendering