#include "Shared.hpp"

void ApplyInput(Vector2 *position, uint8_t input, uint32_t radius)
{
    const float MOVE_SPEED = (10.0f) / std::max(radius, (uint32_t)(10));

    if (input & (1 << 0))
        position->y -= MOVE_SPEED;
    if (input & (1 << 1))
        position->y += MOVE_SPEED;
    if (input & (1 << 2))
        position->x += MOVE_SPEED;
    if (input & (1 << 3))
        position->x -= MOVE_SPEED;
}