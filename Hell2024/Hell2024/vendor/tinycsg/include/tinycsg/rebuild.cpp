#include "tinycsg.hpp"

#include <map>
#include <array>
#include <algorithm>

#include <math.h>
#include <assert.h>

#include <glm/gtc/matrix_access.hpp>

namespace csg {

    enum relation_t {
        RELATION_FRONT,
        RELATION_OUTSIDE = RELATION_FRONT,
        RELATION_BACK,
        RELATION_INSIDE = RELATION_BACK,
        RELATION_ALIGNED,
        RELATION_REVERSE_ALIGNED,
        RELATION_SPLIT
    };

    struct edge_t {
        face_t* faces[2];
    };

    static bool approx_equal(float a, float b) {
        // compare to 3 decimal places
        return int(round(a * 1000)) == int(round(b * 1000));
    }

    static float signed_distance(const glm::vec3& point, const plane_t& plane) {
        return glm::dot(plane.normal, point) + plane.offset;
    }

    static box_t extended(const box_t& box, const glm::vec3& point) {
        return box_t{
            glm::min(box.min, point),
            glm::max(box.max, point)
        };
    }

    // does brush0 come before brush1 in the csg order?
    // compare by time and use uid (global incrementing counter) as tie-breaker
    static bool b0_before_b1(brush_t* b0, brush_t* b1) {
        if (b0->time == b1->time)
            return b0->uid < b1->uid;
        return b0->time < b1->time;
    }

    static relation_t test(vertex_t* vertex, face_t* face) {
        float d = signed_distance(vertex->position, *face->plane);
        if (approx_equal(d, 0.0f))
            return RELATION_ALIGNED;
        else if (d > 0)
            return RELATION_FRONT;
        else
            return RELATION_BACK;
    }

    static relation_t test(vertex_t* vertex, brush_t* brush) {
        int n = brush->faces.size();
        relation_t rel = RELATION_INSIDE;
        for (int i = 0; i < n; ++i) {
            switch (test(vertex, &brush->faces[i])) {
            case RELATION_FRONT:
                return RELATION_OUTSIDE;
            case RELATION_ALIGNED:
                rel = RELATION_ALIGNED;
            default:
                ;
            }
        }
        return rel;
    }

    static relation_t test(fragment_t* fragment, face_t* face) {
        std::map<relation_t, int> count;
        count[RELATION_INSIDE] = 0;
        count[RELATION_ALIGNED] = 0;
        count[RELATION_OUTSIDE] = 0;
        for (vertex_t& v : fragment->vertices)
            count[test(&v, face)] += 1;
        if (count[RELATION_OUTSIDE] > 0 &&
            count[RELATION_INSIDE] > 0)
            return RELATION_SPLIT;
        else if (count[RELATION_OUTSIDE] == 0 &&
            count[RELATION_INSIDE] == 0) {
            float d = glm::dot(face->plane->normal, fragment->face->plane->normal);
            if (d < 0)
                return RELATION_REVERSE_ALIGNED;
            else
                return RELATION_ALIGNED;
        }
        else if (count[RELATION_INSIDE] > 0)
            return RELATION_INSIDE;
        else
            return RELATION_OUTSIDE;
    }

    static void recalculate_intersecting_brushes(brush_t* brush) {
        brush->intersecting_brushes = brush->world->query_box(brush->box);
        std::sort(
            brush->intersecting_brushes.begin(),
            brush->intersecting_brushes.end(),
            b0_before_b1
        );
        auto it = std::find(
            brush->intersecting_brushes.begin(),
            brush->intersecting_brushes.end(),
            brush
        );
        if (it != brush->intersecting_brushes.end())
            brush->intersecting_brushes.erase(it);
    }

    static bool try_get_edge(const vertex_t* vertex0, const vertex_t* vertex1, edge_t* edge) {
        // an edge exists if the two vertices share two faces
        std::array<face_t*, 3> faces;
        auto it = std::set_intersection(
            std::begin(vertex0->faces), std::end(vertex0->faces),
            std::begin(vertex1->faces), std::end(vertex1->faces),
            faces.begin()
        );
        if (it == faces.begin() + 2) {
            if (edge) {
                *edge = edge_t{ {faces[0], faces[1]} };
            }
            return true;
        }
        else {
            return false;
        }
    }

    static bool try_make_vertex(face_t* f0, face_t* f1, face_t* f2, vertex_t& v) {
        // intersect three planes, use cramer's rule
        std::array<face_t*, 3> faces = { f0, f1, f2 };
        std::sort(faces.begin(), faces.end());
        f0 = faces[0];
        f1 = faces[1];
        f2 = faces[2];

        // for (int i=0; i<3; ++i)
        //     printf("plane %d: %f %f %f %f\n",
        //         i,
        //         faces[i]->plane->normal.x,
        //         faces[i]->plane->normal.y,
        //         faces[i]->plane->normal.z,
        //         faces[i]->plane->offset);

        glm::mat3 m;
        m = glm::row(m, 0, f0->plane->normal);
        m = glm::row(m, 1, f1->plane->normal);
        m = glm::row(m, 2, f2->plane->normal);

        // printf("m = %s\n", glm::to_string(m).c_str());
        // fflush(stdout);

        float D = glm::determinant(m);
        // printf("D = %f\n", D); fflush(stdout);

        if (approx_equal(D, 0.0f))
            return false;
        glm::mat3 mx(m), my(m), mz(m);
        glm::vec3 offsets(f0->plane->offset, f1->plane->offset, f2->plane->offset);
        // printf("offsets = %s\n", glm::to_string(offsets).c_str());
        // fflush(stdout);

        mx = glm::column(mx, 0, -offsets);
        my = glm::column(my, 1, -offsets);
        mz = glm::column(mz, 2, -offsets);

        // printf("mx = %s\n", glm::to_string(mx).c_str());
        // fflush(stdout);
        // printf("my = %s\n", glm::to_string(my).c_str());
        // fflush(stdout);
        // printf("mz = %s\n", glm::to_string(mz).c_str());
        // fflush(stdout);

        v.faces[0] = f0;
        v.faces[1] = f1;
        v.faces[2] = f2;
        v.position = glm::vec3(
            glm::determinant(mx) / D,
            glm::determinant(my) / D,
            glm::determinant(mz) / D
        );
        return true;
    }

    static void order_vertices(face_t* face) {
        // use the info we have on the vertices (the planes that meet in the vertex)
        // to order the vertices properly without any floating point calculations
        std::vector<vertex_t> unsorted = move(face->vertices);
        face->vertices.clear();

        auto curr = unsorted.begin();
        while (curr != unsorted.end()) {
            vertex_t v = *curr;
            face->vertices.push_back(v);
            unsorted.erase(curr);
            curr = std::find_if(unsorted.begin(), unsorted.end(), [=](const vertex_t& other) {
                // does an edge exist between v and other?
                return try_get_edge(&v, &other, NULL);
            });
        }
    }

    static void fix_winding(face_t* face) {
        // just reverse the order of vertices if the polygon normal doesn't
        // match the plane normal
        if (face->vertices.size() < 3)
            return;
        glm::vec3 v0 = face->vertices[0].position;
        glm::vec3 v1 = face->vertices[1].position;
        glm::vec3 v2 = face->vertices[2].position;

        float d = dot(cross(v1 - v0, v2 - v0), face->plane->normal);
        if (d < 0) {
            reverse(face->vertices.begin(), face->vertices.end());
        }
    }

    static void rebuild_faces_and_box(brush_t* brush) {
        // printf("rebuild_faces_and_box\n"); fflush(stdout);

        brush->faces.clear();

        int n = brush->planes.size();
        brush->faces.resize(n);
        for (int i = 0; i < n; ++i) {
            brush->faces[i].plane = &brush->planes[i];

            // printf("plane %d: %f %f %f %f\n",
            //     i,
            //     brush->faces[i].plane->normal.x,
            //     brush->faces[i].plane->normal.y,
            //     brush->faces[i].plane->normal.z,
            //     brush->faces[i].plane->offset);

        }

        bool box_initialized = false;

        // build new vertices by intersecting each combination of 3 planes
        for (int i = 0; i < n - 2; ++i)
            for (int j = i + 1; j < n - 1; ++j)
                for (int k = j + 1; k < n; ++k) {
                    face_t* facei = &brush->faces[i];
                    face_t* facej = &brush->faces[j];
                    face_t* facek = &brush->faces[k];
                    vertex_t v;
                    if (try_make_vertex(facei, facej, facek, v) &&
                        test(&v, brush) != RELATION_OUTSIDE)
                    {
                        facei->vertices.push_back(v);
                        facej->vertices.push_back(v);
                        facek->vertices.push_back(v);
                        if (!box_initialized) {
                            brush->box = box_t{ v.position, v.position };
                            box_initialized = true;
                        }
                        else {
                            brush->box = extended(brush->box, v.position);
                        }
                    }
                }

        // order the vertices correctly
        for (auto& face : brush->faces) {
            order_vertices(&face);
            fix_winding(&face);
        }
    }

    static void split(fragment_t* fragment, face_t* splitter, fragment_t* front, fragment_t* back) {
        // splits fragment into front and back piece w.r.t. face
        // call only if test(fragment, face) == RELATION_SPLIT

        std::map<relation_t, fragment_t*> pieces;
        pieces[RELATION_FRONT] = front;
        pieces[RELATION_BACK] = back;
        for (auto& kv : pieces) {
            fragment_t* piece = kv.second;
            piece->face = fragment->face;
            piece->back_volume = fragment->back_volume;
            piece->front_volume = fragment->front_volume;
            piece->back_brush = fragment->back_brush;
            piece->front_brush = fragment->front_brush;
            piece->vertices.clear();
        }

        int vertex_count = fragment->vertices.size();
        for (int i = 0; i < vertex_count; ++i) {
            size_t j = (i + 1) % vertex_count;
            vertex_t v0 = fragment->vertices[i];
            vertex_t v1 = fragment->vertices[j];
            relation_t c0 = test(&v0, splitter);
            relation_t c1 = test(&v1, splitter);
            if (c0 != c1) {
                edge_t edge;
                if (!try_get_edge(&v0, &v1, &edge)) {
                    // this shouldn't happen, but oh well...
                    pieces[c0]->vertices.push_back(v0);
                    continue;
                }
                vertex_t v;
                if (!try_make_vertex(edge.faces[0], edge.faces[1], splitter, v)) {
                    // this shouldn't happen, but oh well...
                    pieces[c0]->vertices.push_back(v0);
                    continue;
                }
                if (c0 == RELATION_ALIGNED) {
                    pieces[c1]->vertices.push_back(v);
                }
                else if (c1 == RELATION_ALIGNED) {
                    pieces[c0]->vertices.push_back(v0);
                    pieces[c0]->vertices.push_back(v);
                }
                else {
                    pieces[c0]->vertices.push_back(v0);
                    pieces[c0]->vertices.push_back(v);
                    pieces[c1]->vertices.push_back(v);
                }
            }
            else {
                pieces[c0]->vertices.push_back(v0);
            }
        }
    }

    static std::vector<fragment_t> carve(
        fragment_t fragment,
        brush_t* brush,
        size_t face_index
    )
    {
        // this carves the given fragment into pieces that can be uniquely
        // classified as being inside/outside/aligned or reverse aligned
        // with the given brush. it does this by pushing the fragment down
        // the convex bsp-tree (just the list of faces) of the given brush
        std::vector<fragment_t> pieces;

        // recursion termination case
        if (face_index >= brush->faces.size()) {
            return { fragment };
        }

        face_t* face = &brush->faces[face_index];
        relation_t rel = test(&fragment, face);
        switch (rel) {
        case RELATION_FRONT:
            // early out: if the fragment is in front of any plane it
            // is outside the brush
            fragment.relation = RELATION_OUTSIDE;
            return { fragment };
        case RELATION_ALIGNED:
        case RELATION_REVERSE_ALIGNED:
            fragment.relation = rel;  // intentional fallthrough!
        case RELATION_BACK:
            // push the fragment further down the bsp-tree
            return carve(std::move(fragment), brush, face_index + 1);
        case RELATION_SPLIT: {
            fragment_t front;
            fragment_t back;
            split(&fragment, face, &front, &back);

            // push the back fragment further down the bsp-tree
            back.relation = fragment.relation;
            auto rest = carve(std::move(back), brush, face_index + 1);

            // prevent some redundant splitting
            if (rest.size() == 1 && rest[0].relation == RELATION_OUTSIDE) {
                fragment.relation = RELATION_OUTSIDE;
                return { fragment };
            }

            front.relation = RELATION_OUTSIDE;
            rest.push_back(std::move(front));
            return rest;
        }
        }

        // to shut up warning... (we shouldn't ever reach here)
        assert(0);
        return {};
    }

    static void rebuild_fragments(brush_t* brush) {
        for (face_t& face : brush->faces) {
            face.fragments.clear();

            // initialize the first fragment
            face.fragments.emplace_back();

            {
                volume_t void_volume = brush->world->void_volume;
                fragment_t* fragment = &face.fragments.back();
                fragment->face = &face;
                fragment->back_volume = brush->volume_operation(void_volume);
                fragment->front_volume = void_volume;
                fragment->back_brush = brush;
                fragment->front_brush = nullptr;
                fragment->vertices = face.vertices;

                // printf("FRONT %d BACK %d\n",
                // fragment->front_volume,
                // fragment->back_volume);
                // fflush(stdout);
            }

            /*
                HEART OF THE ALGORITHM
                use each intersecting brush to carve this brush's fragments into
                pieces, then depending on the piece's relation to the intersecting
                brush (inside/outside/aligned/reverse aligned) and the relative time
                between this brush and the intersecting brush-- adjust the piece's
                front/back volumes, or potentially discard the piece
            */
            for (brush_t* intersecting : brush->intersecting_brushes) {
                bool before_intersecting = b0_before_b1(brush, intersecting);
                int fragment_count = face.fragments.size();

                for (int fragment_index = fragment_count - 1;
                    fragment_index >= 0;
                    --fragment_index)
                {
                    fragment_t fragment = std::move(face.fragments[fragment_index]);
                    face.fragments.erase(face.fragments.begin() + fragment_index);

                    fragment.relation = RELATION_INSIDE;
                    std::vector<fragment_t> pieces = carve(std::move(fragment), intersecting, 0);
                    for (auto& piece : pieces) {
                        bool keep_piece = true;
                        switch (piece.relation) {
                        case RELATION_INSIDE:
                            if (before_intersecting) {
                                piece.back_volume = intersecting->volume_operation(piece.back_volume);
                                piece.back_brush = intersecting;
                            }
                            piece.front_volume = intersecting->volume_operation(piece.front_volume);
                            piece.front_brush = intersecting;
                            break;
                        case RELATION_ALIGNED:
                            if (before_intersecting)
                                keep_piece = false;
                            break;
                        case RELATION_REVERSE_ALIGNED:
                            if (before_intersecting) {
                                keep_piece = false;
                            }
                            else {
                                piece.front_volume = intersecting->volume_operation(piece.front_volume);
                                piece.front_brush = intersecting;
                            }
                            break;
                        }
                        if (keep_piece)
                            face.fragments.emplace_back(std::move(piece));
                    }
                }
            }
        }
    }

    std::set<brush_t*> world_t::rebuild() {
        // todo: parallelize per-brush work

        for (brush_t* brush : need_face_and_box_rebuild) {
            rebuild_faces_and_box(brush);
            need_fragment_rebuild.insert(brush);
        }

        for (brush_t* brush : need_face_and_box_rebuild) {
            recalculate_intersecting_brushes(brush);
            for (brush_t* intersecting : brush->intersecting_brushes) {
                need_fragment_rebuild.insert(intersecting);
            }
        }

        for (brush_t* brush : need_fragment_rebuild) {
            if (!need_face_and_box_rebuild.contains(brush)) {
                recalculate_intersecting_brushes(brush);
            }
            rebuild_fragments(brush);
        }

        std::set<brush_t*> rebuilt_brushes = need_fragment_rebuild;

        need_face_and_box_rebuild.clear();
        need_fragment_rebuild.clear();

        return rebuilt_brushes;
    }

}