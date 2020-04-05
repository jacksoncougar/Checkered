//
// Created by Jackson Cougar Wiebe on 4/3/2020.
//

#ifndef ENGINE_HEALTHBAR_HPP
#define ENGINE_HEALTHBAR_HPP

struct HealthBar : public ComponentBase {
  std::shared_ptr<Model> target; // display the health of this
  std::shared_ptr<Texture> health_sprite;
  std::shared_ptr<Billboard> billboard;
  Engine::Rectangle health_sprite_dimensions;

  HealthBar() {
    health_sprite =
        getEngine()->createComponent<Texture>("Assets/Textures/health.png");
    health_sprite_dimensions = {0, 0, 32, 32};
    billboard = getEngine()->createComponent<Billboard>(health_sprite);
  }

  void tick(float time) {
    auto health_percentage = (target->max_damage - target->current_damage) /
                             static_cast<float>(target->max_damage) * 100.0f;
    auto number_of_health_symbols =
        std::clamp<float>(health_percentage, 0, 100);

    health_sprite_dimensions.width = health_percentage * 3;

    auto [x, y, w, h] = health_sprite_dimensions;
    billboard->plot = BoxModel(w / 2, h / 2, w, h);
    billboard->dst = RelativeAnchor{-1, 1};
    billboard->src = {-1, 1};
  }
};
#include "Rendering/text.hpp"
#include "scenariointerface.hpp"
#include <Billboard.h>
#include <Engine.h>
#include <Pipeline/Library.h>
#include <Vehicle.h>
#include <WorldTransform.h>
#include <optional>
#include <scenery.hpp>
#endif // ENGINE_HEALTHBAR_HPP
