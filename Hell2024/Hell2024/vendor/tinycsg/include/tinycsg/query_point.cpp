#include "tinycsg.hpp"

namespace csg {

static bool box_intersects_box(const box_t& box, const box_t& other_box) {
    return glm::all(glm::lessThanEqual(box.min, other_box.max)) &&
           glm::all(glm::greaterThanEqual(box.max, other_box.min));
}

static bool box_contains_point(const box_t& box, const glm::vec3& point) {
    return box_intersects_box(box, box_t{point, point});
}

std::vector<brush_t*> world_t::query_point(const glm::vec3& point) {
    std::vector<brush_t*> result;
    brush_t *b = first();
    while (b) {
        if (box_contains_point(b->box, point))
            result.push_back(b);
        b = next(b);
    }
    return result;
}

}