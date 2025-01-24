#pragma once

enum class CSGType {
    ADDITIVE_CUBE,
    SUBTRACTIVE,
    DOOR,
    WINDOW,
    ADDITIVE_PLANE
};

enum class CSGContainerType {
    UNDEFINED,
    ADDITIVE_CUBE,
    WALL_PLANE,
    FLOOR_PLANE,
    CEILING_PLANE,
};

enum class BrushShape {
    CUBE,
    PLANE
};

enum BrushType {
    AIR_BRUSH,
    SOLID_BRUSH
};
