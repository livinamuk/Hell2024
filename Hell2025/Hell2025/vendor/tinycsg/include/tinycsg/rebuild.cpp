#include "tinycsg.hpp"

#include <map>
#include <array>
#include <algorithm>
#include <future>

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

    /*
    static bool approx_equal(float a, float b) {
        // compare to 3 decimal places
        return int(round(a * 1000)) == int(round(b * 1000));
    }*/

    static bool approx_equal(float a, float b, float epsilon = 0.001f) {
        return std::abs(a - b) < epsilon;
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

    /*
    static relation_t test(vertex_t* vertex, const face_t* face) {
        float d = signed_distance(vertex->position, *face->plane);
        if (approx_equal(d, 0.0f))
            return RELATION_ALIGNED;
        else if (d > 0)
            return RELATION_FRONT;
        else
            return RELATION_BACK;
    }*/

    static relation_t test(vertex_t* vertex, const face_t* face) {
        float d = signed_distance(vertex->position, *face->plane);
        if (approx_equal(d, 0.0f)) {
            return RELATION_ALIGNED;
        }
        return (d > 0) ? RELATION_FRONT : RELATION_BACK;
    }


    /*
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
    }*/

    static relation_t test(vertex_t* vertex, brush_t* brush) {
        relation_t rel = RELATION_INSIDE;

        for (const face_t& face : brush->faces) {
            relation_t faceRel = test(vertex, &face);

            if (faceRel == RELATION_FRONT) {
                return RELATION_OUTSIDE;
            }

            if (faceRel == RELATION_ALIGNED) {
                rel = RELATION_ALIGNED;
            }
        }

        return rel;
    }

    /*
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
    }*/

    static relation_t test(fragment_t* fragment, face_t* face) {
        int insideCount = 0;
        int outsideCount = 0;

        // Count the number of inside, outside, and aligned vertices
        for (vertex_t& v : fragment->vertices) {
            relation_t rel = test(&v, face);
            if (rel == RELATION_INSIDE) {
                ++insideCount;
            }
            else if (rel == RELATION_OUTSIDE) {
                ++outsideCount;
            }
            // Early exit if the fragment is split
            if (insideCount > 0 && outsideCount > 0) {
                return RELATION_SPLIT;
            }
        }

        // Handle cases where all vertices are either aligned or neither inside nor outside
        if (outsideCount == 0 && insideCount == 0) {
            float d = glm::dot(face->plane->normal, fragment->face->plane->normal);
            return (d < 0.0f) ? RELATION_REVERSE_ALIGNED : RELATION_ALIGNED;
        }

        // Determine the relationship based on the counts
        return (insideCount > 0) ? RELATION_INSIDE : RELATION_OUTSIDE;
    }

    /*
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
    }*/

    static void recalculate_intersecting_brushes(brush_t* brush) {
        // Note you aren't checking TIME! because you don't use that feature

        // Query intersecting brushes and store the result
        brush->intersecting_brushes = brush->world->query_box(brush->box);

        // Remove 'brush' from the list of intersecting brushes (if present)
        auto& brushes = brush->intersecting_brushes;
        auto it = std::remove(brushes.begin(), brushes.end(), brush);
        if (it != brushes.end()) {
            brushes.erase(it, brushes.end());
        }
    }

    static bool try_get_edge(const vertex_t* vertex0, const vertex_t* vertex1, edge_t* edge) {
        face_t* sharedFaces[2] = { nullptr, nullptr };
        int sharedCount = 0;
        // Iterate over the face arrays to find shared faces
        for (int i = 0; i < 3; ++i) {
            for (int j = 0; j < 3; ++j) {
                if (vertex0->faces[i] == vertex1->faces[j]) {
                    sharedFaces[sharedCount++] = vertex0->faces[i];
                    if (sharedCount == 2) {
                        // Early exit if two shared faces are found
                        if (edge) {
                            *edge = edge_t{ {sharedFaces[0], sharedFaces[1]} };
                        }
                        return true;
                    }
                }
            }
        }
        // If less than two shared faces are found, return false
        return false;
    }

    /*
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
    }*/

    static bool try_make_vertex(face_t* f0, face_t* f1, face_t* f2, vertex_t& v) {

        glm::mat3 m = glm::transpose(glm::mat3(
            f0->plane->normal,
            f1->plane->normal,
            f2->plane->normal
        ));

        float D = glm::determinant(m);

        if (approx_equal(D, 0.0f)) {
            return false;  // Planes are parallel or nearly parallel, so no unique intersection point
        }

        glm::vec3 offsets(f0->plane->offset, f1->plane->offset, f2->plane->offset);

        glm::mat3 mx = m;
        glm::mat3 my = m;
        glm::mat3 mz = m;

        mx[0] = -offsets;  // Replace the first column
        my[1] = -offsets;  // Replace the second column
        mz[2] = -offsets;  // Replace the third column

        v.faces[0] = f0;
        v.faces[1] = f1;
        v.faces[2] = f2;

        float invD = 1.0f / D;
        v.position = glm::vec3(
            glm::determinant(mx) * invD,
            glm::determinant(my) * invD,
            glm::determinant(mz) * invD
        );
        return true;
    }

    static void order_vertices(face_t* face) {
        // Use the information we have on the vertices to order them properly without any floating point calculations

        // Move the vertices from face to unsorted
        std::vector<vertex_t> unsorted = std::move(face->vertices);
        face->vertices.clear();
        face->vertices.reserve(unsorted.size());  // Reserve space to avoid reallocations

        // Start with the first vertex
        face->vertices.push_back(std::move(unsorted.back()));
        unsorted.pop_back();

        while (!unsorted.empty()) {
            const vertex_t& lastVertex = face->vertices.back();

            // Find the next vertex connected to the last one
            auto it = std::find_if(unsorted.begin(), unsorted.end(), [&](const vertex_t& other) {
                return try_get_edge(&lastVertex, &other, nullptr);
            });

            if (it != unsorted.end()) {
                face->vertices.push_back(std::move(*it));
                unsorted.erase(it);  // Erase after moving the vertex
            }
            else {
                // If no connected vertex is found, break out to avoid infinite loops
                break;
            }
        }
    }

    /*
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
    }*/

    /*
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
    }*/

    static void fix_winding(face_t* face) {
        // Check if there are fewer than 3 vertices, in which case winding doesn't matter
        if (face->vertices.size() < 3) {
            return;
        }
        // Compute the cross product to determine the winding direction
        const glm::vec3& v0 = face->vertices[0].position;
        const glm::vec3& v1 = face->vertices[1].position;
        const glm::vec3& v2 = face->vertices[2].position;
        // Compute the determinant to check the orientation
        if (glm::dot(glm::cross(v1 - v0, v2 - v0), face->plane->normal) < 0.0f) {
            std::reverse(face->vertices.begin(), face->vertices.end());
        }
    }

    /*
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
    }*/

    static void rebuild_faces_and_box(brush_t* brush) {
        brush->faces.clear();
        int n = brush->planes.size();
        brush->faces.resize(n);

        for (int i = 0; i < n; ++i) {
            brush->faces[i].plane = &brush->planes[i];
        }

        bool box_initialized = false;

        for (int i = 0; i < n - 2; ++i) {
            for (int j = i + 1; j < n - 1; ++j) {
                for (int k = j + 1; k < n; ++k) {
                    vertex_t v;
                    if (try_make_vertex(&brush->faces[i], &brush->faces[j], &brush->faces[k], v) &&
                        test(&v, brush) != RELATION_OUTSIDE) {

                        brush->faces[i].vertices.push_back(v);
                        brush->faces[j].vertices.push_back(v);
                        brush->faces[k].vertices.push_back(v);

                        if (!box_initialized) {
                            brush->box = box_t{ v.position, v.position };
                            box_initialized = true;
                        }
                        else {
                            brush->box = extended(brush->box, v.position);
                        }
                    }
                }
            }
        }

        for (auto& face : brush->faces) {
            order_vertices(&face);
            fix_winding(&face);
        }
    }

    /*
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
    }*/

    static void split(fragment_t* fragment, face_t* splitter, fragment_t* front, fragment_t* back) {
        // Splits fragment into front and back piece w.r.t. face
        // Call only if test(fragment, face) == RELATION_SPLIT

        // Initialize the front and back pieces
        front->face = back->face = fragment->face;
        front->back_volume = back->back_volume = fragment->back_volume;
        front->front_volume = back->front_volume = fragment->front_volume;
        front->back_brush = back->back_brush = fragment->back_brush;
        front->front_brush = back->front_brush = fragment->front_brush;

        front->vertices.clear();
        back->vertices.clear();

        // Split the fragment into front and back pieces
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
                    // Handle the unlikely case of a missing edge
                    (c0 == RELATION_FRONT ? front : back)->vertices.push_back(v0);
                    continue;
                }

                vertex_t v;
                if (!try_make_vertex(edge.faces[0], edge.faces[1], splitter, v)) {
                    // Handle the unlikely case of a failed vertex creation
                    (c0 == RELATION_FRONT ? front : back)->vertices.push_back(v0);
                    continue;
                }

                if (c0 == RELATION_ALIGNED) {
                    (c1 == RELATION_FRONT ? front : back)->vertices.push_back(v);
                }
                else if (c1 == RELATION_ALIGNED) {
                    (c0 == RELATION_FRONT ? front : back)->vertices.push_back(v0);
                    (c0 == RELATION_FRONT ? front : back)->vertices.push_back(v);
                }
                else {
                    (c0 == RELATION_FRONT ? front : back)->vertices.push_back(v0);
                    (c0 == RELATION_FRONT ? front : back)->vertices.push_back(v);
                    (c1 == RELATION_FRONT ? front : back)->vertices.push_back(v);
                }
            }
            else {
                (c0 == RELATION_FRONT ? front : back)->vertices.push_back(v0);
            }
        }
    }


    /*
    static std::vector<fragment_t> carve(fragment_t fragment, brush_t* brush, size_t face_index) {
        std::vector<fragment_t> pieces;

        // Recursion termination case
        if (face_index >= brush->faces.size()) {
            return { std::move(fragment) };
        }

        face_t* face = &brush->faces[face_index];
        relation_t rel = test(&fragment, face);

        switch (rel) {
        case RELATION_FRONT:
            // Early out: if the fragment is in front of any plane, it is outside the brush
            fragment.relation = RELATION_OUTSIDE;
            return { std::move(fragment) };

        case RELATION_ALIGNED:
        case RELATION_REVERSE_ALIGNED:
            fragment.relation = rel;  // Intentional fallthrough to RELATION_BACK
        case RELATION_BACK:
            // Push the fragment further down the BSP-tree
            return carve(std::move(fragment), brush, face_index + 1);

        case RELATION_SPLIT: {
            fragment_t front, back;
            split(&fragment, face, &front, &back);

            // Push the back fragment further down the BSP-tree
            back.relation = fragment.relation;
            auto rest = carve(std::move(back), brush, face_index + 1);

            // Prevent some redundant splitting
            if (rest.size() == 1 && rest[0].relation == RELATION_OUTSIDE) {
                fragment.relation = RELATION_OUTSIDE;
                return { std::move(fragment) };
            }

            front.relation = RELATION_OUTSIDE;
            rest.push_back(std::move(front));
            return rest;
        }
        }

        // Should never reach here, added to satisfy return type and prevent warnings
        assert(false);
        return {};
    }*/

    static std::vector<fragment_t> carve(
        fragment_t fragment,
        brush_t* brush,
        size_t face_index
    ) {
        // Termination case: no more faces to process
        if (face_index >= brush->faces.size()) {
            return { std::move(fragment) };
        }

        face_t* face = &brush->faces[face_index];
        relation_t rel = test(&fragment, face);

        switch (rel) {
        case RELATION_FRONT:
            // If in front of any plane, it's outside the brush
            fragment.relation = RELATION_OUTSIDE;
            return { std::move(fragment) };

        case RELATION_ALIGNED:
        case RELATION_REVERSE_ALIGNED:
            fragment.relation = rel;
            [[fallthrough]];  // Continue to RELATION_BACK

        case RELATION_BACK:
            // Process the next face recursively
            return carve(std::move(fragment), brush, face_index + 1);

        case RELATION_SPLIT: {
            fragment_t front, back;
            split(&fragment, face, &front, &back);

            back.relation = fragment.relation;
            auto rest = carve(std::move(back), brush, face_index + 1);

            // Avoid redundant splitting
            if (rest.size() == 1 && rest[0].relation == RELATION_OUTSIDE) {
                fragment.relation = RELATION_OUTSIDE;
                return { std::move(fragment) };
            }

            front.relation = RELATION_OUTSIDE;
            rest.push_back(std::move(front));
            return rest;
        }
        }

        assert(false);  // Should never reach here
        return {};
    }


    /*
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
    }*/



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

            //
            //   HEART OF THE ALGORITHM
            //   use each intersecting brush to carve this brush's fragments into
            //   pieces, then depending on the piece's relation to the intersecting
            //   brush (inside/outside/aligned/reverse aligned) and the relative time
            //   between this brush and the intersecting brush-- adjust the piece's
            //   front/back volumes, or potentially discard the piece

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

/*static void rebuild_fragments(brush_t* brush) {
    for (face_t& face : brush->faces) {
        face.fragments.clear();

        // Initialize the first fragment
        face.fragments.emplace_back();
        {
            volume_t void_volume = brush->world->void_volume;
            fragment_t& fragment = face.fragments.back();
            fragment.face = &face;
            fragment.back_volume = brush->volume_operation(void_volume);
            fragment.front_volume = void_volume;
            fragment.back_brush = brush;
            fragment.front_brush = nullptr;
            fragment.vertices = face.vertices;
        }

        for (brush_t* intersecting : brush->intersecting_brushes) {
            size_t original_fragment_count = face.fragments.size();

            std::vector<fragment_t> new_fragments;
            new_fragments.reserve(original_fragment_count * 2);  // Pre-reserve more space to minimize allocations

            for (size_t fragment_index = 0; fragment_index < original_fragment_count; ++fragment_index) {
                fragment_t& fragment = face.fragments[fragment_index];

                fragment.relation = RELATION_INSIDE;
                std::vector<fragment_t> pieces = carve(std::move(fragment), intersecting, 0);

                for (auto& piece : pieces) {
                    bool keep_piece = true;
                    switch (piece.relation) {
                    case RELATION_INSIDE:
                        piece.back_volume = intersecting->volume_operation(piece.back_volume);
                        piece.back_brush = intersecting;
                        piece.front_volume = intersecting->volume_operation(piece.front_volume);
                        piece.front_brush = intersecting;
                        break;

                    case RELATION_ALIGNED:
                        // No need to keep aligned pieces since no time comparison
                        keep_piece = false;
                        break;

                    case RELATION_REVERSE_ALIGNED:
                        // Without time comparison, treat as INSIDE
                        piece.front_volume = intersecting->volume_operation(piece.front_volume);
                        piece.front_brush = intersecting;
                        break;
                    }
                    if (keep_piece)
                        new_fragments.emplace_back(std::move(piece));
                }
            }

            // Replace old fragments with new fragments
            face.fragments = std::move(new_fragments);
        }
    }
}*/
    /*

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
    */

    std::set<brush_t*> world_t::rebuild() {
        std::vector<std::future<void>> futures;

        // Rebuilding faces and boxes (parallelized)
        for (brush_t* brush : need_face_and_box_rebuild) {
            futures.push_back(std::async(std::launch::async, [this, brush]() {
                rebuild_faces_and_box(brush);
                need_fragment_rebuild.insert(brush);
            }));
        }
        for (auto& future : futures) {
            future.get();
        }
        futures.clear();

        // Recalculating intersecting brushes
        for (brush_t* brush : need_face_and_box_rebuild) {
            futures.push_back(std::async(std::launch::async, [this, brush]() {
                recalculate_intersecting_brushes(brush);
                for (brush_t* intersecting : brush->intersecting_brushes) {
                    need_fragment_rebuild.insert(intersecting);
                }
            }));
        }
        for (auto& future : futures) {
            future.get();
        }
        futures.clear();

        // Rebuilding fragments
        for (brush_t* brush : need_fragment_rebuild) {
            futures.push_back(std::async(std::launch::async, [this, brush]() {
                if (!need_face_and_box_rebuild.contains(brush)) {
                    recalculate_intersecting_brushes(brush);
                }
                rebuild_fragments(brush);
            }));
        }
        for (auto& future : futures) {
            future.get();
        }

        std::set<brush_t*> rebuilt_brushes = need_fragment_rebuild;

        need_face_and_box_rebuild.clear();
        need_fragment_rebuild.clear();

        return rebuilt_brushes;
    }

}