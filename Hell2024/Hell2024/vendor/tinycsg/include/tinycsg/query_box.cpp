#include "tinycsg.hpp"

namespace csg {

static bool box_intersects_box(const box_t& box, const box_t& other_box) {
    return glm::all(glm::lessThanEqual(box.min, other_box.max)) &&
           glm::all(glm::greaterThanEqual(box.max, other_box.min));
}

std::vector<brush_t*> world_t::query_box(const box_t& box) {
    std::vector<brush_t*> result;
    brush_t *b = first();
    while (b) {
        if (box_intersects_box(b->box, box))
            result.push_back(b);
        b = next(b);
    }
    return result;
}

}