#pragma once

enum class LoadingState {
    AWAITING_LOADING_FROM_DISK,
    LOADING_FROM_DISK,
    LOADING_COMPLETE
};

enum class BakingState {
    AWAITING_BAKE,
    BAKE_COMPLETE
};