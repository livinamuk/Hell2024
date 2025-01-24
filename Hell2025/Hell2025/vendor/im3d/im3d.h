#pragma once

// Define IM3D_CONFIG "myfilename.h" from your build system if you do not want to modify im3d_config.h directly.
#ifdef IM3D_CONFIG
	#include IM3D_CONFIG
#else
	#include "im3d_config.h"
#endif

#define IM3D_VERSION "1.16"

#ifndef IM3D_API
	#define IM3D_API
#endif

#ifndef IM3D_ASSERT
	#include <cassert>
	#define IM3D_ASSERT(e) assert(e)
#endif

#ifndef IM3D_VERTEX_ALIGNMENT
	#define IM3D_VERTEX_ALIGNMENT 4
#endif

#include <cstdarg> // va_list

namespace Im3d {

typedef unsigned int U32;
struct Vec2;
struct Vec3;
struct Vec4;
struct Mat3;
struct Mat4;
struct Color;
struct VertexData;
struct AppData;
struct DrawList;
struct TextDrawList;
struct Context;

typedef U32 Id;
constexpr Id Id_Invalid = 0;

// Get AppData struct from the current context, fill before calling NewFrame().
IM3D_API AppData& GetAppData();

// Call at the start of each frame, after filling the AppData struct.
IM3D_API void NewFrame();
// Call after all Im3d calls have been made for the current frame, before accessing draw data.
IM3D_API void EndFrame();

// Access draw data. Draw lists are valid after calling EndFrame() and before calling NewFrame().
IM3D_API const DrawList* GetDrawLists();
IM3D_API U32 GetDrawListCount();

// Access to text draw data. Draw lists are valid after calling EndFrame() and before calling NewFrame().
IM3D_API const TextDrawList* GetTextDrawLists();
IM3D_API U32 GetTextDrawListCount();

// DEPRECATED (use EndFrame() + GetDrawLists()).
// Call after all Im3d calls have been made for the current frame.
IM3D_API void Draw();


// Begin/end primitive. End() must be called before starting each new primitive type.
IM3D_API void BeginPoints();
IM3D_API void BeginLines();
IM3D_API void BeginLineLoop();
IM3D_API void BeginLineStrip();
IM3D_API void BeginTriangles();
IM3D_API void BeginTriangleStrip();
IM3D_API void End();

// Add a vertex to the current primitive (call between Begin*() and End()).
IM3D_API void Vertex(const Vec3& _position);
IM3D_API void Vertex(const Vec3& _position, Color _color);
IM3D_API void Vertex(const Vec3& _position, float _size);
IM3D_API void Vertex(const Vec3& _position, float _size, Color _color);
IM3D_API void Vertex(float _x, float _y, float _z);
IM3D_API void Vertex(float _x, float _y, float _z, Color _color);
IM3D_API void Vertex(float _x, float _y, float _z, float _size);
IM3D_API void Vertex(float _x, float _y, float _z, float _size, Color _color);

// Color draw state (per vertex).
IM3D_API void PushColor(); // push the stack top
IM3D_API void PushColor(Color _color);
IM3D_API void PopColor();
IM3D_API void SetColor(Color _color);
IM3D_API void SetColor(float _r, float _g, float _b, float _a = 1.0f);
IM3D_API Color GetColor();

// Alpha draw state, multiplies the alpha set by the color draw state (per vertex).
IM3D_API void PushAlpha(); // push the stack top
IM3D_API void PushAlpha(float _alpha);
IM3D_API void PopAlpha();
IM3D_API void SetAlpha(float _alpha);
IM3D_API float GetAlpha();

// Size draw state, for points/lines this is the radius/width in pixels (per vertex).
IM3D_API void PushSize(); // push the stack top
IM3D_API void PushSize(float _size);
IM3D_API void PopSize();
IM3D_API void SetSize(float _size);
IM3D_API float GetSize();

// Sorting draw state, enable depth sorting between primitives (per primitive).
IM3D_API void PushEnableSorting(); // push the stack top
IM3D_API void PushEnableSorting(bool _enable);
IM3D_API void PopEnableSorting();
IM3D_API void EnableSorting(bool _enable);

// Push/pop all draw states (color, alpha, size, sorting).
IM3D_API void PushDrawState();
IM3D_API void PopDrawState();

// Transform state (per vertex).
IM3D_API void PushMatrix(); // push stack top
IM3D_API void PushMatrix(const Mat4& _mat4);
IM3D_API void PopMatrix();
IM3D_API void SetMatrix(const Mat4& _mat4);
IM3D_API void SetIdentity();
IM3D_API void MulMatrix(const Mat4& _mat4);
IM3D_API void Translate(float _x, float _y, float _z);
IM3D_API void Translate(const Vec3& _vec3);
IM3D_API void Rotate(const Vec3& _axis, float _angle);
IM3D_API void Rotate(const Mat3& _rotation);
IM3D_API void Scale(float _x, float _y, float _z);

// High order shapes. Where _detail = -1, an automatic level of detail is chosen based on the distance to the view origin (as specified via the AppData struct).
IM3D_API void DrawXyzAxes();
IM3D_API void DrawPoint(const Vec3& _position, float _size, Color _color);
IM3D_API void DrawLine(const Vec3& _a, const Vec3& _b, float _size, Color _color);
IM3D_API void DrawQuad(const Vec3& _a, const Vec3& _b, const Vec3& _c, const Vec3& _d);
IM3D_API void DrawQuad(const Vec3& _origin, const Vec3& _normal, const Vec2& _size);
IM3D_API void DrawQuadFilled(const Vec3& _a, const Vec3& _b, const Vec3& _c, const Vec3& _d);
IM3D_API void DrawQuadFilled(const Vec3& _origin, const Vec3& _normal, const Vec2& _size);
IM3D_API void DrawCircle(const Vec3& _origin, const Vec3& _normal, float _radius, int _detail = -1);
IM3D_API void DrawCircleFilled(const Vec3& _origin, const Vec3& _normal, float _radius, int _detail = -1);
IM3D_API void DrawSphere(const Vec3& _origin, float _radius, int _detail = -1);
IM3D_API void DrawSphereFilled(const Vec3& _origin, float _radius, int _detail = -1);
IM3D_API void DrawAlignedBox(const Vec3& _min, const Vec3& _max);
IM3D_API void DrawAlignedBoxFilled(const Vec3& _min, const Vec3& _max);
IM3D_API void DrawCylinder(const Vec3& _start, const Vec3& _end, float _radius, int _detail = -1);
IM3D_API void DrawCapsule(const Vec3& _start, const Vec3& _end, float _radius, int _detail = -1);
IM3D_API void DrawPrism(const Vec3& _start, const Vec3& _end, float _radius, int _sides);
IM3D_API void DrawArrow(const Vec3& _start, const Vec3& _end, float _headLength = -1.0f, float _headThickness = -1.0f);
IM3D_API void DrawCone(const Vec3& _origin, const Vec3& _normal,float height, float _radius, int _detail);
IM3D_API void DrawConeFilled(const Vec3& _origin, const Vec3& _normal,float height, float _radius, int _detail);

// Add text. See TextFlags_ enum for _textFlags. _size is a hint to the application-side text rendering.
IM3D_API void Text(const Vec3& _position, U32 _textFlags, const char* _text, ...); // use the current draw state for size/color
IM3D_API void Text(const Vec3& _position, float _size, Color _color, U32 _textFlags, const char* _text, ...);

// IDs are used to uniquely identify gizmos and layers. Gizmo should have a unique ID during a frame.
// Note that ids are a hash of the whole ID stack, see PushId(), PopId().
IM3D_API Id MakeId(const char* _str);
IM3D_API Id MakeId(const void* _ptr);
IM3D_API Id MakeId(int _i);

// PushId(), PopId() affect the result of subsequent calls to MakeId(), use when creating gizmos in a loop.
IM3D_API void PushId(); // push stack top
IM3D_API void PushId(Id _id);
IM3D_API void PushId(const char* _str);
IM3D_API void PushId(const void* _ptr);
IM3D_API void PushId(int _i);
IM3D_API void PopId();
IM3D_API Id   GetId();

// Layer id state, subsequent primitives are added to a separate draw list associated with the id (per primitive).
IM3D_API void PushLayerId(Id _layer);
IM3D_API void PushLayerId(const char* _str); // calls PushLayerId(MakeId(_str))
IM3D_API void PopLayerId();
IM3D_API Id   GetLayerId();

// Manipulate translation/rotation/scale via a gizmo. Return true if the gizmo is 'active' (if it modified the output parameter).
// If _local is true, the Gizmo* functions expect that the local matrix is on the matrix stack; in general the application should
// push the local matrix before calling any of the following.
IM3D_API bool GizmoTranslation(const char* _id, float _translation_[3], bool _local = false);
IM3D_API bool GizmoRotation(const char* _id, float _rotation_[3*3], bool _local = false);
IM3D_API bool GizmoScale(const char* _id, float _scale_[3]); // local scale only
// Unified gizmo, selects local/global, translation/rotation/scale based on the context-global gizmo modes. Return true if the gizmo is active.
IM3D_API bool Gizmo(const char* _id, float _translation_[3], float _rotation_[3*3], float _scale_[3]); // any of _translation_/_rotation_/_scale_ may be null.
IM3D_API bool Gizmo(const char* _id, float _transform_[4*4]);

// Gizmo* overloads which take an ID directly. In some cases the app may want to call MakeId() separately, usually to change the gizmo appearance if hot/active.
IM3D_API bool GizmoTranslation(Id _id, float _translation_[3], bool _local = false);
IM3D_API bool GizmoRotation(Id _id, float _rotation_[3*3], bool _local = false);
IM3D_API bool GizmoScale(Id _id, float _scale_[3]);
IM3D_API bool Gizmo(Id _id, float _transform_[4*4]);
IM3D_API bool Gizmo(Id _id, float _translation_[3], float _rotation_[3*3], float _scale_[3]);

// Active gizmo ID. This will match the _id parameter passed to a Gizmo* function. Return Id_Invalid if no gizmo is in use.
IM3D_API Id GetActiveId();
// ID of the current current 'hot' gizmo (nearest intersecting gizmo along the cursor ray).
IM3D_API Id GetHotId();

// Visibility tests. The application must set a culling frustum via AppData.
IM3D_API bool IsVisible(const Vec3& _origin, float _radius); // sphere
IM3D_API bool IsVisible(const Vec3& _min, const Vec3& _max); // axis-aligned bounding box

// Get/set the current context. All Im3d calls affect the currently bound context.
IM3D_API Context& GetContext();
IM3D_API void SetContext(Context& _ctx);

// Merge vertex data from _src into _dst_. Layers are preserved. Call before EndFrame().
IM3D_API void MergeContexts(Context& _dst_, const Context& _src);


struct IM3D_API Vec2
{
	float x, y;

	Vec2()                                                                   {}
	Vec2(float _xy): x(_xy), y(_xy)                                          {}
	Vec2(float _x, float _y): x(_x), y(_y)                                   {}

	operator float*()                                                        { return &x; }
	operator const float*() const                                            { return &x; }

	#ifdef IM3D_VEC2_APP
		IM3D_VEC2_APP
	#endif
};

struct IM3D_API Vec3
{
	float x, y, z;

	Vec3()                                                                   {}
	Vec3(float _xyz): x(_xyz), y(_xyz), z(_xyz)                              {}
	Vec3(float _x, float _y, float _z): x(_x), y(_y), z(_z)                  {}
	Vec3(const Vec2& _xy, float _z): x(_xy.x), y(_xy.y), z(_z)               {}
	Vec3(const Vec4& _v); // discards w

	operator float*()                                                        { return &x; }
	operator const float*() const                                            { return &x; }

	#ifdef IM3D_VEC3_APP
		IM3D_VEC3_APP
	#endif
};

struct IM3D_API Vec4
{
	float x, y, z, w;

	Vec4()                                                                   {}
	Vec4(float _xyzw): x(_xyzw), y(_xyzw), z(_xyzw), w(_xyzw)                {}
	Vec4(float _x, float _y, float _z, float _w): x(_x), y(_y), z(_z), w(_w) {}
	Vec4(const Vec3& _xyz, float _w): x(_xyz.x), y(_xyz.y), z(_xyz.z), w(_w) {}
	Vec4(Color _rgba);

	operator float*()                                                        { return &x; }
	operator const float*() const                                            { return &x; }

	#ifdef IM3D_VEC4_APP
		IM3D_VEC4_APP
	#endif
};

struct IM3D_API Mat3
{
	float m[3*3]; // column-major unless IM3D_MATRIX_ROW_MAJOR defined

	Mat3()                                                                   {}
	Mat3(float _diagonal);
	Mat3(
		float m00, float m01, float m02,
		float m10, float m11, float m12,
		float m20, float m21, float m22
		);
	Mat3(const Vec3& _colX, const Vec3& _colY, const Vec3& _colZ);
	Mat3(const Mat4& _mat4); // extract upper 3x3

	operator float*()                                                        { return m; }
	operator const float*() const                                            { return m; }

	Vec3 getCol(int _i) const;
	Vec3 getRow(int _i) const;
	void setCol(int _i, const Vec3& _v);
	void setRow(int _i, const Vec3& _v);

	Vec3 getScale() const;
	void setScale(const Vec3& _scale);

	float operator()(int _row, int _col) const
	{
		#ifdef IM3D_MATRIX_ROW_MAJOR
			int i = _row * 3 + _col;
		#else
			int i = _col * 3 + _row;
		#endif
		return m[i];
	}
	float& operator()(int _row, int _col)
	{
		#ifdef IM3D_MATRIX_ROW_MAJOR
			int i = _row * 3 + _col;
		#else
			int i = _col * 3 + _row;
		#endif
		return m[i];
	}

	#ifdef IM3D_MAT3_APP
		IM3D_MAT3_APP
	#endif
};

struct IM3D_API Mat4
{
	float m[4*4]; // column-major unless IM3D_MATRIX_ROW_MAJOR defined

	Mat4()                                                                   {}
	Mat4(float _diagonal);
	Mat4(
		float m00,        float m01,        float m02,        float m03,
		float m10,        float m11,        float m12,        float m13,
		float m20,        float m21,        float m22,        float m23,
		float m30 = 0.0f, float m31 = 0.0f, float m32 = 0.0f, float m33 = 1.0f
		);
	Mat4(const Mat3& _mat3);
	Mat4(const Vec3& _translation, const Mat3& _rotation, const Vec3& _scale);

	operator float*()                                                        { return m; }
	operator const float*() const                                            { return m; }

	Vec4 getCol(int _i) const;
	Vec4 getRow(int _i) const;
	void setCol(int _i, const Vec4& _v);
	void setRow(int _i, const Vec4& _v);

	Vec3 getTranslation() const;
	void setTranslation(const Vec3& _translation);
	Mat3 getRotation() const;
	void setRotation(const Mat3& _rotation);
	Vec3 getScale() const;
	void setScale(const Vec3& _scale);

	float operator()(int _row, int _col) const
	{
		#ifdef IM3D_MATRIX_ROW_MAJOR
			int i = _row * 4 + _col;
		#else
			int i = _col * 4 + _row;
		#endif
		return m[i];
	}
	float& operator()(int _row, int _col)
	{
		#ifdef IM3D_MATRIX_ROW_MAJOR
			int i = _row * 4 + _col;
		#else
			int i = _col * 4 + _row;
		#endif
		return m[i];
	}

	#ifdef IM3D_MAT4_APP
		IM3D_MAT4_APP
	#endif
};

struct IM3D_API Color
{
	U32 v; // rgba8 (MSB = r)

	constexpr Color(): v(0)                                                  {}
	constexpr Color(U32 _rgba): v(_rgba)                                     {}
	          Color(const Vec4& _rgba);
	          Color(const Vec3& _rgb, float _alpha);
	          Color(float _r, float _g, float _b, float _a = 1.0f);

	operator U32() const                                                     { return v; }

	void set(int _i, float _val)
	{
		_i *= 8;
		U32 mask = 0xff << _i;
		v = (v & ~mask) | (U32(_val * 255.0f) << _i);
	}
	void setR(float _val)                                                    { set(3, _val); }
	void setG(float _val)                                                    { set(2, _val); }
	void setB(float _val)                                                    { set(1, _val); }
	void setA(float _val)                                                    { set(0, _val); }

	float get(int _i) const
	{
		_i *= 8;
		U32 mask = 0xff << _i;
		return float((v & mask) >> _i) / 255.0f;
	}
	float getR() const                                                       { return get(3); }
	float getG() const                                                       { return get(2); }
	float getB() const                                                       { return get(1); }
	float getA() const                                                       { return get(0); }

	U32 getABGR() const
	{
		return 0
			| ((v & (0xff << 24)) >> 24) // r
			| ((v & (0xff << 16)) >>  8) // g
			| ((v & (0xff <<  8)) <<  8) // b
			| ((v & (0xff      )) << 24) // a
			;
	}
};

constexpr Color Color_Black   = Color(0x000000ff);
constexpr Color Color_White   = Color(0xffffffff);
constexpr Color Color_Gray    = Color(0x808080ff);
constexpr Color Color_Red     = Color(0xff0000ff);
constexpr Color Color_Green   = Color(0x00ff00ff);
constexpr Color Color_Blue    = Color(0x0000ffff);
constexpr Color Color_Magenta = Color(0xff00ffff);
constexpr Color Color_Yellow  = Color(0xffff00ff);
constexpr Color Color_Cyan    = Color(0x00ffffff);

constexpr Color Color_Pink    = Color(0xffc0cbff);
constexpr Color Color_Orange  = Color(0xffa500ff);
constexpr Color Color_Gold    = Color(0xffd700ff);
constexpr Color Color_Brown   = Color(0x8b4513ff);
constexpr Color Color_Purple  = Color(0x800080ff);
constexpr Color Color_Teal    = Color(0x008080ff);
constexpr Color Color_Navy    = Color(0x000080ff);

struct alignas(IM3D_VERTEX_ALIGNMENT) VertexData
{
	Vec4   m_positionSize; // xyz = position, w = size
	Color  m_color;        // rgba8 (MSB = r)

	VertexData() {}
	VertexData(const Vec3& _position, float _size, Color _color): m_positionSize(_position, _size), m_color(_color) {}
};

enum DrawPrimitiveType
{
 // order here determines the order in which unsorted primitives are drawn
	DrawPrimitive_Triangles,
	DrawPrimitive_Lines,
	DrawPrimitive_Points,

	DrawPrimitive_Count
};

struct DrawList
{
	Id                m_layerId;
	DrawPrimitiveType m_primType;
	const VertexData* m_vertexData;
	U32               m_vertexCount;
};
typedef void (DrawPrimitivesCallback)(const DrawList& _drawList);

enum TextFlags
{
	TextFlags_AlignLeft    = (1 << 0),
	TextFlags_AlignRight   = (1 << 1),
	TextFlags_AlignTop     = (1 << 3),
	TextFlags_AlignBottom  = (1 << 4),

	TextFlags_Default      = 0 // align center
};

struct alignas(IM3D_VERTEX_ALIGNMENT) TextData
{
	Vec4      m_positionSize;     // xyz = position, w = size
	Color     m_color;            // rgba8 (MSB = r)
	U32       m_flags;            // TextFlags
	U32       m_textLength;       // # chars in the text, excluding null terminator
	U32       m_textBufferOffset; // start of the text in the draw list's text buffer
};

struct TextDrawList
{
	Id              m_layerId;
	const TextData* m_textData;
	U32             m_textDataCount;
	const char*     m_textBuffer;
};

enum Key
{
	Mouse_Left,
	Key_L,
	Key_R,
	Key_S,
	Key_T,

	Key_Count,

// the following map keys -> 'action' states which may be more intuitive
	Action_Select           = Mouse_Left,
	Action_GizmoLocal       = Key_L,
	Action_GizmoRotation    = Key_R,
	Action_GizmoScale       = Key_S,
	Action_GizmoTranslation = Key_T,

	Action_Count
};

enum FrustumPlane
{
	FrustumPlane_Near,
	FrustumPlane_Far,
	FrustumPlane_Top,
	FrustumPlane_Right,
	FrustumPlane_Bottom,
	FrustumPlane_Left,

	FrustumPlane_Count
};

struct AppData
{
	bool   m_keyDown[Key_Count]              = { false };               // Key states.
	Vec4   m_cullFrustum[FrustumPlane_Count] = { Vec4(0.0f) };          // Frustum planes for culling (if culling enabled).
	Vec3   m_cursorRayOrigin                 = Vec3(0.0f);              // World space cursor ray origin.
	Vec3   m_cursorRayDirection              = Vec3(0.0f);              // World space cursor ray direction.
	Vec3   m_worldUp                         = Vec3(0.0f, 1.0f, 0.0f);  // World space 'up' vector.
	Vec3   m_viewOrigin                      = Vec3(0.0f);              // World space render origin (camera position).
	Vec3   m_viewDirection                   = Vec3(0.0f);              // World space view direction.
	Vec2   m_viewportSize                    = Vec2(0.0f);              // Viewport size (pixels).
	float  m_projScaleY                      = 1.0f;                    // Scale factor used to convert from pixel size -> world scale; use tan(fov) for perspective projections, far plane height for ortho.
	bool   m_projOrtho                       = false;                   // If the projection matrix is orthographic.
	float  m_deltaTime                       = 0.0f;                    // Time since previous frame (seconds).
	float  m_snapTranslation                 = 0.0f;                    // Snap value for translation gizmos (world units). 0 = disabled.
	float  m_snapRotation                    = 0.0f;                    // Snap value for rotation gizmos (radians). 0 = disabled.
	float  m_snapScale                       = 0.0f;                    // Snap value for scale gizmos. 0 = disabled.
	bool   m_flipGizmoWhenBehind             = true;                    // Flip gizmo axes when viewed from behind.
	void*  m_appData                         = nullptr;                 // App-specific data.

	DrawPrimitivesCallback* drawCallback     = nullptr; // e.g. void Im3d_Draw(const DrawList& _drawList)

	// Extract cull frustum planes from the view-projection matrix.
	// Set _ndcZNegativeOneToOne = true if the proj matrix maps z from [-1,1] (OpenGL style).
	void setCullFrustum(const Mat4& _viewProj, bool _ndcZNegativeOneToOne);
};

// Minimal vector.
template <typename T>
struct Vector
{
	            Vector()                             {}
	            ~Vector();

	T&          operator[](U32 _i)                   { IM3D_ASSERT(_i < m_size); return m_data[_i]; }
	const T&    operator[](U32 _i) const             { IM3D_ASSERT(_i < m_size); return m_data[_i]; }
	T*          data()                               { return m_data; }
	const T*    data() const                         { return m_data; }

	T&          push_back()                          { if (m_size == m_capacity) { reserve(m_capacity + m_capacity / 2); } return m_data[m_size++]; }
	void        push_back(const T& _v)               { T tmp = _v; if (m_size == m_capacity) { reserve(m_capacity + m_capacity / 2); } m_data[m_size++] = tmp; }
	void        pop_back()                           { IM3D_ASSERT(m_size > 0); --m_size; }
	void        append(const T* _v, U32 _count);
	void        append(const Vector<T>& _other)      { append(_other.data(), _other.size()); }

	T*          begin()                              { return m_data; }
	const T*    begin() const                        { return m_data; }
	T*          end()                                { return m_data + m_size; }
	const T*    end() const                          { return m_data + m_size; }
	T&          front()                              { IM3D_ASSERT(m_size > 0); return m_data[0]; }
	const T&    front() const                        { IM3D_ASSERT(m_size > 0); return m_data[0]; }
	T&          back()                               { IM3D_ASSERT(m_size > 0); return m_data[m_size - 1]; }
	const T&    back() const                         { IM3D_ASSERT(m_size > 0); return m_data[m_size - 1]; }

	U32         size() const                         { return m_size; }
	U32         capacity() const                     { return m_capacity; }
	bool        empty() const                        { return m_size == 0; }

	void        clear()                              { m_size = 0; }
	void        reserve(U32 _capacity);
	void        resize(U32 _size, const T& _val);
	void        resize(U32 _size);

	static void swap(Vector<T>& _a_, Vector<T>& _b_);

private:

	T*   m_data     = nullptr;
	U32  m_size     = 0;
	U32  m_capacity = 0;
};


enum PrimitiveMode
{
	PrimitiveMode_None,
	PrimitiveMode_Points,
	PrimitiveMode_Lines,
	PrimitiveMode_LineStrip,
	PrimitiveMode_LineLoop,
	PrimitiveMode_Triangles,
	PrimitiveMode_TriangleStrip
};
enum GizmoMode
{
	GizmoMode_Translation,
	GizmoMode_Rotation,
	GizmoMode_Scale
};

// Context stores all relevant state - main interface affects the context currently bound via SetCurrentContext().
struct IM3D_API Context
{
	                    Context();
	                    ~Context();

	void                begin(PrimitiveMode _mode);
	void                end();

	void                vertex(const Vec3& _position, float _size, Color _color);
	void                vertex(const Vec3& _position )   { vertex(_position, getSize(), getColor()); }

	void                text(const Vec3& _position, float _size, Color _color, TextFlags _flags, const char* _textStart, const char* _textEnd);
	void                text(const Vec3& _position, float _size, Color _color, TextFlags _flags, const char* _text, va_list _args);

	void                reset();
	void                merge(const Context& _src);
	void                endFrame();
	void                draw(); // DEPRECATED (see Im3d::Draw)

	const DrawList*     getDrawLists() const             { return m_drawLists.data(); }
	U32                 getDrawListCount() const         { return m_drawLists.size(); }

	const TextDrawList* getTextDrawLists() const         { return m_textDrawLists.data();  }
	U32                 getTextDrawListCount() const     { return m_textDrawLists.size();  }


	void                setColor(Color _color)           { m_colorStack.back() = _color;   }
	Color               getColor() const                 { return m_colorStack.back();     }
	void                pushColor(Color _color)          { m_colorStack.push_back(_color); }
	void                popColor()                       { IM3D_ASSERT(m_colorStack.size() > 1); m_colorStack.pop_back(); }

	void                setAlpha(float _alpha)           { m_alphaStack.back() = _alpha;   }
	float               getAlpha() const                 { return m_alphaStack.back();     }
	void                pushAlpha(float _alpha)          { m_alphaStack.push_back(_alpha); }
	void                popAlpha()                       { IM3D_ASSERT(m_alphaStack.size() > 1); m_alphaStack.pop_back(); }

	void                setSize(float _size)             { m_sizeStack.back() = _size;     }
	float               getSize() const                  { return m_sizeStack.back();      }
	void                pushSize(float _size)            { m_sizeStack.push_back(_size);   }
	void                popSize()                        { IM3D_ASSERT(m_sizeStack.size() > 1); m_sizeStack.pop_back(); }

	void                setEnableSorting(bool _enable);
	bool                getEnableSorting() const         { return m_enableSortingStack.back(); }
	void                pushEnableSorting(bool _enable);
	void                popEnableSorting();

	Id                  getLayerId() const               { return m_layerIdStack.back(); }
	void                pushLayerId(Id _layer);
	void                popLayerId();

	void                setMatrix(const Mat4& _mat4)     { m_matrixStack.back() = _mat4;   }
	const Mat4&         getMatrix() const                { return m_matrixStack.back();    }
	void                pushMatrix(const Mat4& _mat4)    { m_matrixStack.push_back(_mat4); }
	void                popMatrix()                      { IM3D_ASSERT(m_matrixStack.size() > 1); m_matrixStack.pop_back(); }

	void                setId(Id _id)                    { m_idStack.back() = _id;   }
	Id                  getId() const                    { return m_idStack.back();  }
	void                pushId(Id _id)                   { m_idStack.push_back(_id); }
	void                popId()                          { IM3D_ASSERT(m_idStack.size() > 1); m_idStack.pop_back(); }

	AppData&            getAppData()                     { return m_appData; }

 // Low-level interface for internal and app-defined gizmos. May be subject to breaking changes.

	bool                gizmoAxisTranslation_Behavior(Id _id, const Vec3& _origin, const Vec3& _axis, float _snap, float _worldHeight, float _worldSize, Vec3* _out_);
	void                gizmoAxisTranslation_Draw(Id _id, const Vec3& _origin, const Vec3& _axis, float _worldHeight, float _worldSize, Color _color);

	bool                gizmoPlaneTranslation_Behavior(Id _id, const Vec3& _origin, const Vec3& _normal, float _snap, float _worldSize, Vec3* _out_);
	void                gizmoPlaneTranslation_Draw(Id _id, const Vec3& _origin, const Vec3& _normal, float _worldSize, Color _color);

	bool                gizmoAxislAngle_Behavior(Id _id, const Vec3& _origin, const Vec3& _axis, float _snap, float _worldRadius, float _worldSize, float* _out_);
	void                gizmoAxislAngle_Draw(Id _id, const Vec3& _origin, const Vec3& _axis, float _worldRadius, float _angle, Color _color, float _minAlpha);

	bool                gizmoAxisScale_Behavior(Id _id, const Vec3& _origin, const Vec3& _axis, float _snap, float _worldHeight, float _worldSize, float *_out_);
	void                gizmoAxisScale_Draw(Id _id, const Vec3& _origin, const Vec3& _axis, float _worldHeight, float _worldSize, Color _color);

	// Convert pixels -> world space size based on distance between _position and view origin.
	float               pixelsToWorldSize(const Vec3& _position, float _pixels);
	// Convert world space size -> pixels based on distance between _position and view origin.
	float               worldSizeToPixels(const Vec3& _position, float _pixels);
	// Blend between _min and _max based on distance betwen _position and view origin.
	int                 estimateLevelOfDetail(const Vec3& _position, float _worldSize, int _min = 4, int _max = 256);

	// Make _id hot if _depth < m_hotDepth && _intersects.
	bool                makeHot(Id _id, float _depth, bool _intersects);
	// Make _id active.
	void                makeActive(Id _id);
	// Reset the acive/hot ids and the hot depth.
	void                resetId();

	// Interpret key state.
	bool                isKeyDown(Key _key) const     { return m_keyDownCurr[_key]; }
	bool                wasKeyPressed(Key _key) const { return m_keyDownCurr[_key] && !m_keyDownPrev[_key]; }

	// Visibiity tests for culling.
	bool                isVisible(const VertexData* _vdata, DrawPrimitiveType _prim); // per-vertex
	bool                isVisible(const Vec3& _origin, float _radius);                // sphere
	bool                isVisible(const Vec3& _min, const Vec3& _max);                // axis-aligned box

 // Gizmo state.

	bool                m_gizmoLocal;         // Global mode selection for gizmos.
	GizmoMode           m_gizmoMode;          //               "
	Id                  m_activeId;           // Currently active gizmo. If set, this is the same as m_hotId.
	Id                  m_hotId;              // ID of the current 'hot' gizmo (nearest intersecting gizmo along the cursor ray). NB this is the id of the *sub* gizmo, not the app-specified ID.
	float               m_hotDepth;           // Depth of the current hot gizmo along the cursor ray, for handling occlusion.
	Id                  m_appId;              // Current ID *without* the hashing the ID stack (= _id arg to Gizmo* functions).
	Id                  m_appActiveId;		  // Copied from m_appId for the current active gizmo.
	Id                  m_appHotId;			  // Copied from m_appId for the current 'hot' gizmo.
	Vec3                m_gizmoStateVec3;     // Stored state for the active gizmo.
	Mat3                m_gizmoStateMat3;     //               "
	float               m_gizmoStateFloat;    //               "
	float               m_gizmoHeightPixels;  // Height/radius of gizmos.
	float               m_gizmoSizePixels;    // Thickness of gizmo lines.


 // Stats, debugging.

	// Return the total number of primitives (sorted + unsorted) of the given _type in all layers.
	U32                 getPrimitiveCount(DrawPrimitiveType _type) const;

	// Return the total number of text primitives in all layers.
	U32                 getTextCount() const;

	// Return the number of layers.
	U32                 getLayerCount() const { return m_layerIdMap.size(); }

private:

 // State stacks.
	Vector<Color>       m_colorStack;
	Vector<float>       m_alphaStack;
	Vector<float>       m_sizeStack;
	Vector<bool>        m_enableSortingStack;
	Vector<Mat4>        m_matrixStack;
	Vector<Id>          m_idStack;
	Vector<Id>          m_layerIdStack;

 // Vertex data: one list per layer, per primitive type, *2 for sorted/unsorted.
	typedef Vector<VertexData> VertexList;
	Vector<VertexList*> m_vertexData[2];                    // Each layer is DrawPrimitive_Count consecutive lists.
	int                 m_vertexDataIndex;                  // 0, or 1 if sorting enabled.
	Vector<Id>          m_layerIdMap;                       // Map Id -> vertex data index.
	int                 m_layerIndex;                       // Index of the currently active layer in m_layerIdMap.
	Vector<DrawList>    m_drawLists;                        // All draw lists for the current frame, available after calling endFrame() before calling reset().
	bool                m_sortCalled;                       // Avoid calling sort() during every call to draw().
	bool                m_endFrameCalled;                   // For assert, if vertices are pushed after endFrame() was called.

 // Text data: one list per layer.
	typedef Vector<TextData> TextList;
	Vector<TextList*>    m_textData;
	Vector<char>         m_textBuffer;
	Vector<TextDrawList> m_textDrawLists;

 // Primitive state.
	PrimitiveMode       m_primMode;
	DrawPrimitiveType   m_primType;
	U32                 m_firstVertThisPrim;                // Index of the first vertex pushed during this primitive.
	U32                 m_vertCountThisPrim;                // # calls to vertex() since the last call to begin().
	Vec3                m_minVertThisPrim;
	Vec3                m_maxVertThisPrim;

 // App data.
	AppData             m_appData;
	bool                m_keyDownCurr[Key_Count];           // Key state captured during reset().
	bool                m_keyDownPrev[Key_Count];           // Key state from previous frame.
	Vec4                m_cullFrustum[FrustumPlane_Count];  // Optimized frustum planes from m_appData.m_cullFrustum.
	int                 m_cullFrustumCount;                 // # valid frustum planes in m_cullFrustum.

	// Sort primitive data.
	void                sort();

	// Return -1 if _id not found.
	int                 findLayerIndex(Id _id) const;

	// Access the current vertex/text data based on m_layerIndex.
	VertexList*         getCurrentVertexList();
	TextList*           getCurrentTextList();
};

namespace internal {

#if IM3D_THREAD_LOCAL_CONTEXT_PTR
	#define IM3D_THREAD_LOCAL thread_local
#else
	#define IM3D_THREAD_LOCAL
#endif

extern IM3D_THREAD_LOCAL Context* g_CurrentContext;

}

inline AppData&            GetAppData()                                                                                     { return GetContext().getAppData(); }
inline void                NewFrame()                                                                                       { GetContext().reset(); }
inline void                EndFrame()                                                                                       { GetContext().endFrame(); }
inline void                Draw()                                                                                           { GetContext().draw(); }

inline const DrawList*     GetDrawLists()                                                                                   { return GetContext().getDrawLists(); }
inline U32                 GetDrawListCount()                                                                               { return GetContext().getDrawListCount(); }

inline const TextDrawList* GetTextDrawLists()                                                                               { return GetContext().getTextDrawLists(); }
inline U32                 GetTextDrawListCount()                                                                           { return GetContext().getTextDrawListCount(); }

inline void                BeginPoints()                                                                                    { GetContext().begin(PrimitiveMode_Points); }
inline void                BeginLines()                                                                                     { GetContext().begin(PrimitiveMode_Lines); }
inline void                BeginLineLoop()                                                                                  { GetContext().begin(PrimitiveMode_LineLoop); }
inline void                BeginLineStrip()                                                                                 { GetContext().begin(PrimitiveMode_LineStrip); }
inline void                BeginTriangles()                                                                                 { GetContext().begin(PrimitiveMode_Triangles); }
inline void                BeginTriangleStrip()                                                                             { GetContext().begin(PrimitiveMode_TriangleStrip); }
inline void                End()                                                                                            { GetContext().end(); }

inline void                Vertex(const Vec3& _position)                                                                    { GetContext().vertex(_position, GetContext().getSize(), GetContext().getColor()); }
inline void                Vertex(const Vec3& _position, Color _color)                                                      { GetContext().vertex(_position, GetContext().getSize(), _color); }
inline void                Vertex(const Vec3& _position, float _size)                                                       { GetContext().vertex(_position, _size, GetContext().getColor()); }
inline void                Vertex(const Vec3& _position, float _size, Color _color)                                         { GetContext().vertex(_position, _size, _color); }
inline void                Vertex(float _x, float _y, float _z)                                                             { Vertex(Vec3(_x, _y, _z)); }
inline void                Vertex(float _x, float _y, float _z, Color _color)                                               { Vertex(Vec3(_x, _y, _z), _color); }
inline void                Vertex(float _x, float _y, float _z, float _size)                                                { Vertex(Vec3(_x, _y, _z), _size); }
inline void                Vertex(float _x, float _y, float _z, float _size, Color _color)                                  { Vertex(Vec3(_x, _y, _z), _size, _color); }

inline void                PushDrawState()                                                                                  { Context& ctx = GetContext(); ctx.pushColor(ctx.getColor()); ctx.pushAlpha(ctx.getAlpha()); ctx.pushSize(ctx.getSize()); ctx.pushEnableSorting(ctx.getEnableSorting()); }
inline void                PopDrawState()                                                                                   { Context& ctx = GetContext(); ctx.popColor(); ctx.popAlpha(); ctx.popSize(); ctx.popEnableSorting(); }

inline void                PushColor()                                                                                      { GetContext().pushColor(GetContext().getColor()); }
inline void                PushColor(Color _color)                                                                          { GetContext().pushColor(_color); }
inline void                PopColor()                                                                                       { GetContext().popColor(); }
inline void                SetColor(Color _color)                                                                           { GetContext().setColor(_color); }
inline void                SetColor(float _r, float _g, float _b, float _a)                                                 { GetContext().setColor(Color(_r, _g, _b, _a)); }
inline Color               GetColor()                                                                                       { return GetContext().getColor(); }

inline void                PushAlpha()                                                                                      { GetContext().pushAlpha(GetContext().getAlpha()); }
inline void                PushAlpha(float _alpha)                                                                          { GetContext().pushAlpha(_alpha); }
inline void                PopAlpha()                                                                                       { GetContext().popAlpha(); }
inline void                SetAlpha(float _alpha)                                                                           { GetContext().setAlpha(_alpha); }
inline float               GetAlpha()                                                                                       { return GetContext().getAlpha(); }

inline void                PushSize()                                                                                       { GetContext().pushSize(GetContext().getAlpha()); }
inline void                PushSize(float _size)                                                                            { GetContext().pushSize(_size); }
inline void                PopSize()                                                                                        { GetContext().popSize(); }
inline void                SetSize(float _size)                                                                             { GetContext().setSize(_size); }
inline float               GetSize()                                                                                        { return GetContext().getSize(); }

inline void                PushEnableSorting()                                                                              { GetContext().pushEnableSorting(GetContext().getEnableSorting()); }
inline void                PushEnableSorting(bool _enable)                                                                  { GetContext().pushEnableSorting(_enable); }
inline void                PopEnableSorting()                                                                               { GetContext().popEnableSorting(); }
inline void                EnableSorting(bool _enable)                                                                      { GetContext().setEnableSorting(_enable); }

inline void                PushMatrix()                                                                                     { GetContext().pushMatrix(GetContext().getMatrix()); }
inline void                PushMatrix(const Mat4& _mat4)                                                                    { GetContext().pushMatrix(_mat4); }
inline void                PopMatrix()                                                                                      { GetContext().popMatrix(); }
inline void                SetMatrix(const Mat4& _mat4)                                                                     { GetContext().setMatrix(_mat4); }
inline void                SetIdentity()                                                                                    { GetContext().setMatrix(Mat4(1.0f)); }

inline void                PushId()                                                                                         { GetContext().pushId(GetContext().getId()); }
inline void                PushId(Id _id)                                                                                   { GetContext().pushId(_id); }
inline void                PushId(const char* _str)                                                                         { GetContext().pushId(MakeId(_str)); }
inline void                PushId(const void* _ptr)                                                                         { GetContext().pushId(MakeId(_ptr)); }
inline void                PushId(int _i)                                                                                   { GetContext().pushId(MakeId(_i)); }
inline void                PopId()                                                                                          { GetContext().popId(); }
inline Id                  GetId()                                                                                          { return GetContext().getId(); }

inline void                PushLayerId()                                                                                    { GetContext().pushLayerId(GetContext().getLayerId()); }
inline void                PushLayerId(Id _layer)                                                                           { GetContext().pushLayerId(_layer); }
inline void                PushLayerId(const char* _str)                                                                    { PushLayerId(MakeId(_str)); }
inline void                PopLayerId()                                                                                     { GetContext().popLayerId(); }
inline Id                  GetLayerId()                                                                                     { return GetContext().getLayerId(); }

inline bool                GizmoTranslation(const char* _id, float _translation_[3], bool _local)                           { return GizmoTranslation(MakeId(_id), _translation_, _local);   }
inline bool                GizmoRotation(const char* _id, float _rotation_[3*3], bool _local)                               { return GizmoRotation(MakeId(_id), _rotation_, _local);}
inline bool                GizmoScale(const char* _id, float _scale_[3])                                                    { return GizmoScale(MakeId(_id), _scale_); }
inline bool                Gizmo(const char* _id, float _translation_[3], float _rotation_[3*3], float _scale_[3])          { return Gizmo(MakeId(_id), _translation_, _rotation_, _scale_); }
inline bool                Gizmo(const char* _id, float _transform_[4*4])                                                   { return Gizmo(MakeId(_id), _transform_); }
inline Id                  GetActiveId()                                                                                    { return GetContext().m_appActiveId;}
inline Id                  GetHotId()                                                                                       { return GetContext().m_appHotId; }

inline bool                IsVisible(const Vec3& _origin, float _radius)                                                    { return GetContext().isVisible(_origin, _radius); }
inline bool                IsVisible(const Vec3& _min, const Vec3& _max)                                                    { return GetContext().isVisible(_min, _max);}

inline Context&            GetContext()                                                                                     { return *internal::g_CurrentContext; }
inline void                SetContext(Context& _ctx)                                                                        { internal::g_CurrentContext = &_ctx; }
inline void                MergeContexts(Context& _dst_, const Context& _src)                                               { _dst_.merge(_src); }

} // namespac Im3d
