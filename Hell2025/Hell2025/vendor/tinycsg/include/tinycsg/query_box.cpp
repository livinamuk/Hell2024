#include "tinycsg.hpp"

namespace csg {

//static bool box_intersects_box(const box_t& box, const box_t& other_box) {
//    return glm::all(glm::lessThanEqual(box.min, other_box.max)) &&
//           glm::all(glm::greaterThanEqual(box.max, other_box.min));
//}

static bool box_intersects_box(const box_t& box, const box_t& other_box) {
    return (box.min.x <= other_box.max.x && box.max.x >= other_box.min.x) &&
        (box.min.y <= other_box.max.y && box.max.y >= other_box.min.y) &&
        (box.min.z <= other_box.max.z && box.max.z >= other_box.min.z);
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