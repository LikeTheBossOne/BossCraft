#include "Physics.h"

#include <algorithm>

#include "World.h"

bool Physics::RayCast(glm::vec3 rayPos, glm::vec3 rayDir, RayCastHit& hit, float dist, World* world)
{
    glm::ivec3 roundedRayPos =
    {
        floor(rayPos.x),
        floor(rayPos.y),
        floor(rayPos.z)
    };
    
    glm::ivec3 step{};
    glm::vec3 deltaDist{};
    glm::vec3 sideDist{};

    glm::vec3 minPos =
    {
        std::min(rayPos.x, rayPos.x + (rayDir.x * dist)),
        std::min(rayPos.y, rayPos.y + (rayDir.y * dist)),
        std::min(rayPos.z, rayPos.z + (rayDir.z * dist)),
    };
    glm::vec3 maxPos =
    {
        std::max(rayPos.x, rayPos.x + (rayDir.x * dist)),
        std::max(rayPos.y, rayPos.y + (rayDir.y * dist)),
        std::max(rayPos.z, rayPos.z + (rayDir.z * dist)),
    };

    if (rayDir.x == 0)
    {
        step.x = 0;
        deltaDist.x = rayDir.x;
        sideDist.x = (rayPos.x - roundedRayPos.x) * deltaDist.x;
    }
    else if (rayDir.x > 0)
    {
        step.x = 1;
        deltaDist.x = step.x / rayDir.x;
        sideDist.x = ((roundedRayPos.x + 1) - rayPos.x) * deltaDist.x;
    }
    else
    {
        step.x = -1;
        deltaDist.x = step.x / rayDir.x;
        sideDist.x = (rayPos.x - roundedRayPos.x) * deltaDist.x;
    }

    if (rayDir.y == 0)
    {
        step.y = 0;
        deltaDist.y = rayDir.y;
        sideDist.y = (rayPos.y - roundedRayPos.y) * deltaDist.y;
    }
    else if (rayDir.y > 0)
    {
        step.y = 1;
        deltaDist.y = step.y / rayDir.y;
        sideDist.y = ((roundedRayPos.y + 1) - rayPos.y) * deltaDist.y;
    }
    else
    {
        step.y = -1;
        deltaDist.y = step.y / rayDir.y;
        sideDist.y = (rayPos.y - roundedRayPos.y) * deltaDist.y;
    }

    if (rayDir.z == 0)
    {
        step.z = 0;
        deltaDist.z = rayDir.z;
        sideDist.z = (rayPos.z - roundedRayPos.z) * deltaDist.z;
    }
    else if (rayDir.z > 0)
    {
        step.z = 1;
        deltaDist.z = step.z / rayDir.z;
        sideDist.z = ((roundedRayPos.z + 1) - rayPos.z) * deltaDist.z;
    }
    else
    {
        step.z = -1;
        deltaDist.z = step.z / rayDir.z;
        sideDist.z = (rayPos.z - roundedRayPos.z) * deltaDist.z;
    }

    do
    {
        glm::ivec3 normal;
        if (sideDist.x < sideDist.y)
        {
            if (sideDist.x < sideDist.z)
            {
                roundedRayPos.x += step.x;
                if (roundedRayPos.x < minPos.x || roundedRayPos.x > maxPos.x)
                {
                    return false;
                }
                sideDist.x += deltaDist.x;
                normal = { -step.x, 0, 0 };
            }
            else
            {
                roundedRayPos.z += step.z;
                if (roundedRayPos.z < minPos.z || roundedRayPos.z > maxPos.z)
                {
                    return false;
                }
                sideDist.z += deltaDist.z;
                normal = { 0, 0, -step.z };
            }
        }
        else
        {
            if (sideDist.y < sideDist.z)
            {
                roundedRayPos.y += step.y;
                if (roundedRayPos.y < minPos.y || roundedRayPos.y > maxPos.y)
                {
                    return false;
                }
                sideDist.y += deltaDist.y;
                normal = { 0, -step.y, 0 };
            }
            else
            {
                roundedRayPos.z += step.z;
                if (roundedRayPos.z < minPos.z || roundedRayPos.z > maxPos.z)
                {
                    return false;
                }
                sideDist.z += deltaDist.z;
                normal = { 0, 0, -step.z };
            }
        }

        if (world->GetBlockAtAbsPos(roundedRayPos) != 0)
        {
            hit =
            RayCastHit{
            	roundedRayPos,
            	normal
            };
            return true;
        }

    } while (true);
}
