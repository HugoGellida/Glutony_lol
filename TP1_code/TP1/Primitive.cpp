#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class Primitive {
    private:
    glm::vec3 position;
    public:
    void load();
    void unload();
    void draw();

    virtual ~Primitive() = default;
};