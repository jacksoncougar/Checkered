#pragma once

#include "ComponentId.h"
#include "ComponentBase.h"
#include <string>
namespace Component {

	struct Sound : public Component::ComponentBase {
		std::string name;
		float volume;
	
	};
}