#define GLM_ENABLE_EXPERIMENTAL
#include "tinycsg.hpp"
#include <algorithm>
#include <glm/gtx/intersect.hpp>

namespace csg {

static float signed_distance(const glm::vec3& point, const plane_t& plane) {
    return glm::dot(plane.normal, point) + plane.offset;
}

static glm::vec3 projection_of_point_onto_plane(const glm::vec3& point,
                                                const plane_t& plane)
{
    return point - signed_distance(point, plane) * plane.normal;
}

static bool ray_intersects_box(const ray_t& ray,
                               const box_t& box,
                               const glm::vec3& one_over_ray_direction)
{
    // branchless slab method
    // https://tavianator.com/2015/ray_box_nan.html

    float t1 = (box.min[0] - ray.origin[0])*one_over_ray_direction[0];
    float t2 = (box.max[0] - ray.origin[0])*one_over_ray_direction[0];

    float tmin = glm::min(t1, t2);
    float tmax = glm::max(t1, t2);

    for (int i = 1; i < 3; ++i) {
        t1 = (box.min[i] - ray.origin[i])*one_over_ray_direction[i];
        t2 = (box.max[i] - ray.origin[i])*one_over_ray_direction[i];

        tmin = glm::max(tmin, glm::min(t1, t2));
        tmax = glm::min(tmax, glm::max(t1, t2));
    }

    return tmax > glm::max(tmin, 0.0f);
}

static bool ray_intersects_plane(const ray_t& ray,
                                 const plane_t& plane,
                                 float& t)
{
    return glm::intersectRayPlane(
        ray.origin,
        ray.direction,
        projection_of_point_onto_plane(glm::vec3(0,0,0), plane),
        plane.normal,
        t
    );
}

static bool point_inside_convex_polygon(const glm::vec3& point,
                                        const std::vector<vertex_t>& vertices)
{
    int n = vertices.size();
    if (n < 3)
        return false;

    glm::vec3 v0 = vertices[0].position;
    glm::vec3 v1 = vertices[1].position;
    glm::vec3 v2 = vertices[2].position;
    glm::vec3 normal = glm::cross(v1-v0, v2-v0);

    for (int i=0; i<n; ++i) {
        int j = (i+1) % n;

        glm::vec3 vi = vertices[i].position;
        glm::vec3 vj = vertices[j].position;

        // if (glm::dot(normal, glm::cross(vj-vi, point-vi)) < 0.0f)

        static constexpr float epsilon = 0.001f;
        if (glm::dot(normal, glm::cross(vj-vi, point-vi)) < -epsilon)
            return false;
    }

    return true;
}

std::vector<ray_hit_t> world_t::query_ray(const ray_t& ray) {
    glm::vec3 one_over_ray_direction = 1.0f / ray.direction;

    std::vector<ray_hit_t> result;
    brush_t *b = first();
    while (b) {
        if (ray_intersects_box(ray, b->box, one_over_ray_direction)) {
            for (size_t iplane=0; iplane<b->planes.size(); ++iplane) {
                float t;
                if (ray_intersects_plane(ray, b->planes[iplane], t)) {
                    glm::vec3 intersection = ray.origin + t * ray.direction;
                    face_t& face = b->faces[iplane];
                    if (point_inside_convex_polygon(intersection,
                                                    face.vertices)) {
                        for (fragment_t& fragment: face.fragments) {
                            if (point_inside_convex_polygon(intersection,
                                                            face.vertices)) {
                                ray_hit_t ray_hit;
                                ray_hit.brush = b;
                                ray_hit.face = &face;
                                ray_hit.fragment = &fragment;
                                ray_hit.parameter = t;
                                ray_hit.position = intersection;
                                result.push_back(ray_hit);
                            }
                        }
                    }
                }
            }
        }
        b = next(b);
    }

    std::sort(result.begin(), result.end(),
        [](const ray_hit_t& hit0, const ray_hit_t& hit1) {
            return hit0.parameter < hit1.parameter;
        }
    );
    return result;
}

}