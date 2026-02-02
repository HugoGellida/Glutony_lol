#pragma once


#include <glm/vec3.hpp>

namespace geometry {
    class Primitive {
        protected:
            glm::vec3 m_position;
        public:
            /// @brief used once before first draw
            virtual void load() = 0;
            /// @brief used after last draw
            virtual void unload() = 0;
            /// @brief draw the primitive
            virtual void draw() = 0;

            virtual ~Primitive(){};
    };
}