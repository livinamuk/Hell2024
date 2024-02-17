#pragma once
#include "../../vendor/im3d/im3d.h"
#include "../Renderer/Shader.h"
//#include "../Util.hpp"

namespace Gizmo {

    GLuint g_Im3dVertexArray;
    GLuint g_Im3dVertexBuffer;
    GLuint g_Im3dShaderPoints;
    GLuint g_Im3dShaderLines;
    GLuint g_Im3dShaderTriangles;

    Shader g_TriangleShader;
    Shader g_LineShader;
    Shader g_PointShader;

    //enum State {TRANSLATE, ROTATE, SCALE};
    //State g_state = TRANSLATE;

    inline glm::mat4 Im3dMat4ToGlmMat4(Im3d::Mat4 pxMatrix) {
        glm::mat4 matrix;
        for (int x = 0; x < 4; x++)
            for (int y = 0; y < 4; y++)
                matrix[x][y] = pxMatrix[x * 4 + y];
        return matrix;
    }

    inline Im3d::Mat4 GlmMat4ToIm3dMat4(glm::mat4 glmMatrix) {
        Im3d::Mat4 matrix;
        for (int i = 0; i < 4; ++i) {
            for (int j = 0; j < 4; ++j) {
                matrix[i * 4 + j] = glmMatrix[i][j];
            }
        }
        return matrix;
    }


    void Init() {
        g_TriangleShader.Load("im3d_triangles.vert", "im3d_triangles.frag");
        g_LineShader.Load("im3d_lines.vert", "im3d_lines.frag", "im3d_lines.geom");
        g_PointShader.Load("im3d_points.vert", "im3d_points.frag");
        glGenBuffers(1, &g_Im3dVertexBuffer);;
        glGenVertexArrays(1, &g_Im3dVertexArray);
        glBindVertexArray(g_Im3dVertexArray);
        glBindBuffer(GL_ARRAY_BUFFER, g_Im3dVertexBuffer);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, sizeof(Im3d::VertexData), (GLvoid*)offsetof(Im3d::VertexData, m_positionSize));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(Im3d::VertexData), (GLvoid*)offsetof(Im3d::VertexData, m_color));
        glBindVertexArray(0);
    }

    inline glm::vec3 GetMouseRay(glm::mat4 projection, glm::mat4 view, int windowWidth, int windowHeight, int mouseX, int mouseY) {
        float x = (2.0f * mouseX) / (float)windowWidth - 1.0f;
        float y = 1.0f - (2.0f * mouseY) / (float)windowHeight;
        float z = 1.0f;
        glm::vec3 ray_nds = glm::vec3(x, y, z);
        glm::vec4 ray_clip = glm::vec4(ray_nds.x, ray_nds.y, ray_nds.z, 1.0f);
        glm::vec4 ray_eye = glm::inverse(projection) * ray_clip;
        ray_eye = glm::vec4(ray_eye.x, ray_eye.y, ray_eye.z, 0.0f);
        glm::vec4 inv_ray_wor = (inverse(view) * ray_eye);
        glm::vec3 ray_wor = glm::vec3(inv_ray_wor.x, inv_ray_wor.y, inv_ray_wor.z);
        ray_wor = normalize(ray_wor);
        return ray_wor;
    }
    
    inline glm::vec3 GetTranslationFromMatrix666(glm::mat4 matrix) {
        return glm::vec3(matrix[3][0], matrix[3][1], matrix[3][2]);
    }

    inline void NextGizmoMode() {
        int gizmoMode = (int)Im3d::GetContext().m_gizmoMode;
        gizmoMode++;
        if (gizmoMode == 3) {
            gizmoMode = 0;
        }
        Im3d::GetContext().m_gizmoMode = (Im3d::GizmoMode)(gizmoMode);
    }

    void Update(glm::vec3 viewPos, glm::vec3 viewDir, float mouseX, float mouseY, glm::mat4 projection, glm::mat4 view, bool leftMouseDown, float viewportWidth, float viewportHeight) {

        float deltaTime = 1.0f / 60.0f;

        Im3d::AppData& ad = Im3d::GetAppData();

        ad.m_deltaTime = deltaTime;
        ad.m_viewportSize = Im3d::Vec2((float)viewportWidth, (float)viewportHeight);
        ad.m_viewOrigin = { viewPos.x, viewPos.y, viewPos.z }; // for VR use the head position
        ad.m_viewDirection = { viewDir.x, viewDir.y, viewDir.z };
        ad.m_worldUp = Im3d::Vec3(0.0f, 1.0f, 0.0f); // used internally for generating orthonormal bases
        ad.m_projOrtho = false;// g_Example->m_camOrtho;

        // m_projScaleY controls how gizmos are scaled in world space to maintain a constant screen height
        ad.m_projScaleY = tanf(1.0f * 0.5f) * 2.0f; // or vertical fov for a perspective projection

        // World space cursor ray from mouse position; for VR this might be the position/orientation of the HMD or a tracked controller.
        Im3d::Vec2 cursorPos = { mouseX, mouseY };
        cursorPos.x = (cursorPos.x / ad.m_viewportSize.x) * 2.0f - 1.0f;
        cursorPos.y = (cursorPos.y / ad.m_viewportSize.y) * 2.0f - 1.0f;
        cursorPos.y = -cursorPos.y; // window origin is top-left, ndc is bottom-left
        Im3d::Vec3 rayOrigin, rayDirection;
        rayOrigin = ad.m_viewOrigin;

        glm::vec3 mouseRay = GetMouseRay(projection, view, viewportWidth, viewportHeight, mouseX, mouseY);
        rayDirection = { mouseRay.x, mouseRay.y, mouseRay.z };

        ad.m_cursorRayOrigin = rayOrigin;
        ad.m_cursorRayDirection = rayDirection;

        // Fill the key state array; using GetAsyncKeyState here but this could equally well be done via the window proc.
        // All key states have an equivalent (and more descriptive) 'Action_' enum.
        ad.m_keyDown[Im3d::Mouse_Left/*Im3d::Action_Select*/] = leftMouseDown;

    }

    Im3d::Mat4 Draw(glm::mat4 projection, glm::mat4 view, float viewportWidth, float viewportHeight, glm::mat4 matrix) {

        Im3d::NewFrame();

        // give shit to the library

        Im3d::Context& ctx = Im3d::GetContext();
        Im3d::AppData& ad = Im3d::GetAppData();



        // this needs expanding upon and refactoring !!!
        // this needs expanding upon and refactoring !!!
        // this needs expanding upon and refactoring !!!
        // this needs expanding upon and refactoring !!!

        static Im3d::Mat4 transform = (1.0f);
        transform = GlmMat4ToIm3dMat4(matrix);





        // Context-global gizmo modes are set via actions in the AppData::m_keyDown but could also be modified via a GUI as follows:
      //  int gizmoMode = (int)Im3d::GetContext().m_gizmoMode;

      //  Im3d::GetContext().m_gizmoMode = (Im3d::GizmoMode)gizmoMode;
       
       // Im3d::GetContext().m_gizmoMode = (Im3d::GizmoMode)(g_state);

        // The ID passed to Gizmo() should be unique during a frame - to create gizmos in a loop use PushId()/PopId().
        if (Im3d::Gizmo("GizmoUnified", transform))
        {
           // int gizmoMode = (int)Im3d::GetContext().m_gizmoMode;
           /*gizmoMode = Im3d::GizmoMode_Translation;
            ImGui::SameLine();
            ImGui::RadioButton("Rotate (Ctrl+R)", &gizmoMode, Im3d::GizmoMode_Rotation);
            ImGui::SameLine();
            ImGui::RadioButton("Scale (Ctrl+S)", &gizmoMode, Im3d::GizmoMode_Scale);*/ 
         


        }


        ///////////////////////////

     /*  Im3d::PushDrawState();
        Im3d::SetSize(2.0f);
        Im3d::BeginLineLoop();
        Im3d::Vertex(0.0f, 0.0f, 0.0f, Im3d::Color_Magenta);
        Im3d::Vertex(1.0f, 1.0f, 0.0f, Im3d::Color_Yellow);
        Im3d::Vertex(2.0f, 2.0f, 0.0f, Im3d::Color_Cyan);
        Im3d::End();
        Im3d::PopDrawState();*/ 

        Im3d::EndFrame();


       // float viewportWidth = 1920 * 1.5f;
      //  float viewportHeight = 1080 * 1.5f;

        // Primitive rendering.
        // Typical pipeline state: enable alpha blending, disable depth test and backface culling.
        (glEnable(GL_BLEND));
        (glBlendEquation(GL_FUNC_ADD));
        (glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));
        (glEnable(GL_PROGRAM_POINT_SIZE));
        (glDisable(GL_DEPTH_TEST));
        (glDisable(GL_CULL_FACE));

        (glViewport(0, 0, (GLsizei)viewportWidth, (GLsizei)viewportHeight));

        for (Im3d::U32 i = 0, n = Im3d::GetDrawListCount(); i < n; ++i)
        {
            const Im3d::DrawList& drawList = Im3d::GetDrawLists()[i];

            if (drawList.m_layerId == Im3d::MakeId("NamedLayer"))
            {
                // The application may group primitives into layers, which can be used to change the draw state (e.g. enable depth testing, use a different shader)
            }

            Shader* shader = nullptr;
            GLenum prim;
            //          GLuint sh;
            switch (drawList.m_primType)
            {
            case Im3d::DrawPrimitive_Points:
                prim = GL_POINTS;

                //sh = g_Im3dShaderPoints;
                shader = &g_PointShader;
                (glDisable(GL_CULL_FACE)); // points are view-aligned
                break;
            case Im3d::DrawPrimitive_Lines:
                prim = GL_LINES;
                //sh = g_Im3dShaderLines;
                shader = &g_LineShader;
                (glDisable(GL_CULL_FACE)); // lines are view-aligned
                break;
            case Im3d::DrawPrimitive_Triangles:
                prim = GL_TRIANGLES;
                //sh = g_Im3dShaderTriangles; 
                shader = &g_TriangleShader;
                //glAssert(glEnable(GL_CULL_FACE)); // culling valid for triangles, but optional
                break;
            default:
                IM3D_ASSERT(false);
                return transform;
            };

            if (nullptr) {
                continue;
            }

            (glBindVertexArray(g_Im3dVertexArray));
            (glBindBuffer(GL_ARRAY_BUFFER, g_Im3dVertexBuffer));
            (glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)drawList.m_vertexCount * sizeof(Im3d::VertexData), (GLvoid*)drawList.m_vertexData, GL_STREAM_DRAW));

            Im3d::AppData& ad = Im3d::GetAppData();

            //(glUseProgram(sh));
            //(glUniform2f(glGetUniformLocation(sh, "uViewport"), ad.m_viewportSize.x, ad.m_viewportSize.y));
            //(glUniformMatrix4fv(glGetUniformLocation(sh, "uViewProjMatrix"), 1, false, (const GLfloat*)g_Example->m_camViewProj));

            shader->Use();
            shader->SetVec2("uViewport", glm::vec2(viewportWidth, viewportHeight));
            shader->SetMat4("uViewProjMatrix", projection * view);

            (glDrawArrays(prim, 0, (GLsizei)drawList.m_vertexCount));
        }

        glm::mat4 gizmoWorldMatrix = Im3dMat4ToGlmMat4(transform);
        glm::vec3 gizmoWorldPos = GetTranslationFromMatrix666(gizmoWorldMatrix);

        //  Im3d::NewFrame();

        return transform;
    }

}