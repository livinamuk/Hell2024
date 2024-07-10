#include "tinycsg.hpp"

#include <glm/gtc/matrix_access.hpp>

namespace csg {

static float signed_distance(const glm::vec3& point, const plane_t& plane) {
    return glm::dot(plane.normal, point) + plane.offset;
}

static bool is_bit_set(int x, int bit) {
    return (x >> bit) & 1;
}

// a 3-bit encoding representing a box corner
using box_corner_t = int;

static constexpr box_corner_t left_bottom_near  = 0b000;
static constexpr box_corner_t right_bottom_near = 0b001;
static constexpr box_corner_t left_top_near     = 0b010;
static constexpr box_corner_t right_top_near    = 0b011;
static constexpr box_corner_t left_bottom_far   = 0b100;
static constexpr box_corner_t right_bottom_far  = 0b101;
static constexpr box_corner_t left_top_far      = 0b110;
static constexpr box_corner_t right_top_far     = 0b111;

static glm::vec3 box_corner(box_t box, box_corner_t corner) {
    return glm::vec3(
        is_bit_set(corner, 0)? box.max.x: box.min.x,
        is_bit_set(corner, 1)? box.max.y: box.min.y,
        is_bit_set(corner, 2)? box.max.z: box.min.z
    );
}

// the box corner with the smallest signed distance to the plane
// (always the same corner with every box)
static box_corner_t min_box_corner(const plane_t& plane) {
    box_t box{glm::vec3(0,0,0), glm::vec3(1,1,1)};
    std::pair<float, box_corner_t> distance_corner_pairs[8];
    for (box_corner_t i=0; i<8; ++i)
        distance_corner_pairs[i] = {
            signed_distance(box_corner(box, i), plane),
            i
        };
    return std::min_element(
        begin(distance_corner_pairs),
        end(distance_corner_pairs)
    )->second;
}

struct frustum_t {
    // left right bottom top near far
    plane_t planes[6];
    box_corner_t min_box_corners[6];
};

static frustum_t make_frustum_from_matrix(
    const glm::mat4& view_projection)
{
    // https://gdbooks.gitbooks.io/legacyopengl/content/Chapter8/frustum.html
    // The plane values can be found by adding or subtracting one of the
    // first three rows of the viewProjection matrix from the fourth row.
    // Left Plane Row 0 (addition)
    // Right Plane Row 0 Negated (subtraction)
    // Bottom Plane Row 1 (addition)
    // Top Plane Row 1 Negated (subtraction)
    // Near Plane Row 2 (addition)
    // Far Plane Row 2 Negated (subtraction)
    glm::vec4 l = glm::row(view_projection, 3) + glm::row(view_projection, 0);
    glm::vec4 r = glm::row(view_projection, 3) - glm::row(view_projection, 0);
    glm::vec4 b = glm::row(view_projection, 3) + glm::row(view_projection, 1);
    glm::vec4 t = glm::row(view_projection, 3) - glm::row(view_projection, 1);
    glm::vec4 n = glm::row(view_projection, 3) + glm::row(view_projection, 2);
    glm::vec4 f = glm::row(view_projection, 3) - glm::row(view_projection, 2);
    // we need to negate because the above source has normals pointing *inside*
    // the frustum
    plane_t left  {-glm::vec3(l), -l.w};
    plane_t right {-glm::vec3(r), -r.w};
    plane_t bottom{-glm::vec3(b), -b.w};
    plane_t top   {-glm::vec3(t), -t.w};
    plane_t near  {-glm::vec3(n), -n.w};
    plane_t far   {-glm::vec3(f), -f.w};
    return frustum_t{
        { left, right, bottom, top, near, far },
        {
            min_box_corner(left),
            min_box_corner(right),
            min_box_corner(bottom),
            min_box_corner(top),
            min_box_corner(near),
            min_box_corner(far)
        }
    };
}

// inexact-- returns false positives, but good for frustum culling
bool frustum_intersects_box(const frustum_t& frustum, const box_t& box) {
    for (int i=0; i<6; ++i) {
        glm::vec3 min_corner = box_corner(box, frustum.min_box_corners[i]);
        if (signed_distance(min_corner, frustum.planes[i]) > 0.0f)
            return false;
    }
    return true;
   /*
    glm::vec3 center = (box.max + box.min) / 2.0f;

    for (int i=0; i<6; ++i) {
        if (signed_distance(center, frustum.planes[i]) > 0.0f)
            return false;
    }
    return true;
    */
}

std::vector<brush_t*> world_t::query_frustum(
    const glm::mat4& view_projection
)
{
    frustum_t frustum = make_frustum_from_matrix(view_projection);
    std::vector<brush_t*> result;
    brush_t *b = first();
    while (b) {
        if (frustum_intersects_box(frustum, b->box))
            result.push_back(b);
        b = next(b);
    }
    return result;
}

}