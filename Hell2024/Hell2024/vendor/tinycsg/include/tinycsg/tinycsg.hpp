#pragma once

#include <vector>
#include <set>
#include <functional>
#include <any>

#include <glm/glm.hpp>

namespace csg {

struct world_t;
struct face_t;
struct brush_t;
struct fragment_t;

struct plane_t {
    glm::vec3 normal;
    float     offset;
};

struct ray_t {
    glm::vec3 origin;
    glm::vec3 direction;
};

struct ray_hit_t {
    brush_t    *brush;
    face_t     *face;
    fragment_t *fragment;
    float      parameter;
    glm::vec3  position;
};

struct box_t {
    glm::vec3 min, max;
};

using volume_t = int;
using volume_operation_t = std::function<volume_t(volume_t)>;

volume_operation_t make_fill_operation(volume_t with);
volume_operation_t make_convert_operation(volume_t from, volume_t to);

struct vertex_t {
    face_t    *faces[3];
    glm::vec3 position;
};

struct triangle_t {
    int i,j,k;
};

struct fragment_t {
    face_t                  *face;
    std::vector<vertex_t>   vertices;
    volume_t                front_volume;
    volume_t                back_volume;
    brush_t                 *front_brush;
    brush_t                 *back_brush;

    int                     relation;
};

std::vector<triangle_t> triangulate(const fragment_t& fragment);

struct face_t {
    const plane_t           *plane;
    std::vector<vertex_t>   vertices;
    std::vector<fragment_t> fragments;
};

struct brush_t {
    void                        set_planes(const std::vector<plane_t>& planes);
    const std::vector<plane_t>& get_planes() const;
    void                        set_volume_operation(const volume_operation_t& volume_operation);
    const std::vector<face_t>&  get_faces() const;
    void                        set_time(int time);
    int                         get_time() const;
    int                         get_uid() const;
    box_t                       get_box() const;
    std::any                    userdata;

    brush_t() = default;
    ~brush_t() = default;
    brush_t(const brush_t& other) = default;
    brush_t& operator=(const brush_t& other) = default;
    brush_t(brush_t&& other) = default;
    brush_t& operator=(brush_t&& other) = default;
    brush_t               *next;
    brush_t               *prev;
    world_t               *world;
    std::vector<plane_t>  planes;
    std::vector<brush_t*> intersecting_brushes;
    volume_operation_t    volume_operation;
    std::vector<face_t>   faces;
    box_t                 box;
    int                   time;
    int                   uid;
};

struct world_t {
    world_t();
    ~world_t();
    brush_t                *first();
    brush_t                *next(brush_t *brush);
    void                   remove(brush_t *brush);
    brush_t                *add();
    std::set<brush_t*>     rebuild();
    void                   set_void_volume(volume_t void_volume);
    volume_t               get_void_volume() const;
    // todo: accelerate queries with bvh
    std::vector<brush_t*>  query_point(const glm::vec3& point);
    std::vector<brush_t*>  query_box(const box_t& box);
    std::vector<ray_hit_t> query_ray(const ray_t& ray);
    std::vector<brush_t*>  query_frustum(const glm::mat4& view_projection);
    std::any               userdata;

    world_t(const world_t& other) = delete;
    world_t& operator=(const world_t& other) = delete;
    // todo: implement move constructors
    world_t(world_t&& other) = delete;
    world_t& operator=(world_t&& other) = delete;
    brush_t            *sentinel;
    std::set<brush_t*> need_face_and_box_rebuild;
    std::set<brush_t*> need_fragment_rebuild;
    volume_t           void_volume;
    int                next_uid;
};

} // end namespace csg
