//
// Created by root on 11/1/20.
//

#include "ComponentId.h"
#include "Index.h"
#include "ComponentBase.h"
#include "../Engine.h"

unsigned int Component::next_id = 0xFEED0000;

std::ostream &Component::operator<<(std::ostream &out, const Component::ClassId &id) {
    return out << (int)id;
}

Component::ComponentId::ComponentId(const Component::ComponentId &other) {
    id = other.id;
}

Component::ComponentId Component::ComponentId::Create() {
    return ComponentId(true);
}

bool Component::ComponentId::operator<(const Component::ComponentId &other) const {
    return this->id < other.id;
}

std::ostream &Component::operator<<(std::ostream &out, const Component::ComponentId &id) {

    if(Engine::hasName(id)) return out << Engine::names[id];
    else return out << id.id;
}

Component::ComponentId::ComponentId(bool) noexcept : id(Component::next_id++) {}

Component::ComponentId::ComponentId() : id(0xFFFFFFFFu) {        }

Component::ComponentId::ComponentId(bool, unsigned int id) noexcept: id(id) {}

std::shared_ptr<Component::ComponentInterface> Component::ComponentId::data() const {
    return Index::entityData<Component::ComponentInterface>(*this);
}