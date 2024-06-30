#pragma once

#include <iostream>
#include <glm/glm.hpp>
#include <glm/gtx/string_cast.hpp>
#include <glm/gtx/norm.hpp>
namespace hlab
{
	class Circle
	{
	public:
		glm::vec2 center;
		float radius;
		float radiusSquared;
		glm::vec4 color;

		Circle(const glm::vec2 &center, const float radius, const glm::vec4 &color)
			: center(center), color(color), radius(radius), radiusSquared(radius* radius)
		{
		}

		// x는 벡터이기 때문에 내부적으로 x좌표와 y좌표를 모두 갖고 있음
		// screen 좌표계에서는 x좌표와 y좌표가 int지만 float로 변환
		bool IsInside(const glm::vec2 &x)
		{
			// TODO: 여기에 원의 방정식을 이용해서 x가 원 안에 들어 있는지 아닌지에 따라
			//		true나 false 반환
			
			// 첫번째 방법
			{
				const float distance = sqrt(pow(x.x - center.x, 2) + pow(x.y - center.y, 2));
				return distance <= this->radius ? true : false;
			}
			// 두번째 방법
			{
				/*sqrt 연산이 느림
				radius의 제곱을 미리 저장
				거리의 제곱 = 자기 자신 dot product*/
				const float distance = glm::dot(center.x, center.x);
				return radiusSquared >= distance ? true : false;
			}
			// 세번째 방법
			{
				// glm 연산
				const float distance = glm::length2(x - center);
				return distance <= this->radius ? true : false;
			}
			
		}
	};
}
