#include <vector>
#include "tinycsg.hpp"

namespace csg {

std::vector<triangle_t> triangulate(const fragment_t& fragment) {
    int vertex_count = fragment.vertices.size();
    std::vector<int> indices;
    std::vector<triangle_t> tris;
    indices.resize(vertex_count);
    for (int i=0; i<vertex_count; ++i)
        indices[i] = i;
    for (;;) {
        int n = indices.size();
        if (n < 3)
            break;
        if (n == 3) {
            tris.push_back(triangle_t{indices[0], indices[1], indices[2]});
            break;
        }
        tris.push_back(triangle_t{indices[0], indices[1], indices[2]});
        tris.push_back(triangle_t{indices[2], indices[3], indices[4 % n]});
        indices.erase(indices.begin() + 3);
        indices.erase(indices.begin() + 1);
    }
    return tris;
}

volume_operation_t make_fill_operation(volume_t with) {
    return [=](volume_t) { return with; };
}

volume_operation_t make_convert_operation(volume_t from, volume_t to) {
    return [=](volume_t old) {
        return (old == from)? to: old;
    };
}

void brush_t::set_planes(const std::vector<plane_t>& planes) {
    this->planes = planes;
    world->need_face_and_box_rebuild.insert(this);
    for (brush_t* intersecting: intersecting_brushes)
        world->need_fragment_rebuild.insert(intersecting);
}

const std::vector<plane_t>& brush_t::get_planes() const {
    return planes;
}

void brush_t::set_volume_operation(const volume_operation_t& volume_operation) {
    this->volume_operation = volume_operation;
    world->need_fragment_rebuild.insert(this);
    for (brush_t* intersecting: intersecting_brushes)
        world->need_fragment_rebuild.insert(intersecting);
}

const std::vector<face_t>& brush_t::get_faces() const {
    return faces;
}

void brush_t::set_time(int time) {
    this->time = time;
    world->need_fragment_rebuild.insert(this);
    for (brush_t* intersecting: intersecting_brushes)
        world->need_fragment_rebuild.insert(intersecting);
}

int brush_t::get_time() const {
    return time;
}

int brush_t::get_uid() const {
    return uid;
}

box_t brush_t::get_box() const {
    return box;
}

world_t::world_t() {
    sentinel = new brush_t;
    sentinel->next = sentinel;
    sentinel->prev = sentinel;

    void_volume = 0;
    next_uid = 0;
}

world_t::~world_t() {
    brush_t *b = first();
    while (b) {
        brush_t *n = next(b);
        delete b;
        b = n;
    }
    delete sentinel;
}

brush_t *world_t::first() {
    return (sentinel->next == sentinel)? nullptr: sentinel->next;
}

brush_t *world_t::next(brush_t *brush) {
    return (brush->next == sentinel)? nullptr: brush->next;
}

void world_t::remove(brush_t *brush) {
    brush_t *prev = brush->prev;
    brush_t *next = brush->next;
    prev->next = next;
    next->prev = prev;
    delete brush;
}

brush_t *world_t::add() {
    brush_t *brush = new brush_t;
    brush->world = this;
    brush->volume_operation = std::identity{};
    brush->box = box_t{ glm::vec3(1,1,1), glm::vec3(-1,-1,-1) };
    brush->uid = next_uid++;
    brush->time = 0;//brush->uid;

    brush_t *after = sentinel->prev;
    brush_t *next = after->next;
    after->next = brush;
    next->prev = brush;
    brush->next = next;
    brush->prev = after;

    return brush;
}

void world_t::set_void_volume(volume_t void_volume) {
    this->void_volume = void_volume;
    brush_t *b = first();
    while (b) {
        need_fragment_rebuild.insert(b);
        b = next(b);
    }
}

volume_t world_t::get_void_volume() const {
    return void_volume;
}


}