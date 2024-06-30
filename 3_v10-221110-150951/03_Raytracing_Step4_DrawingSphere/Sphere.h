#pragma once

#include "Hit.h"
#include "Ray.h"

namespace hlab
{
    using namespace glm;

    class Sphere
    {
    public:
        glm::vec3 center;
        float radius;
        glm::vec3 color; // 뒤에서 '재질(material)'로 확장

        Sphere(const glm::vec3 &center, const float radius, const glm::vec3 &color)
            : center(center), color(color), radius(radius)
        {
        }

        // Wikipedia Line–sphere intersection
        // https://en.wikipedia.org/wiki/Line-sphere_intersection
        Hit IntersectRayCollision(Ray &ray)
        {
            Hit hit = Hit{-1.0f, vec3(0.0f), vec3(0.0f)}; // d가 음수이면 충돌을 안한 것으로 가정

            /*
             * hit.d = ... // 광선의 시작점으로부터 충돌지점까지의 거리 (float)
             * hit.point = ... // 광선과 구가 충돌한 지점의 위치 (vec3)
             * hit.normal = .. // 충돌 지점에서 구의 단위 법선 벡터(unit normal vector)
             */

            // const float a = glm::dot(ray.dir, ray.dir); // dir이 unit vector라면 a는 1.0f라서 생략 가능
            const float b = 2.0f * glm::dot(ray.dir, ray.start - center);
            const float c = glm::dot(ray.start - center, ray.start - center) - pow(radius, 2);
            // a,b,c,를 구해서 distance를 구한다
            const float nabla = pow(b / 2.f, 2) - c; // (b / 2)의 제곱  - c
            if (nabla > 0)
            {
                const float d1 = -b / 2.f + sqrt(nabla);
				const float d2 = -b / 2.f - sqrt(nabla);
				hit.d = glm::min(d1, d2); // d를 구한 후

				hit.point = ray.start * ray.dir * hit.d;
                // normalize(b−a)는 a에서 b를 향하는 방향
                hit.normal = glm::normalize(hit.point - center);
			}


            return hit;
        }
    };
}
